////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
//  See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_ACCUMULATOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_ACCUMULATOR_HPP

#include "common.hpp"

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
                m_adapter.add(std::move(value));
            }

            virtual accumulator_adapter_interface<T>* move(void* ptr)
            {
                return new(ptr) this_type(std::move(m_adapter));
            }

        private:
            adapter_type m_adapter;
        };

        //  FIXME: Visual C++ has issues with decltype resolving to int if template
        //  substitution fails instead of giving an error message; typically this
        //  results in errors much deeper in the template code, which are extremely
        //  unclear. This workaround, while hacky, makes error messages MUCH cleaner.
        //  See the commented out code below for the original version.
        template <typename T, typename U>
        struct get_accumulator_adapter_type
        {
            typedef decltype(make_accumulator_adapter<T>(declval<U>())) type;
        };

        template <typename T, typename U>
        inline auto make_accumulator_adapter_proxy(U&& param) 
            -> accumulator_adapter_proxy<T, typename get_accumulator_adapter_type<T, U>::type>
        {
            return accumulator_adapter_proxy<T, typename get_accumulator_adapter_type<T, U>::type>
                (make_accumulator_adapter<T>(std::forward<U>(param)));
        }

        /*
        template <typename T, typename U>
        inline auto make_accumulator_adapter_proxy(U&& param) -> accumulator_adapter_proxy<T, decltype(make_accumulator_adapter<T>(std::forward<U>(param)))>
        {
            return accumulator_adapter_proxy<T, decltype(make_accumulator_adapter<T>(std::forward<U>(param)))>(make_accumulator_adapter<T>(std::forward<U>(param)));
        }
        */

        //
        //  Accumulator adapter encapsulating a collection implementing a push_back method.
        //
        //  Parameters:
        //      [template] C
        //          Collection type.
        //
        template <typename C>
        class push_back_accumulator_adapter
        {
        public:
            typedef C collection_type;
            typedef typename C::value_type value_type;
            typedef push_back_accumulator_adapter<C> this_type;

            push_back_accumulator_adapter(collection_type& collection)
            : m_collection(collection)
            {
            }

            void add(value_type&& value)
            {
                m_collection.push_back(std::move(value));
            }

        private:
            collection_type& m_collection;
        };

        template <typename T, typename C>
        struct supports_push_back_accumulator_adapter
        {
            static const bool value = has_push_back<C, T>::value;
        };

        template <typename T, typename C>
        inline auto make_accumulator_adapter(C&& collection)
            -> typename boost::enable_if<supports_push_back_accumulator_adapter<T, typename boost::remove_reference<C>::type>,
                                         push_back_accumulator_adapter<typename boost::remove_reference<C>::type>>::type
        {
            return push_back_accumulator_adapter<typename boost::remove_reference<C>::type>(std::forward<C>(collection));
        }

        //
        //  Accumulator adapter encapsulating two output iterators representing a range.
        //
        template <typename I>
        class iterator_accumulator_adapter
        {
        public:
            typedef I iterator_type;
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

        template <typename T, typename C>
        struct supports_iterator_accumulator_adapter
        {
            //  If it supports both iterators and push_back, prefer push_back because
            //  that will allow the container to grow.
            static const bool value = has_iterator<C>::value &&
                                      !supports_push_back_accumulator_adapter<T, C>::value;
        };

        template <typename T, typename C>
        inline auto make_accumulator_adapter(C& collection)
            -> typename boost::enable_if<supports_iterator_accumulator_adapter<T, C>,
                                         iterator_accumulator_adapter<typename C::iterator>>::type
        {
            return iterator_accumulator_adapter<typename C::iterator>(collection.begin(), collection.end());
        }

        template <typename T, typename C>
        inline auto make_accumulator_adapter(const C& collection)
            -> typename boost::enable_if<supports_iterator_accumulator_adapter<T, C>,
                                         iterator_accumulator_adapter<typename C::iterator>>::type
        {
            return iterator_accumulator_adapter<typename C::const_iterator>(collection.cbegin(), collection.cend());
        }

        template <typename T, size_t N>
        inline auto make_accumulator_adapter(T (&ar)[N])
            -> iterator_accumulator_adapter<T*>
        {
            return iterator_accumulator_adapter<T*>(std::begin(ar), std::end(ar));
        }

        //
        //  Accumulator adapter encapsulating a functor.
        //
        template <typename T, typename F>
        class functional_accumulator_adapter
        {
        public:
            typedef T value_type;
            typedef F function_type;

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

        template <typename T, typename F>
        struct supports_functional_accumulator_adapter
        {
            static const bool value = is_callable_1<F, T>::value;
        };

        template <typename T, typename F>
        inline auto make_accumulator_adapter(F&& func)
            -> typename boost::enable_if<supports_functional_accumulator_adapter<T, typename boost::remove_reference<F>::type>,
                                         functional_accumulator_adapter<T, typename boost::remove_reference<F>::type>>::type
        {
            return functional_accumulator_adapter<T, typename boost::remove_reference<F>::type>(std::forward<F>(func));
        }
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_ACCUMULATOR_HPP
