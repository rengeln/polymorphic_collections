//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_ACCESSOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_ACCESSOR_HPP

#include "common.hpp"

namespace polymorphic_collections
{
    namespace detail
    {
        //
        //  Interface used by the accessor to manipulate the underlying adapter.
        //
        //  Parameters:
        //      [template] K
        //          Key type.
        //      [template] T
        //          Value type.
        //
        template <typename K, typename T> class accessor_adapter_interface
        {
        public:
            typedef K key_type;
            typedef T value_type;
            typedef accessor_adapter_interface<K, T> this_type;

            virtual ~accessor_adapter_interface() = 0;
            virtual this_type* move(void* ptr) = 0;
            virtual boost::optional<T&> get(const K& key) = 0;
        };
        
        template <typename K, typename T>
        inline accessor_adapter_interface<K, T>::~accessor_adapter_interface()
        {
        }

        //
        //  Proxy class which holds the actual adapter and exposes the interface
        //  used by the parent accessor.
        //
        //  Parameters:
        //      [template] K
        //          Key type.
        //      [template] T
        //          Value type.
        //      [template] A
        //          Adapter type.
        //
        template <typename K, typename T, typename A> class accessor_adapter_proxy : public accessor_adapter_interface<K, T>,
                                                                                     public boost::noncopyable
        {
        public:
            typedef K key_type;
            typedef T value_type;
            typedef A adapter_type;
            typedef accessor_adapter_proxy<K, T, A> this_type;

            accessor_adapter_proxy(this_type&& rhs)
            : m_adapter(std::move(rhs.m_adapter))
            {
            }

            accessor_adapter_proxy(adapter_type&& adapter)
            : m_adapter(std::move(adapter))
            {
            }
            
            virtual ~accessor_adapter_proxy()
            {
            }

            virtual boost::optional<value_type&> get(const key_type& key)
            {
                auto value = m_adapter.get(key);
                if (value)
                {
                    return boost::optional<value_type&>(*value);
                }
                else
                {
                    return boost::none;
                }
            }

            virtual accessor_adapter_interface<key_type, value_type>* move(void* ptr)
            {
                return new(ptr) this_type(std::move(m_adapter));
            }

        private:
            adapter_type m_adapter;
        };

        //
        //  Accessor adapter for types which support a find() method.
        //
        template <typename T>
        class find_accessor_adapter : public boost::noncopyable
        {
        public:
            typedef T collection_type;
            typedef typename T::iterator iterator_type;
            typedef typename T::mapped_type value_type;
            typedef typename T::key_type key_type;
            typedef find_accessor_adapter<T> this_type;

            find_accessor_adapter(collection_type& collection)
            : m_collection(collection)
            {
            }

            find_accessor_adapter(this_type&& rhs)
            : m_collection(rhs.m_collection)
            {
            }

            boost::optional<value_type&> get(const key_type& key)
            {
                auto it = m_collection.find(key);
                if (it != m_collection.end())
                {
                    return it->second;
                }
                return boost::none;
            }

        private:
            collection_type& m_collection;
        };

        template <typename K, typename T, typename C>
        struct supports_find_accessor_adapter
        {
            static const bool value = has_find<C, K>::value;
        };

        template <typename K, typename T, typename C>
        inline auto make_accessor_adapter(C& collection)
            -> typename boost::enable_if<supports_find_accessor_adapter<K, T, C>,
                                         find_accessor_adapter<C>>::type
        {
            return find_accessor_adapter<C>(collection);
        }

        template <typename K, typename T, typename C>
        inline auto make_accessor_adapter(const C& collection)
            -> typename boost::enable_if<supports_find_accessor_adapter<K, T, C>,
                                         find_accessor_adapter<const C>>::type
        {
            return find_accessor_adapter<const C>(collection);
        }

        //
        //  Accessor adapter for embedding collections which support a find() method.
        //
        template <typename T>
        class embedded_find_accessor_adapter : public boost::noncopyable
        {
        public:
            typedef T collection_type;
            typedef typename T::iterator iterator_type;
            typedef typename T::mapped_type value_type;
            typedef typename T::key_type key_type;
            typedef embedded_find_accessor_adapter<T> this_type;

