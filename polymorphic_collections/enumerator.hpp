////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
//  See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_ENUMERATOR_HPP
#define POLYMORPHIC_COLLECTIONS_ENUMERATOR_HPP

#include <algorithm>
#include <iterator>
#include <utility>

#include <boost/optional.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility.hpp>

#include "policy.hpp"
#include "detail/common.hpp"
#include "detail/enumerator.hpp"

namespace polymorphic_collections
{
    //
    //  Polymorphic interface for linear navigation of a collection.
    //
    //  Parameters:
    //      [template] T
    //          Value type held by the collection. As the enumerator will expose
    //          references to the objects in the collection, this should be const
    //          if the objects are immutable.
    //      [template] P1, ...
    //          Policies (see policy.hpp)
    //
    template <typename T, typename P1 = no_lock>
    class enumerator : public P1, 
                       public boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef enumerator<T, P1> this_type;

        //  Size, in bytes, of the internal storage buffer used to store the
        //  type-erased adapter object, thereby avoiding a heap allocation.
        static const size_t internal_storage_size = 32 - sizeof(ptrdiff_t);

        enumerator()
        : m_adapter(nullptr)
        {
            
        }

        template <typename U>
        enumerator(detail::enumerator_adapter_proxy<T, U>&& adapter)
        {
            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::enumerator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::enumerator_adapter_proxy<T, U>(std::move(adapter));
            }
        }

        template <typename _P1>
        enumerator(enumerator<T, _P1>&& rhs)
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
        enumerator(U&& param)
        : m_adapter(nullptr)
        {
            *this = detail::make_enumerator_adapter_proxy<T>(std::forward<U>(param));
        }

        ~enumerator()
        {
            dispose();
        }

        template <typename _P1>
        this_type& operator=(enumerator<T, _P1>&& rhs)
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
        this_type& operator=(detail::enumerator_adapter_proxy<T, U>&& adapter)
        {
            dispose();

            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::enumerator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::enumerator_adapter_proxy<T, U>(std::move(adapter));
            }

            return *this;
        }

        template <typename U>
        this_type& operator=(U&& param)
        {
            *this = detail::make_enumerator_adapter_proxy<T>(std::forward<U>(param));
            return *this;
        }

        boost::optional<T&> next()
        {
            if (!m_adapter)
            {
                return boost::none;
            }
            else
            {
                if (lock())
                {
                    boost::optional<T&> value = m_adapter->next();
                    unlock();
                    return value;
                }
                else
                {
                    return boost::none;
                }
            }
        }

    private:
        template <typename U, typename _P1>
        friend class enumerator;

        void dispose()
        {
            if (m_adapter)
            {
                if (reinterpret_cast<char*>(m_adapter) == m_storage)
                {
                    m_adapter->~enumerator_adapter_interface<T>();
                }
                else
                {
                    delete m_adapter;
                }
                m_adapter = nullptr;
            }
        }
        detail::enumerator_adapter_interface<T>* m_adapter;
        char m_storage[internal_storage_size];
    };

    //
    //  Makes an explicitly-typed enumerator out of the source object.
    //
    //  Example:
    //      std::vector<int> v;
    //      auto e = make_typed_enumerator<const int>(v);   // enumerator<const int>
    //
    //  Parameters:
    //      [template] U
    //          Enumerator type.
    //      [in] param
    //          Parameter which will be passed to make_enumerator_adapter().
    //
    template <typename U, typename T>
    inline auto make_typed_enumerator(T&& param) -> enumerator<U>
    {
        return enumerator<U>(detail::make_enumerator_adapter_proxy<U>(std::forward<T>(param)));
    }

    //
    //  Makes an implicitly-typed enumerator out of the source object.
    //
    //  Example:
    //      std::vector<int> v;
    //      auto e = make_enumerator(v);        (enumerator<int>)
    //
    template <typename T>
    inline auto make_enumerator(T&& param) -> enumerator<decltype(detail::get_enumerator_value_type(detail::make_enumerator_adapter(std::forward<T>(param))))>
    {
        typedef decltype(detail::get_enumerator_value_type(detail::make_enumerator_adapter(std::forward<T>(param)))) value_type;
        return enumerator<value_type>(detail::make_enumerator_adapter_proxy<value_type>(std::forward<T>(param)));
    }

    //
    //  Makes an enumerator out of a pointer and an element count.
    //
    //  Example:
    //      Foo* foos = new Foo[16];
    //      auto e = make_enumerator(foos, 16);
    //
    template <typename T>
    inline enumerator<T> make_enumerator(T* ptr, size_t count)
    {
        return enumerator<T>(detail::enumerator_adapter_proxy<T, detail::iterator_enumerator_adapter<T*>>
            (detail::make_enumerator_adapter(ptr, ptr + count)));
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ENUMERATOR_HPP
