//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_ACCESSOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_ACCESSOR_HPP

#include <boost/optional.hpp>

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

            virtual ~accessor_adapter_interface() = 0 { }
            virtual this_type* move(void* ptr) = 0;
            virtual boost::optional<T&> get(const K& key) = 0;
        };

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
                auto& value = m_adapter.get(key);
                if (value)
                {
                    value_type& ref = value->second;
                    return boost::optional<value_type&>(ref);
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

        template <typename K, typename T, typename U>
        inline auto make_accessor_adapter_proxy(U&& param) -> accessor_adapter_proxy<K, T, decltype(make_accessor_adapter(std::forward<U>(param)))>
        {
            return accessor_adapter_proxy<K, T, decltype(make_accessor_adapter(std::forward<U>(param)))>(make_accessor_adapter(std::forward<U>(param)));
        }

        template <typename T>
        inline auto get_accessor_key_type(const T&) -> typename T::key_type
        {
        }

        template <typename T>
        inline auto get_accessor_value_type(const T&) -> typename T::value_type
        {
        }

        //
        //  Accessor adapter for types which support a find() method.
        //
        template <typename T>
        class find_accessor_adapter : public boost::noncopyable
        {
        public:
            typedef T collection_type;
            typedef typename T::iterator iterator_type;
            typedef typename T::value_type value_type;
            typedef typename T::key_type key_type;
            typedef find_accessor_adapter<T> this_type;

            find_accessor_adapter(collection_type& collection)
            : m_collection(collection)
            {
            }

            find_accessor_adapter(this_type&& rhs)
            : m_collection(std::move(rhs.m_collection))
            {
            }

            boost::optional<value_type&> get(const key_type& key)
            {
                auto it = m_collection.find(key);
                if (it != m_collection.end())
                {
                    return *it;
                }
                return boost::none;
            }

        private:
            collection_type& m_collection;
        };

        template <typename T>
        struct supports_find_accessor_adapter
        {
            static const bool value = has_find<T>::value &&
                                      has_iterator<T>::value;
        };

        template <typename T>
        inline typename boost::enable_if<supports_find_accessor_adapter<T>, find_accessor_adapter<T>>::type 
            make_accessor_adapter(T& collection)
        {
            return find_accessor_adapter<T>(collection);
        }

        template <typename T>
        inline typename boost::enable_if<supports_find_accessor_adapter<T>, find_accessor_adapter<T>>::type 
            make_accessor_adapter(const T& collection)
        {
            return find_accessor_adapter<const T>(collection);
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
            typedef typename T::value_type value_type;
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
                    return *it;
                }
                return boost::none;
            }

        private:
            std::unique_ptr<collection_type> m_collection;
        };

        template <typename T>
        struct supports_embedded_find_accessor_adapter
        {
            static const bool value = has_find<T>::value &&
                                      has_iterator<T>::value;
        };
        
        template <typename T>
        inline typename boost::enable_if<supports_embedded_find_accessor_adapter<T>, embedded_find_accessor_adapter<T>>::type 
            make_accessor_adapter(T&& collection, typename T::iterator* dummy = nullptr)
        {
            // FIXME: won't compile without the dummy parameter, ambiguous vs the T&/const T&
            // find_accessor_adapter implementations. Somehow, enumerator doesn't have
            // this issue - must figure out why.
            return embedded_find_accessor_adapter<T>(std::move(collection));
        }
        
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_ACCESSOR_HPP
