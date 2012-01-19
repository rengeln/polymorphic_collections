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

namespace polymorphic_collections
{
    static const size_t enumerator_internal_storage_size = 32;

    template <typename T, typename Enable = void>
    class enumerator_adapter_interface
    {
    };
    
    template <typename T>
    class enumerator_adapter_interface<T, typename boost::disable_if<boost::is_const<T>>::type> : public enumerator_adapter_interface<const T>
    {
    public:
        typedef enumerator_adapter_interface<T> this_type;
        typedef T value_type;

        virtual ~enumerator_adapter_interface() = 0 
        {
        };

        virtual value_type& _next(value_type*) = 0;
        virtual this_type* _move(void* ptr, T*) = 0;
    };

    template <typename T>
    class enumerator_adapter_interface<T, typename boost::enable_if<boost::is_const<T>>::type> : public boost::noncopyable
    {
    public:
        typedef enumerator_adapter_interface<T> this_type;
        typedef T value_type;
     
        virtual ~enumerator_adapter_interface() = 0 
        {
        };

        virtual bool _is_valid() const = 0;
        virtual value_type& _next(T*) = 0;
        virtual this_type* _move(void* ptr, T*) = 0;
    };

    template <typename T, typename V, typename Enabler = void>
    class enumerator_adapter 
    {
    };

    template <typename T, typename V>
    class enumerator_adapter<T, V, typename boost::disable_if<boost::is_const<V>>::type> : public enumerator_adapter_interface<typename V>
    {
    public:
        typedef enumerator_adapter<T, V> this_type;
        typedef V value_type;
        typedef const typename boost::remove_const<value_type>::type const_value_type;

        virtual ~enumerator_adapter()
        {
        }

        virtual bool _is_valid() const
        {
            return static_cast<const T*>(this)->is_valid();
        }

        virtual enumerator_adapter_interface<value_type>* _move(void* ptr, value_type*)
        {
            return static_cast<T*>(this)->move(ptr);
        }

        virtual enumerator_adapter_interface<const_value_type>* _move(void* ptr, const_value_type*)
        {
            return static_cast<T*>(this)->move(ptr);
        }

        virtual value_type& _next(value_type*)
        {
            return static_cast<T*>(this)->next();
        }

        virtual const_value_type& _next(const_value_type*)
        {
            return static_cast<T*>(this)->next();
        }
    };

    template <typename T, typename V>
    class enumerator_adapter<T, V, typename boost::enable_if<boost::is_const<T>>::type> : public enumerator_adapter_interface<typename T::value_type>
    {
    public:
        typedef enumerator_adapter<T, V> this_type;
        typedef typename T::value_type value_type;
        typedef typename T::value_type const_value_type;

        virtual ~enumerator_adapter()
        {
        }

        virtual bool _is_valid() const
        {
            return static_cast<const T*>(this)->is_valid();
        }

        virtual enumerator_adapter_interface<const_value_type>* _move(void* ptr, const_value_type*)
        {
            return static_cast<T*>(this)->move(ptr);
        }

        virtual const_value_type& _next(const_value_type*)
        {
            return static_cast<T*>(this)->next();
        }
    };

    template <typename T>
    class iterator_enumerator_adapter : public enumerator_adapter<iterator_enumerator_adapter<T>, typename std::iterator_traits<T>::value_type>
    {
    public:
        typedef iterator_enumerator_adapter<T> this_type;
        typedef T iterator_type;
        typedef typename std::iterator_traits<T>::value_type value_type;

        iterator_enumerator_adapter(const iterator_type& begin, const iterator_type& end)
        : m_begin(begin), m_end(end)
        {
        }

        iterator_enumerator_adapter(this_type&& rhs)
        : m_begin(std::move(rhs.m_begin)), m_end(std::move(rhs.m_end))
        {
        }

        iterator_enumerator_adapter<T>& operator=(this_type&& rhs)
        {
            m_begin = std::move(rhs.m_begin);
            m_end = std::move(rhs.m_end);
            return *this;
        }

        virtual ~iterator_enumerator_adapter()
        {
        }

        bool is_valid() const
        {
            return m_begin != m_end;
        }

        value_type& next()
        {
            if (!is_valid())
            {
                throw std::out_of_range("iterator_enumerator_adapter::next()");
            }
            return *m_begin++;
        }

        this_type* move(void* ptr)
        {
            return new (ptr) this_type(std::move(*this));
        }

    private:
        iterator_type m_begin, m_end;
    };

     template <typename T, typename F>
    class functional_enumerator_adapter : public enumerator_adapter<functional_enumerator_adapter<T, F>, typename boost::remove_reference<T>::type>
    {
    public:
        typedef functional_enumerator_adapter<T, F> this_type;
        typedef F function_type;
        typedef T return_type;
        typedef typename boost::remove_reference<T>::type value_type;

        functional_enumerator_adapter(const function_type& func)
        : m_func(func), m_hungry(true)
        {
        }

        functional_enumerator_adapter(function_type&& func)
        : m_func(std::move(func)), m_hungry(true)
        {
        }

        functional_enumerator_adapter(this_type&& rhs)
        : m_func(std::move(rhs.m_func)), m_next(std::move(rhs.m_next)), m_hungry(std::move(rhs.m_hungry))
        {
        }    

        this_type& operator=(this_type&& rhs)
        {
            m_func = std::move(rhs.m_func);
            m_hungry = std::move(rhs.m_hungry);
            m_next = std::move(rhs.m_next);
            return *this;
        }

        virtual ~functional_enumerator_adapter()
        {
        }

        bool is_valid() const
        {
            if (m_hungry)
            {
                m_next = std::move(m_func());
                m_hungry = false;
            }
            return m_next;
        }

        T& next()
        {
            if (!is_valid())
            {
                throw std::out_of_range("functional_enumerator_adapter::next()");
            }
            else
            {
                m_hungry = true;
                return m_next.get();
            }
        }

        this_type* move(void* ptr)
        {
            return new (ptr) this_type(std::move(*this));
        }

    private:
        mutable bool m_hungry;
        mutable boost::optional<T> m_next;
        function_type m_func;
    };

    template <typename T>
    class embedded_enumerator_adapter : public enumerator_adapter<embedded_enumerator_adapter<T>, typename std::iterator_traits<typename T::iterator>::value_type>
    {
    public:
        typedef embedded_enumerator_adapter<T> this_type;
        typedef T collection_type;
        typedef typename T::iterator iterator_type;
        typedef typename std::iterator_traits<iterator_type>::value_type value_type;

        explicit embedded_enumerator_adapter(collection_type&& collection)
        : m_collection(new collection_type(std::move(collection)))
        {
            m_begin = m_collection->begin();
            m_end = m_collection->end();
        }

        explicit embedded_enumerator_adapter(this_type&& rhs)
        {
            *this = std::move(rhs);
        }

        this_type& operator=(this_type&& rhs)
        {
            m_collection = std::move(rhs.m_collection);
            m_begin = std::move(rhs.m_begin);
            m_end = std::move(rhs.m_end);
            return *this;
        }

        virtual ~embedded_enumerator_adapter()
        {
        }

        bool is_valid() const
        {
            return m_begin != m_end;
        }

        value_type& next()
        {
            return *m_begin++;
        }
        
        this_type* move(void* ptr)
        {
            return new (ptr) this_type(std::move(*this));
        }

    private:
        std::unique_ptr<collection_type> m_collection;
        iterator_type m_begin, m_end;
    };

    template <typename T, typename Enabler = void>
    class enumerator_adapter_proxy
    {
    };

    template <typename T>
    class enumerator_adapter_proxy<T, typename boost::disable_if<boost::is_const<T>>::type> : public boost::noncopyable
    {
    public:
        typedef enumerator_adapter_proxy<T> this_type;
        typedef T value_type;

        enumerator_adapter_proxy()
        : m_ptr(nullptr)
        {
        }

        explicit enumerator_adapter_proxy(this_type&& rhs)
        {
            *this = std::move(rhs);
        }

        this_type& operator=(this_type&& rhs)
        {
            if (reinterpret_cast<char*>(rhs.m_ptr) == rhs.m_storage)
            {
                m_ptr = rhs.m_ptr->_move(m_storage, static_cast<T*>(nullptr));
                rhs.m_ptr->~enumerator_adapter_interface();
            }
            else
            {
                m_ptr = rhs.m_ptr;
            }
            rhs.m_ptr = nullptr;
            return *this;
        }

        template <typename Adapter> enumerator_adapter_proxy(Adapter&& adapter)
        {
            if (sizeof(Adapter) <= enumerator_internal_storage_size)
            {
                m_ptr = adapter._move(m_storage, static_cast<value_type*>(nullptr));
            }
            else
            {
                m_ptr = new Adapter(std::move(adapter));
            }
        }

        ~enumerator_adapter_proxy()
        {
            if (m_ptr)
            {
                if (reinterpret_cast<char*>(m_ptr) == m_storage)
                {
                    m_ptr->~enumerator_adapter_interface();
                }
                else
                {
                    delete m_ptr;
                }
                m_ptr = 0;
            }
        }

        bool is_valid() const
        {
            return m_ptr && m_ptr->_is_valid();
        }

        T& next()
        {
            if (!m_ptr)
            {
                throw std::out_of_range("Enumerator is empty");
            }
            return m_ptr->_next(static_cast<T*>(nullptr));
        }

    private:
        template <typename T, class Enabler> friend class enumerator_adapter_proxy;
        enumerator_adapter_interface<value_type>* m_ptr;
        union
        {
            char m_storage[enumerator_internal_storage_size];
        };
    };

    template <typename T>
    class enumerator_adapter_proxy<T, typename boost::enable_if<boost::is_const<T>>::type> : public boost::noncopyable
    {
    public:
        typedef enumerator_adapter_proxy<T> this_type;
        typedef T value_type;
        typedef T const_value_type;
        typedef typename boost::remove_const<value_type>::type mutable_value_type;

        enumerator_adapter_proxy()
            : m_ptr(nullptr)
        {
        }

        explicit enumerator_adapter_proxy(enumerator_adapter_proxy<const_value_type>&& rhs)
        {
            *this = std::move(rhs);
        }

        explicit enumerator_adapter_proxy(enumerator_adapter_proxy<mutable_value_type>&& rhs)
        {
            *this = std::move(rhs);
        }

        enumerator_adapter_proxy<T>& operator=(enumerator_adapter_proxy<const_value_type>&& rhs)
        {
            if (reinterpret_cast<char*>(rhs.m_ptr) == rhs.m_storage)
            {
                m_ptr = rhs.m_ptr->move_(m_storage, static_cast<const_value_type*>(nullptr));
                rhs.m_ptr->~enumerator_adapter_interface();
            }
            else
            {
                m_ptr = rhs.m_ptr;
            }
            rhs.m_ptr = nullptr;
            return *this;
        }

        enumerator_adapter_proxy<T>& operator=(enumerator_adapter_proxy<mutable_value_type>&& rhs)
        {
            if (reinterpret_cast<char*>(rhs.m_ptr) == rhs.m_storage)
            {
                m_ptr = rhs.m_ptr->_move(m_storage, static_cast<mutable_value_type*>(nullptr));
                rhs.m_ptr->~enumerator_adapter_interface();
            }
            else
            {
                m_ptr = rhs.m_ptr;
            }
            rhs.m_ptr = nullptr;
            return *this;
        }

        template <typename Adapter> enumerator_adapter_proxy(Adapter&& adapter)
        {
            if (sizeof(Adapter) <= enumerator_internal_storage_size)
            {
                m_ptr = adapter._move(m_storage, static_cast<T*>(nullptr));
            }
            else
            {
                m_ptr = new Adapter(std::move(adapter));
            }
        }

        ~enumerator_adapter_proxy()
        {
            if (m_ptr)
            {
                if (reinterpret_cast<char*>(m_ptr) == m_storage)
                {
                    m_ptr->~enumerator_adapter_interface();
                }
                else
                {
                    delete m_ptr;
                }
                m_ptr = 0;
            }
        }

        bool is_valid() const
        {
            return m_ptr && m_ptr->_is_valid();
        }

        T& next()
        {
            if (!m_ptr)
            {
                throw std::out_of_range("Enumerator is empty");
            }
            return m_ptr->_next(static_cast<T*>(nullptr));
        }

    private:
        template <typename T, class Enabler> friend class enumerator_adapter_proxy;
        enumerator_adapter_interface<value_type>* m_ptr;
        union
        {
            char m_storage[enumerator_internal_storage_size];
        };
    };

    template <typename T, typename Enabler = void>
    class enumerator
    {
    };

    template <typename T>
    class enumerator<T, typename boost::disable_if<boost::is_const<T>>::type> : public boost::noncopyable
    {
    public:
        typedef enumerator<T> this_type;
        typedef T value_type;
        typedef const T const_value_type;
        typedef T mutable_value_type;

        enumerator()
        {
        }

        enumerator(this_type&& rhs)
        : m_adapter(std::move(rhs.m_adapter))
        {
        }

        explicit enumerator(enumerator_adapter_proxy<mutable_value_type>&& adapter)
        : m_adapter(std::move(adapter))
        {
        }

        template <typename A> 
        enumerator(A&& arg)
        {
            *this = make_enumerator(std::forward<A>(arg));
        }

        this_type& operator=(this_type&& rhs)
        {
            m_adapter = std::move(rhs.m_adapter);
            return *this;
        }

        bool is_valid() const
        {
            return m_adapter.is_valid();
        }

        T& next()
        {
            return m_adapter.next();
        }

    private:
        template <typename T, typename Enabler> friend class enumerator;
        enumerator_adapter_proxy<value_type> m_adapter;
    };


    template <typename T>
    class enumerator<T, typename boost::enable_if<boost::is_const<T>>::type> : public boost::noncopyable
    {
    public:
        typedef enumerator<T> this_type;
        typedef T value_type;
        typedef T const_value_type;
        typedef typename boost::remove_const<value_type>::type mutable_value_type;

        enumerator()
        {
        }

        enumerator(enumerator_adapter_proxy<const_value_type>&& adapter)
        : m_adapter(std::move(adapter))
        {
        }

        enumerator(enumerator_adapter_proxy<mutable_value_type>&& adapter)
        : m_adapter(std::move(adapter))
        {
        }

        template <typename A> 
        enumerator(A&& arg)
        {
            *this = make_enumerator(std::forward<A>(arg));
        }

        enumerator(enumerator<const_value_type>&& rhs)
        {
            *this = std::move(rhs);
        }

        enumerator(enumerator<mutable_value_type>&& rhs)
        {
            *this = std::move(rhs);
        }

        enumerator<value_type>& operator=(enumerator<const_value_type>&& rhs)
        {
            m_adapter = std::move(rhs.m_adapter);
            return *this;
        }

        enumerator<value_type>& operator=(enumerator<mutable_value_type>&& rhs)
        {
            m_adapter = std::move(rhs.m_adapter);
            return *this;
        }

        bool is_valid() const
        {
            return m_adapter.is_valid();
        }

        T& next()
        {
            return m_adapter.next();
        }

    private:
        template <typename T, typename Enabler> friend class enumerator;
        enumerator_adapter_proxy<value_type> m_adapter;
    };

    template <typename T>
    inline enumerator<typename iterator_enumerator_adapter<T>::value_type> make_enumerator(const T& begin, const T& end)
    {
        typedef iterator_enumerator_adapter<T> adapter_type;
        typedef typename adapter_type::value_type value_type;
        return enumerator<value_type>(enumerator_adapter_proxy<value_type>(adapter_type(begin, end)));
    }

    template <typename T>
    inline enumerator<typename std::iterator_traits<typename T::iterator>::value_type> make_enumerator(T& collection)
    {
        typedef iterator_enumerator_adapter<typename T::iterator> adapter_type;
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        return enumerator<value_type>(enumerator_adapter_proxy<value_type>(adapter_type(collection.begin(), collection.end())));
    }

    template <typename T>
    inline enumerator<typename std::iterator_traits<typename T::const_iterator>::value_type> make_enumerator(const T& collection)
    {
        typedef iterator_enumerator_adapter<typename T::const_iterator> adapter_type;
        typedef typename std::iterator_traits<typename T::const_iterator>::value_type value_type;
        return enumerator<value_type>(enumerator_adapter_proxy<value_type>(adapter_type(collection.cbegin(), collection.cend())));
    }  

    template <typename T>
    inline enumerator<typename std::iterator_traits<typename T::iterator>::value_type> make_enumerator(T&& collection)
    {
        typedef embedded_enumerator_adapter<T> adapter_type;
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        return enumerator<value_type>(enumerator_adapter_proxy<value_type>(adapter_type(std::move(collection))));
    }

    namespace detail
    {
        //
        //  Workaround for a bug in Visual C++ making it impossible to do this:
        //      decltype(func())::value_type
        //
        template <typename T>
        auto get_optional_internal_type(boost::optional<T>& t) -> T
        {
        }

        template <typename T>
        auto get_optional_internal_type(boost::optional<T&>& t) -> T&
        {
        }

        template <typename T>
        auto get_optional_internal_naked_type(boost::optional<T>& t) -> typename boost::remove_reference<T>::type
        {
        }
    }

    template <typename T>
    inline auto make_enumerator(const T& func) -> enumerator<decltype(detail::get_optional_internal_naked_type(func()))>
    {
        typedef T function_type;
        //typedef typename decltype(detail::get_optional_internal_type(func())) value_type;     // Causes an error in Visual C++, though it works in the function declaration...
        typedef functional_enumerator_adapter<decltype(detail::get_optional_internal_type(func())), function_type> adapter_type;
        typedef adapter_type::value_type value_type;
        return enumerator<value_type>(enumerator_adapter_proxy<value_type>(adapter_type(func)));
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ENUMERATOR_HPP