            embedded_find_accessor_adapter(collection_type&& collection)
            : m_collection(new collection_type(std::move(collection)))
            {
            }

            embedded_find_accessor_adapter(this_type&& rhs)
            : m_collection(std::move(rhs.m_collection))
            {
            }

            boost::optional<value_type&> get(const key_type& key)
            {
                auto it = m_collection->find(key);
                if (it != m_collection->end())
                {
                    return it->second;
                }
                return boost::none;
            }

        private:
            std::unique_ptr<collection_type> m_collection;
        };

        template <typename K, typename T, typename C>
        struct supports_embedded_find_accessor_adapter
        {
            static const bool value = has_find<C, K>::value;
        };
        
        template <typename K, typename T, typename C>
        inline auto make_accessor_adapter(C&& collection, typename C::iterator* dummy = nullptr)
            -> typename boost::enable_if<supports_embedded_find_accessor_adapter<K, T, typename boost::remove_reference<C>::type>,
                                         embedded_find_accessor_adapter<typename boost::remove_reference<C>::type>>::type
        {
            // FIXME: the dummy parameter should not be necessary but Visual C++ chokes if it isn't present.
            return embedded_find_accessor_adapter<typename boost::remove_reference<C>::type>(std::forward<C>(collection));
        }

        //
        //  Accessor adapter for a functor.
        //
        template <typename F, typename K>
        class functional_accessor_adapter
        {
        public:
            typedef F function_type;
            typedef K key_type;
            typedef typename is_callable_1<F, K>::return_type::value_type return_type;
            typedef typename boost::remove_reference<return_type>::type value_type;
            typedef functional_accessor_adapter<F, K> this_type;

            functional_accessor_adapter(const function_type& func)
            : m_func(func)
            {
            }
            
            functional_accessor_adapter(function_type&& func)
            : m_func(std::move(func))
            {
            }
            
            functional_accessor_adapter(this_type&& rhs)
            : m_func(std::move(rhs.m_func))
            {
            }
            
            boost::optional<value_type&> get(const key_type& key)
            {
                m_value = m_func(key);
                if (m_value)
                {
                    return *m_value;
                }
                else
                {
                    return boost::none;
                }
            }

        private:
            function_type m_func;
            boost::optional<return_type> m_value;
        };
        
        template <typename K, typename T, typename F>
        struct supports_functional_accessor_adapter
        {
            static const bool value = is_callable_1<F, K>::value;
        };

        template <typename K, typename T, typename F>
        inline auto make_accessor_adapter(F&& func)
            -> typename boost::enable_if<supports_functional_accessor_adapter<K, T, typename boost::remove_reference<F>::type>,
                                         functional_accessor_adapter<typename boost::remove_reference<F>::type, K>>::type
        {
            return functional_accessor_adapter<typename boost::remove_reference<F>::type, K>(std::forward<F>(func));
        }
        
        //  FIXME: Visual C++ has issues with decltype resolving to int if template
        //  substitution fails instead of giving an error message; typically this
        //  results in errors much deeper in the template code, which are extremely
        //  unclear. This workaround, while hacky, makes error messages MUCH cleaner.
        //  See the commented out code below for the original version.
        template <typename K, typename T, typename U>
        struct get_accessor_adapter_type
        {
            typedef decltype(make_accessor_adapter<K, T>(declval<U>())) type;
        };

        template <typename K, typename T, typename U>
        inline auto make_accessor_adapter_proxy(U&& param) 
            -> accessor_adapter_proxy<K, T, typename get_accessor_adapter_type<K, T, U>::type>
        {
            return accessor_adapter_proxy<K, T, typename get_accessor_adapter_type<K, T, U>::type>
                (make_accessor_adapter<K, T>(std::forward<U>(param)));
        }
        
        /*
        template <typename K, typename T, typename U>
        inline auto make_accessor_adapter_proxy(U&& param) -> accessor_adapter_proxy<K, T, decltype(make_accessor_adapter<K, T>(std::forward<U>(param)))>
        {
            return accessor_adapter_proxy<K, T, decltype(make_accessor_adapter<K, T>(std::forward<U>(param)))>(make_accessor_adapter<K, T>(std::forward<U>(param)));
        }
        */        
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_ACCESSOR_HPP
