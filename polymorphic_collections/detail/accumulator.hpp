////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
//  See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_ACCUMULATOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_ACCUMULATOR_HPP

namespace polymorphic_collections
{
    namespace detail
    {
        //
        //  Interface used by the accumulator to manipulate the underlying adapter.
        //
        //  Parameters:
        //      [template] T
        //          Value type.
        //
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

        //
        //  Proxy class which holds the actual adapter.
        //
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

        template <typename T, typename U>
        inline auto make_accumulator_adapter_proxy(U&& param) -> accumulator_adapter_proxy<T, decltype(make_accumulator_adapter(std::forward<U>(param)))>
        {
            return accumulator_adapter_proxy<T, decltype(make_accumulator_adapter(std::forward<U>(param)))>(make_accumulator_adapter(std::forward<U>(param)));
        }

        //
        //  Workaround for a limitation in Visual C++ to do
        //      declspec(foo())::value_type
        //
        template <typename T>
        inline auto get_accumulator_value_type(const T& adapter) -> typename T::value_type
        {
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

        template <typename T>
        inline auto make_accumulator_adapter(T&& param) -> decltype(make_accumulator_adapter_impl<typename boost::remove_reference<T>::type>::exec(std::forward<T>(param)))
        {
            return make_accumulator_adapter_impl<typename boost::remove_reference<T>::type>::exec(std::forward<T>(param));
        }
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_ACCUMULATOR_HPP
