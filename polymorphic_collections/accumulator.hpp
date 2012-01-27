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
#include "detail.hpp"

namespace polymorphic_collections
{
    template <typename T>
    class accumulator_adapter_interface
    {
    public:
        typedef T value_type;
        typedef accumulator_adapter_interface<T> this_type;

        virtual ~accumulator_adapter_interface() = 0 { }
        virtual void add(value_type&&) = 0;
        virtual this_type* move(void*) = 0;
    };

    template <typename T, typename A>
    class accumulator_adapter_proxy : public accumulator_adapter_interface<T>,
                                      public boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef A adapter_type;
        typedef accumulator_adapter_proxy<T, A> this_type;

        accumulator_adapter_proxy(adapter_type&& adapter)
        : m_adapter(std::move(adapter))
        {
        }

        accumulator_adapter_proxy(this_type&& rhs)
        : m_adapter(std::move(rhs.m_adapter))
        {
        }

        virtual ~accumulator_adapter_proxy()
        {
        }

        virtual void add(value_type&& value)
        {
            return m_adapter.add(std::move(value));
        }

        virtual accumulator_adapter_interface<T>* move(void* ptr)
        {
            return new(ptr) this_type(std::move(m_adapter));
        }

    private:
        adapter_type m_adapter;
    };

    template <typename T>
    class accumulator : public boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef accumulator<T> this_type;

        //  Size, in bytes, of the internal storage buffer used to store the
        //  type-erased adapter object, thereby avoiding a heap allocation.
        static const size_t internal_storage_size = 32 - sizeof(ptrdiff_t);

        accumulator()
        : m_adapter(nullptr)
        {
        }

        template <typename U>
        accumulator(accumulator_adapter_proxy<T, U>&& adapter)
        {
            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) accumulator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new accumulator_adapter_proxy<T, U>(std::move(adapter));
            }
        }

        accumulator(this_type&& rhs)
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
            *this = make_accumulator_adapter_proxy<T>(std::forward<U>(param));
        }

        ~accumulator()
        {
            dispose();
        }

        this_type& operator=(this_type&& rhs)
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
        this_type& operator=(accumulator_adapter_proxy<T, U>&& adapter)
        {
            dispose();

            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) accumulator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new accumulator_adapter_proxy<T, U>(std::move(adapter));
            }

            return *this;
        }

        template <typename U>
        this_type& operator=(U&& param)
        {
            *this = make_accumulator_adapter_proxy<T>(std::forward<U>(param));
            return *this;
        }

        accumulator<T>& add(const T& value)
        {
            if (!m_adapter)
            {
                throw std::overflow_error("accumulator::add()");
            }
            T new_value = value;
            m_adapter->add(std::move(new_value));
            return *this;
        }
        
        accumulator<T>& add(T&& value)
        {
            if (!m_adapter)
            {
                throw std::overflow_error("accumulator::add()");
            }
            m_adapter->add(std::move(value));
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
        accumulator_adapter_interface<T>* m_adapter;
        char m_storage[internal_storage_size];
    };

    template <typename T, typename U>
    inline auto make_accumulator_adapter_proxy(U&& param) -> accumulator_adapter_proxy<T, decltype(make_accumulator_adapter(std::forward<U>(param)))>
    {
        return accumulator_adapter_proxy<T, decltype(make_accumulator_adapter(std::forward<U>(param)))>(make_accumulator_adapter(std::forward<U>(param)));
    }

    namespace detail
    {
        //
        //  Workaround for a limitation in Visual C++ to do
        //      declspec(foo())::value_type
        //
        template <typename T>
        inline auto get_accumulator_value_type(const T& adapter) -> typename T::value_type
        {
        }
    }

    //
    //  Accumulator adapter encapsulating a collection implementing a push_back method.
    //
    //  Parameters:
    //      [template] T
    //          Collection type.
    //
    template <typename T>
    class push_back_accumulator_adapter
    {
    public:
        typedef T collection_type;
        typedef typename T::value_type value_type;
        typedef push_back_accumulator_adapter<T> this_type;

        push_back_accumulator_adapter(collection_type& collection)
        : m_collection(collection)
        {
        }

        void add(value_type&& value)
        {
            m_collection.push_back(std::move(value));
        }

    private:
        T& m_collection;
    };

    //
    //  Accumulator adapter encapsulating two output iterators representing a range.
    //
    template <typename T>
    class iterator_accumulator_adapter
    {
    public:
        typedef T iterator_type;
        typedef typename std::iterator_traits<iterator_type>::value_type value_type;
        typedef typename iterator_accumulator_adapter<iterator_type> this_type;

        iterator_accumulator_adapter(const iterator_type& begin, const iterator_type& end)
        : m_begin(begin), m_end(end)
        {
        }

        void add(value_type&& value)
        {
            if (m_begin == m_end)
            {
                throw std::overflow_error("iterator_accumulator_adapter::add()");
            }
            *m_begin = std::move(value);
            ++m_begin;
        }

    private:
        iterator_type m_begin, m_end;
    };

    //
    //  Accumulator adapter encapsulating a functor.
    //
    template <typename F, typename T>
    class functional_accumulator_adapter
    {
    public:
        typedef F function_type;
        typedef T value_type;

        functional_accumulator_adapter(const function_type& func)
        : m_func(func)
        {
        }

        functional_accumulator_adapter(function_type&& func)
        : m_func(std::move(func))
        {
        }

        void add(value_type&& value)
        {
            m_func(std::move(value));
        }

    private:
        function_type m_func;
    };

    //  Implementations for make_accumulator_adapter
    namespace detail
    {
        //  Default matcher
        template <typename T, typename Enable = void>
        struct make_accumulator_adapter_impl
        {
            static void exec(T ar)
            {
                //  An error here means you're trying to initialize an accumulator
                //  from a type that doesn't match any of the available adapters.
            }
        };

        //  Matches types with a push_back() method
        template <typename T>
        struct make_accumulator_adapter_impl<T, typename boost::enable_if<has_push_back<T>>::type>
        {
            static push_back_accumulator_adapter<T> exec(T& collection)
            {
                return push_back_accumulator_adapter<T>(collection);
            }
        };

        //  Matches arrays
        template <typename T>
        struct make_accumulator_adapter_impl<T, typename boost::enable_if<boost::is_array<T>>::type>
        {
            typedef typename boost::remove_extent<T>::type value_type;
  
            template <typename U, size_t N>
            static iterator_accumulator_adapter<value_type*> exec(U (&ar)[N])
            {
                return iterator_accumulator_adapter<value_type*>(std::begin(ar), std::end(ar));
            }
        };

        //  Matches types with iterators, but no push_back (i.e. std::array)
        template <typename T>
        struct make_accumulator_adapter_impl<T, typename boost::enable_if<boost::mpl::and_<has_iterator<T>, boost::mpl::not_<has_push_back<T>>>>::type>
        {
            typedef typename T::iterator iterator_type;
            typedef typename std::iterator_traits<iterator_type>::value_type value_type;

            static iterator_accumulator_adapter<iterator_type> exec(T& collection)
            {
                return iterator_accumulator_adapter<iterator_type>(std::begin(collection), std::end(collection));
            }
        };

        //  Matches builtin callable types
        //  Non-builtin callables can't be matched because there's no way of knowing what the parameter type
        //  is (since there might be multiple overloads of operator() accepting different parameters.)
        //  Therefore, to encapsulate a non-builtin callable such as a lambda, use:
        //
        //  accumulator<T> = make_accumulator<T>(lamba);
        //
        template <typename T>
        struct make_accumulator_adapter_impl<T, typename boost::enable_if<boost::function_types::is_callable_builtin<T>>::type>
        {
            typedef typename boost::mpl::front<boost::function_types::parameter_types<T>>::type parameter_type;
            typedef typename boost::remove_reference<parameter_type>::type value_type;

            static functional_accumulator_adapter<T, value_type> exec(const T& func)
            {
                return functional_accumulator_adapter<T, value_type>(func);
            }

            static functional_accumulator_adapter<T, value_type> exec(T&& func)
            {
                return functional_accumulator_adapter<T, value_type>(std::move(func));
            }
        };
    };

    template <typename T>
    inline auto make_accumulator_adapter(T&& param) -> decltype(detail::make_accumulator_adapter_impl<typename boost::remove_reference<T>::type>::exec(std::forward<T>(param)))
    {
        return detail::make_accumulator_adapter_impl<typename boost::remove_reference<T>::type>::exec(std::forward<T>(param));
    }

    template <typename T>
    inline auto make_accumulator(T&& param) -> accumulator<decltype(detail::get_accumulator_value_type(make_accumulator_adapter(std::forward<T>(param))))>
    {
        typedef decltype(detail::get_accumulator_value_type(make_accumulator_adapter(std::forward<T>(param)))) value_type;
        return accumulator<value_type>(make_accumulator_adapter_proxy<value_type>(std::forward<T>(param)));
    }

    template <typename T, typename F>
    inline accumulator<T> make_accumulator(F&& func, decltype(func(*(T*)0))* dummy = nullptr)
    {
        //  Dummy parameter ensures this is specialized only for callables
        return accumulator<T>(accumulator_adapter_proxy<T, functional_accumulator_adapter<F, T>>(functional_accumulator_adapter<F, T>(std::forward<F>(func))));
    }

    template <typename T>
    inline accumulator<typename std::iterator_traits<T>::value_type> make_accumulator(const T& begin, const T& end)
    {
        typedef typename std::iterator_traits<T>::value_type value_type;
        return accumulator<value_type>(accumulator_adapter_proxy<value_type, iterator_accumulator_adapter<T>>(iterator_accumulator_adapter<T>(begin, end)));
    }

    template <typename T>
    inline accumulator<T> make_accumulator(T* ptr, size_t count)
    {
        return accumulator<T>(accumulator_adapter_proxy<T, iterator_accumulator_adapter<T*>>(iterator_accumulator_adapter<T*>(ptr, ptr + count)));
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP

