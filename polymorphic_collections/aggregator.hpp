//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_AGGREGATOR_HPP
#define POLYMORPHIC_COLLECTIONS_AGGREGATOR_HPP

#include "policy.hpp"
#include "detail/aggregator.hpp"

namespace polymorphic_collections
{
    //
    //  Polymorphic interface for aggregating key-value pairs into a collection.
    //
    //  Parameters:
    //      [template] K
    //          Key type.
    //      [template] T
    //          Value type.
    //      [template] P1, ...
    //          Policies (see policy.hpp)
    //
    template <typename K, typename T, typename P1 = no_lock>
    class aggregator : public P1,
                       public boost::noncopyable
    {
    public:
        typedef K key_type;
        typedef T value_type;
        typedef aggregator<K, T, P1> this_type;
        typedef P1 lock_policy;

        //  Size, in bytes, of the internal storage buffer used to store the
        //  type-erased adapter object, thereby avoiding a heap allocation.
        static const size_t internal_storage_size = 32 - sizeof(ptrdiff_t);

        aggregator()
        : m_adapter(nullptr)
        {
        }

        template <typename A>
        aggregator(detail::aggregator_adapter_proxy<K, T, A>&& adapter)
        {
            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::aggregator_adapter_proxy<K, T, A>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::aggregator_adapter_proxy<K, T, A>(std::move(adapter));
            }
        }

        template <typename P1_>
        aggregator(aggregator<K, T, P1_>&& rhs)
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
        aggregator(U&& param)
        : m_adapter(nullptr)
        {
            *this = detail::make_aggregator_adapter_proxy<K, T>(std::forward<U>(param));
        }

        ~aggregator()
        {
            dispose();
        }

        template <typename P1_>
        this_type& operator=(aggregator<K, T, P1_>&& rhs)
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

        template <typename A>
        this_type& operator=(detail::aggregator_adapter_proxy<K, T, A>&& adapter)
        {
            dispose();

            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) detail::aggregator_adapter_proxy<K, T, A>(std::move(adapter));
            }
            else
            {
                m_adapter = new detail::aggregator_adapter_proxy<K, T, A>(std::move(adapter));
            }

            return *this;
        }

        template <typename U>
        this_type& operator=(U&& param)
        {
            *this = detail::make_aggregator_adapter_proxy<K, T>(std::forward<U>(param));
            return *this;
        }
        
        aggregator<K, T>& add(const K& key, const T& value)
        {
            if (lock_policy::lock())
            {
                try
                {
                    if (!m_adapter)
                    {
                        throw std::overflow_error("aggregator::add()");
                    }
                    key_type new_key = key;
                    value_type new_value = value;
                    m_adapter->add(std::move(new_key), std::move(new_value));
                }
                catch (...)
                {
                    lock_policy::unlock();
                    throw;
                }
                lock_policy::unlock();                
            }
            return *this;
        }

        aggregator<K, T>& add(const K&  key, T&& value)
        {
            if (lock_policy::lock())
            {
                try
                {
                    if (!m_adapter)
                    {
                        throw std::overflow_error("aggregator::add()");
                    }
                    key_type new_key = key;
                    m_adapter->add(std::move(new_key), std::move(value));
                }
                catch (...)
                {
                    lock_policy::unlock();
                    throw;
                }
                lock_policy::unlock();                
            }
            return *this;
        }

        aggregator<K, T>& add(K&& key, const T& value)
        {
            if (lock_policy::lock())
            {
                try
                {
                    if (!m_adapter)
                    {
                        throw std::overflow_error("aggregator::add()");
                    }
                    value_type new_value = value;
                    m_adapter->add(std::move(key), std::move(new_value));
                }
                catch (...)
                {
                    lock_policy::unlock();
                    throw;
                }
                lock_policy::unlock();                
            }
            return *this;
        }

        aggregator<K, T>& add(K&& key, T&& value)
        {
            if (lock_policy::lock())
            {
                try
                {
                    if (!m_adapter)
                    {
                        throw std::overflow_error("aggregator::add()");
                    }
                    m_adapter->add(std::move(key), std::move(value));
                }
                catch (...)
                {
                    lock_policy::unlock();
                    throw;
                }
                lock_policy::unlock(); 
            }
            return *this;
        }

    private:
        void dispose()
        {
            if (m_adapter)
            {
                if (reinterpret_cast<char*>(m_adapter) == m_storage)
                {
                    m_adapter->~aggregator_adapter_interface();
                }
                else
                {
                    delete m_adapter;
                }
                m_adapter = nullptr;
            }
        }
        detail::aggregator_adapter_interface<K, T>* m_adapter;
        char m_storage[internal_storage_size];
    };
}

#endif  // POLYMORPHIC_COLLECTIONS_AGGREGATOR_HPP
