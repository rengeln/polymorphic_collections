//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP
#define POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP

#include <algorithm>
#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/front.hpp>
#include <boost/type_traits.hpp>

#include "policy.hpp"
#include "detail/common.hpp"
#include "detail/accumulator.hpp"

namespace polymorphic_collections
{
    //
    //  Polymorphic interface for aggregating items into a collection.
    //
    //  Parameters:
    //      [template] T
    //          Value type held by the collection.
    //      [template] P1, ...
    //          Policies (see policy.hpp)
    //
    template <typename T, typename P1 = no_lock>
    class accumulator : public P1, 
                        public boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef accumulator<T, P1> this_type;

        //  Size, in bytes, of the internal storage buffer used to store the
        //  type-erased adapter object, thereby avoiding a heap allocation.
        static const size_t internal_storage_size = 32 - sizeof(ptrdiff_t);

        accumulator()
        : m_adapter(nullptr)
        {
        }

        template <typename U>
        accumulator(detail::accumulator_adapter_proxy<T, U>&& adapter)
        {
            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::accumulator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::accumulator_adapter_proxy<T, U>(std::move(adapter));
            }
        }

        template <typename _P1>
        accumulator(accumulator<T, _P1>&& rhs)
        {
            if (reinterpret_cast<char*>(rhs.m_adapter) == rhs.m_storage)
            {
                m_adapter = rhs.m_adapter->move(m_storage);
            }
            else
            {
                m_adapter = rhs.m_adapter;
            }
            rhs.m_adapter = nullptr;
        }

        template <typename U>
        accumulator(U&& param)
        : m_adapter(nullptr)
        {
            *this = detail::make_accumulator_adapter_proxy<T>(std::forward<U>(param));
        }

        ~accumulator()
        {
            dispose();
        }

        template <typename _P1>
        this_type& operator=(accumulator<T, _P1>&& rhs)
        {
            dispose();

            if (reinterpret_cast<char*>(rhs.m_adapter) == rhs.m_storage)
            {
                m_adapter = rhs.m_adapter->move(m_storage);
            }
            else
            {
                m_adapter = rhs.m_adapter;
            }
            rhs.m_adapter = nullptr;
            return *this;
        }

        template <typename U>
        this_type& operator=(detail::accumulator_adapter_proxy<T, U>&& adapter)
        {
            dispose();

            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::accumulator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::accumulator_adapter_proxy<T, U>(std::move(adapter));
            }

            return *this;
        }

        template <typename U>
        this_type& operator=(U&& param)
        {
            *this = detail::make_accumulator_adapter_proxy<T>(std::forward<U>(param));
            return *this;
        }

        accumulator<T>& add(const T& value)
        {
            if (lock())
            {
                if (!m_adapter)
                {
                    throw std::overflow_error("accumulator::add()");
                }
                T new_value = value;
                m_adapter->add(std::move(new_value));
                unlock();
            }
            return *this;
        }
        
        accumulator<T>& add(T&& value)
        {
            if (lock())
            {
                if (!m_adapter)
                {
                    throw std::overflow_error("accumulator::add()");
                }
                m_adapter->add(std::move(value));
                unlock();
            }
            return *this;
        }

        accumulator<T>& operator+=(const T& value)
        {
            return add(value);
        }

        accumulator<T>& operator+=(T&& value)
        {
            return add(std::move(value));
        }

    private:
        void dispose()
        {
            if (m_adapter)
            {
                if (reinterpret_cast<char*>(m_adapter) == m_storage)
                {
                    m_adapter->~accumulator_adapter_interface<T>();
                }
                else
                {
                    delete m_adapter;
                }
                m_adapter = nullptr;
            }
        }
        detail::accumulator_adapter_interface<T>* m_adapter;
        char m_storage[internal_storage_size];
    };

    template <typename T>
    inline auto make_accumulator(T&& param) -> accumulator<decltype(detail::get_accumulator_value_type(detail::make_accumulator_adapter(std::forward<T>(param))))>
    {
        typedef decltype(detail::get_accumulator_value_type(detail::make_accumulator_adapter(std::forward<T>(param)))) value_type;
        return accumulator<value_type>(detail::make_accumulator_adapter_proxy<value_type>(std::forward<T>(param)));
    }

    template <typename T, typename F>
    inline accumulator<T> make_accumulator(F&& func, decltype(func(*(T*)0))* dummy = nullptr)
    {
        //  Dummy parameter ensures this is specialized only for callables
        return accumulator<T>(detail::accumulator_adapter_proxy<T, detail::functional_accumulator_adapter<F, T>>
            (detail::functional_accumulator_adapter<F, T>(std::forward<F>(func))));
    }

    template <typename T>
    inline accumulator<typename std::iterator_traits<T>::value_type> make_accumulator(const T& begin, const T& end)
    {
        typedef typename std::iterator_traits<T>::value_type value_type;
        return accumulator<value_type>(detail::accumulator_adapter_proxy<value_type, detail::iterator_accumulator_adapter<T>>
            (detail::iterator_accumulator_adapter<T>(begin, end)));
    }

    template <typename T>
    inline accumulator<T> make_accumulator(T* ptr, size_t count)
    {
        return accumulator<T>(detail::accumulator_adapter_proxy<T, detail::iterator_accumulator_adapter<T*>>
            (detail::iterator_accumulator_adapter<T*>(ptr, ptr + count)));
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP

