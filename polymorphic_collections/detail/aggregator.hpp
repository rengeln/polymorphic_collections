//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_AGGREGATOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_AGGREGATOR_HPP

#include "common.hpp"

namespace polymorphic_collections
{
    namespace detail
    {
        //
        //  Interface used by the aggregator to manipulate the underlying adapter.
        //
        //  Parameters:
        //      [template] K
        //          Key type.
        //      [tempalte] T
        //          Value type.
        //
        template <typename K, typename T>
        class aggregator_adapter_interface
        {
        public:
            typedef K key_type;
            typedef T value_type;
            typedef aggregator_adapter_interface<K, T> this_type;

            virtual ~aggregator_adapter_interface() = 0 { }
            virtual void add(key_type&&, value_type&&) = 0;
            virtual this_type* move(void* ptr) = 0;
        };

        //
        //  Proxy class which holds the actual adapter.
        //
        template <typename K, typename T, typename A>
        class aggregator_adapter_proxy : public aggregator_adapter_interface<K, T>,
                                         public boost::noncopyable
        {
        public:
            typedef K key_type;
            typedef T value_type;
            typedef A adapter_type;
            typedef aggregator_adapter_proxy<K, T, A> this_type;

            aggregator_adapter_proxy(adapter_type&& adapter)
            : m_adapter(std::move(adapter))
            {
            }

            aggregator_adapter_proxy(this_type&& rhs)
            : m_adapter(std::move(rhs.m_adapter))
            {
            }

            virtual ~aggregator_adapter_proxy()
            {
            }

            virtual void add(key_type&& key, value_type&& value)
            {
                m_adapter.add(std::move(key), std::move(value));
            }

            virtual aggregator_adapter_interface<K, T>* move(void* ptr)
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
        template <typename K, typename T, typename U>
        struct get_aggregator_adapter_type
        {
            typedef decltype(make_aggregator_adapter<K, T>(declval<U>())) type;
        };

        template <typename K, typename T, typename U>
        inline auto make_aggregator_adapter_proxy(U&& param) 
            -> aggregator_adapter_proxy<K, T, typename get_aggregator_adapter_type<K, T, U>::type>
        {
            return aggregator_adapter_proxy<K, T, typename get_aggregator_adapter_type<K, T, U>::type>
                (make_aggregator_adapter<K, T>(std::forward<U>(param)));
        }

        /*
        template <typename K, typename T, typename U>
        inline auto make_aggregator_adapter_proxy(U&& param)
            -> aggregator_adapter_proxy<K, T, decltype(make_aggregator_adapter<K, T>(std::forward<U>(param)))>
        {
            return aggregator_adapter_proxy<K, T, decltype(make_aggregator_adapter<K, T>(std::forward<U>(param)))>(
                make_aggregator_adapter<K, T>(std::forward<U>(param)));
        }
        */

        //
        //  Aggregator adapter encapsulating a collection implementing an insert() method.
        //
        //  Parameters:
        //      [template] T
        //          Collection type.
        //
        template <typename T>
        class insert_aggregator_adapter
        {
        public:
            typedef T collection_type;
            typedef typename T::key_type key_type;
            typedef typename T::mapped_type value_type;
            typedef typename T::value_type pair_type;
            typedef insert_aggregator_adapter<T> this_type;

            insert_aggregator_adapter(T& collection)
            : m_collection(collection)
            {
            }

            void add(key_type&& key, value_type&& value)
            {
                m_collection.insert(pair_type(std::move(key), std::move(value)));
            }

        private:
            collection_type& m_collection;
        };

        template <typename K, typename T, typename C>
        struct supports_insert_aggregator_adapter
        {
            static const bool value = has_insert<C, std::pair<K, T>>::value &&
                                      has_key_type<C>::value &&
                                      has_mapped_type<C>::value &&
                                      has_value_type<C>::value;
        };

        template <typename K, typename T, typename C>
        inline auto make_aggregator_adapter(C&& collection)
            -> typename boost::enable_if<supports_insert_aggregator_adapter<K, T, typename boost::remove_reference<C>::type>,
                                         insert_aggregator_adapter<typename boost::remove_reference<C>::type>>::type
        {
            return insert_aggregator_adapter<typename boost::remove_reference<C>::type>(std::forward<C>(collection));
        }

        //
        //  Aggregator adapter encapsulating a functor.
        //
        //  Parameters:
        //      [template] K
        //          Key type.
        //      [template] T
        //          Value type.
        //      [template] F
        //          Function type.
        //
        template <typename K, typename T, typename F>
        class functional_aggregator_adapter
        {
        public:
            typedef K key_type;
            typedef T value_type;
            typedef F function_type;
            typedef functional_aggregator_adapter<K, T, F> this_type;

            functional_aggregator_adapter(const function_type& func)
            : m_func(func)
            {
            }

            functional_aggregator_adapter(function_type&& func)
            : m_func(std::move(func))
            {
            }

            functional_aggregator_adapter(this_type&& rhs)
            : m_func(std::move(rhs.m_func))
            {
            }

            void add(key_type&& key, value_type&& value)
            {
                m_func(std::move(key), std::move(value));
            }

        private:
            function_type m_func;
        };

        template <typename K, typename T, typename F>
        struct supports_functional_aggregator_adapter
        {
            static const bool value = is_callable_2<F, K, T>::value;
        };

        template <typename K, typename T, typename F>
        inline auto make_aggregator_adapter(F&& func)
            -> typename boost::enable_if<supports_functional_aggregator_adapter<K, T, typename boost::remove_reference<F>::type>,
                                         functional_aggregator_adapter<K, T, typename boost::remove_reference<F>::type>>::type
        {
            return functional_aggregator_adapter<K, T, typename boost::remove_reference<F>::type>(std::forward<F>(func));
        }
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_AGGREGATOR_HPP
