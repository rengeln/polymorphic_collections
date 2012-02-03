//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_ACCESSOR_HPP
#define POLYMORPHIC_COLLECTIONS_ACCESSOR_HPP

#include "policy.hpp"
#include "detail/accessor.hpp"

namespace polymorphic_collections
{
    //
    //  Polymorphic interface for accessing a key-value data store.
    //
    //  Parameters:
    //      [template] K
    //          Key type.
    //      [template] T
    //          Value type. The accessor will expose references to the objects
    //          in the collection, so this should be const if the values
    //          are to be immutable.
    //      [template] P1, ...
    //          Policies.
    //
    template <typename K, typename T, typename P1 = no_lock>
    class accessor : public P1,
                     public boost::noncopyable
    {
    public:
        typedef K key_type;
        typedef T value_type;
        typedef accessor<K, T, P1> this_type;
        typedef P1 lock_policy;

        //  Size, in bytes, of the internal storage buffer used to store the
        //  type-erased adapter object, thereby avoiding a heap allocation.
        static const size_t internal_storage_size = 32 - sizeof(ptrdiff_t);

        accessor()
        : m_adapter(nullptr)
        {
        }

        template <typename A>
        accessor(detail::accessor_adapter_proxy<K, T, A>&& adapter)
        {
            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::accessor_adapter_proxy<K, T, A>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::accessor_adapter_proxy<K, T, A>(std::move(adapter));
            }
        };

        template <typename P1_>
        accessor(accessor<K, T, P1_>&& rhs)
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
        accessor(U&& param)
        : m_adapter(nullptr)
        {
            *this = detail::make_accessor_adapter_proxy<K, T>(std::forward<U>(param));
        }

        ~accessor()
        {
            dispose();
        }

        template <typename P1_>
        this_type& operator=(accessor<K, T, P1_>&& rhs)
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
            return *this;
        }

        template <typename A>
        this_type& operator=(detail::accessor_adapter_proxy<K, T, A>&& adapter)
        {
            dispose();

            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::accessor_adapter_proxy<K, T, A>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::accessor_adapter_proxy<K, T, A>(std::move(adapter));
            }

            return *this;
        }
        
        template <typename U>
        this_type& operator=(U&& param)
        {
            *this = detail::make_accessor_adapter_proxy<K, T>(std::forward<U>(param));
            return *this;
        }

        boost::optional<T&> get(const K& key)
        {
            if (!m_adapter)
            {
                return boost::none;
            }
            else
            {
                if (lock_policy::lock())
                {
                    try
                    {
                        boost::optional<T&> value = m_adapter->get(key);
                        lock_policy::unlock();
                        return value;
                    }
                    catch (...)
                    {
                        lock_policy::unlock();
                        throw;
                    }
                }
                else
                {
                    return boost::none;
                }
            }
        }

        boost::optional<T&> operator[](const K& key)
        {
            return get(key);
        }

    private:
        template <typename K_, typename T_, typename P1_>
        friend class accessor;

        void dispose()
        {
            if (m_adapter)
            {
                if (reinterpret_cast<char*>(m_adapter) == m_storage)
                {
                    m_adapter->~accessor_adapter_interface();
                }
                else
                {
                    delete m_adapter;
                }
                m_adapter = nullptr;
            }
        }
        detail::accessor_adapter_interface<K, T>* m_adapter;
        char m_storage[internal_storage_size];
    };

    //
    //  Makes an implicitly-typed accessor out of the source object.
    //
    /*
    template <typename T>
    inline auto make_accessor(T&& param) -> accessor<decltype(detail::get_accessor_key_type(detail::make_accessor_adapter(std::forward<T>(param)))),
                                                     decltype(detail::get_accessor_value_type(detail::make_accessor_adapter(std::forward<T>(param))))>
    {
        typedef decltype(detail::get_accessor_key_type(detail::make_accessor_adapter(std::forward<T>(param)))) key_type;
        typedef decltype(detail::get_accessor_value_type(detail::make_accessor_adapter(std::forward<T>(param)))) value_type;
        return accessor<key_type, value_type>(detail::make_accessor_adapter_proxy<key_type, value_type>(std::forward<T>(param)));
    }
    */
}

#endif  // POLYMORPHIC_COLLECTIONS_ACCESSOR_HPP
