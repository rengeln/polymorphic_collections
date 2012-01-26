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
    //
    //  Interface used by the enumerator to manipulate the underlying adapter.
    //
    //  Parameters:
    //      [template] T
    //          Value type.
    //
    template <typename T> class enumerator_adapter_interface
    {
    public:
        typedef T value_type;
        typedef enumerator_adapter_interface<T> this_type;

        virtual bool is_valid() const = 0;
        virtual T& next() = 0;
        virtual this_type* move(void* ptr) = 0;
    };

    //
    //  Proxy class which holds the actual adapter and exposes an interface
    //  potentially for a different value type.
    //
    //  This is the mechanism by which you can have the following work:
    //      vector<int> v;
    //      enumerator<const int> e = v;
    //
    //  The underlying adapter is an iterator_enumerator_adapter<int> held
    //  by a enumerator_adapter_proxy<int, const int> which implements the
    //  enumerator_adapter_interface<const int> interface exposed to the
    //  containing enumerator.
    //
    //  Parameters:
    //      [template] T
    //          Value type which will be exposed to the parent enumerator.
    //      [template] A
    //          Adapter type.
    //
    template <typename T, typename A> class enumerator_adapter_proxy : public enumerator_adapter_interface<T>,
                                                                       public boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef A adapter_type;
        typedef enumerator_adapter_proxy<T, A> this_type;

        enumerator_adapter_proxy(this_type&& rhs)
        : m_adapter(std::move(rhs.m_adapter))
        {
        }

        enumerator_adapter_proxy(adapter_type&& adapter)
        : m_adapter(std::move(adapter))
        {
        }

        virtual value_type& next()
        {
            return m_adapter.next();
        }

        virtual bool is_valid() const
        {
            return m_adapter.is_valid();
        }

        virtual enumerator_adapter_interface<value_type>* move(void* ptr)
        {
            return new(ptr) this_type(std::move(m_adapter));
        }

    private:
        A m_adapter;
    };

    //
    //  Polymorphic interface for linear navigation of a collection.
    //
    //  Parameters:
    //      [template] T
    //          Value type held by the collection. As the enumerator will expose
    //          references to the objects in the collection, this should be const
    //          if the objects are immutable.
    //
    template <typename T>
    class enumerator
    {
    public:
        typedef T value_type;
        typedef enumerator<T> this_type;

        //  Size, in bytes, of the internal storage buffer used to store the
        //  type-erased adapter object, thereby avoiding a heap allocation.
        static const size_t internal_storage_size = 32 - sizeof(ptrdiff_t);

        enumerator()
        : m_adapter(nullptr)
        {
            
        }

        template <typename U>
        enumerator(enumerator_adapter_proxy<T, U>&& adapter)
        {
            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) enumerator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new enumerator_adapter_proxy<T, U>(std::move(adapter));
            }
        }

        enumerator(this_type&& rhs)
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
            *this = make_enumerator_adapter_proxy<T>(std::forward<U>(param));
        }

        ~enumerator()
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
        this_type& operator=(enumerator_adapter_proxy<T, U>&& adapter)
        {
            dispose();

            size_t proxy_size = sizeof(adapter);
            if (proxy_size <= internal_storage_size)
            {
                m_adapter = new (m_storage) enumerator_adapter_proxy<T, U>(std::move(adapter));
            }
            else
            {
                m_adapter = new enumerator_adapter_proxy<T, U>(std::move(adapter));
            }

            return *this;
        }

        template <typename U>
        this_type& operator=(U&& param)
        {
            *this = make_enumerator_adapter_proxy<T>(std::forward<U>(param));
            return *this;
        }

        bool is_valid() const
        {
            return m_adapter && m_adapter->is_valid();
        }

        T& next()
        {
            if (!m_adapter)
            {
                throw std::out_of_range("enumerator::next()");
            }
            else
            {
                return m_adapter->next();
            }
        }

    private:
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
        enumerator_adapter_interface<T>* m_adapter;
        char m_storage[internal_storage_size];
    };

    template <typename T, typename U>
    inline auto make_enumerator_adapter_proxy(U&& param) -> enumerator_adapter_proxy<T, decltype(make_enumerator_adapter(std::forward<U>(param)))>
    {
        return enumerator_adapter_proxy<T, decltype(make_enumerator_adapter(std::forward<U>(param)))>(make_enumerator_adapter(std::forward<U>(param)));
    }

    namespace detail
    {
        //
        //  Workaround for a limitation in Visual C++ to do
        //      declspec(foo())::value_type
        //
        template <typename T>
        inline auto get_enumerator_value_type(const T& adapter) -> typename T::value_type
        {
        }
    }

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
        return enumerator<U>(make_enumerator_adapter_proxy<U>(std::forward<T>(param)));
    }

    //
    //  Makes an implicitly-typed enumerator out of the source object.
    //
    //  Example:
    //      std::vector<int> v;
    //      auto e = make_enumerator(v);        //  enumerator<int>
    //
    template <typename T>
    inline auto make_enumerator(T&& param) -> enumerator<decltype(detail::get_enumerator_value_type(make_enumerator_adapter(std::forward<T>(param))))>
    {
        typedef decltype(detail::get_enumerator_value_type(make_enumerator_adapter(std::forward<T>(param)))) value_type;
        return enumerator<value_type>(make_enumerator_adapter_proxy<value_type>(std::forward<T>(param)));
    }

    //
    //  Enumerator adapter based on a range specified by a pair of STL-compatible
    //  forward iterators.
    //
    template <typename T>
    class iterator_enumerator_adapter
    {
    public:
        typedef T iterator_type;
        typedef typename std::iterator_traits<T>::value_type value_type;
        typedef iterator_enumerator_adapter<T> this_type;
        
        iterator_enumerator_adapter(const iterator_type& begin, const iterator_type& end)
        : m_begin(begin), m_end(end)
        {
        }

        iterator_enumerator_adapter(this_type&& rhs)
        : m_begin(std::move(rhs.m_begin)), m_end(std::move(rhs.m_end))
        {
        }

        this_type& operator=(this_type&& rhs)
        {
            m_begin = std::move(rhs.m_begin);
            m_end = std::move(rhs.m_end);
            return *this;
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

    private:
        T m_begin, m_end;
    };

    //
    //  Makes an iterator_enumerator_adapter out of an STL-compatible collection.
    //
    template <typename T>
    inline iterator_enumerator_adapter<typename T::iterator> make_enumerator_adapter(T& collection)
    {
        return iterator_enumerator_adapter<typename T::iterator>(collection.begin(), collection.end());
    }

    template <typename T>
    inline iterator_enumerator_adapter<typename T::const_iterator> make_enumerator_adapter(const T& collection)
    {
        return iterator_enumerator_adapter<typename T::const_iterator>(collection.cbegin(), collection.cend());
    }

    //
    //  Enumerator adapter which embeds an STL-compatible collection in the enumerator.
    //
    //  This is useful for return values, for example:
    //
    //  enumerator<int> foo()
    //  {
    //      std::vector<int> v;
    //      v.push_back(0);
    //      v.push_back(1);
    //      v.push_back(2);
    //      return std::move(v);
    //  }
    //
    //  The std::move is important - without it, an iterator_enumerator_adapter will
    //  return where the iterators refer to a non-existent object that has been
    //  automatically destroyed. With the std::move, the vector is moved into the
    //  resulting enumerator object.
    //
    //  Parameters:
    //      [template] T
    //          Collection type.
    //
    template <typename T>
    class embedded_enumerator_adapter
    {
    public:
        typedef T collection_type;
        typedef typename T::iterator iterator_type;
        typedef typename T::value_type value_type;
        typedef embedded_enumerator_adapter<T> this_type;

        embedded_enumerator_adapter(T&& collection)
        : m_collection(new T(std::move(collection))), 
          m_begin(m_collection->begin()), 
          m_end(m_collection->end())
        {
        }

        embedded_enumerator_adapter(this_type&& rhs)
        : m_collection(std::move(rhs.m_collection)),
          m_begin(std::move(rhs.m_begin)),
          m_end(std::move(rhs.m_end))
        {
        }

        this_type& operator=(this_type&& rhs)
        {
            m_collection = std::move(rhs.m_collection);
            m_begin = std::move(rhs.m_begin);
            m_end = std::move(rhs.m_end);
            return *this;
        }

        bool is_valid() const
        {
            return m_begin != m_end;
        }

        value_type& next()
        {
            if (!is_valid())
            {
                throw std::out_of_range("embedded_enumerator_adapter::next()");
            }
            return *m_begin++;
        }

    private:
        std::unique_ptr<collection_type> m_collection;
        iterator_type m_begin, m_end;
    };

    template <typename T>
    inline embedded_enumerator_adapter<T> make_enumerator_adapter(T&& collection,
                                                                  const typename T::iterator* dummy = nullptr)
    {
        // The dummy parameter ensures this will only be instantiated for collection types.
        return embedded_enumerator_adapter<T>(std::move(collection));
    }

    //
    //  Enumerator adapter which encapsulates a functor.
    //
    //  The functor's return type must be wrapped in a boost::optional object. When
    //  the return value is empty, is_valid() will return false.
    //
    //  Inside of the boost::optional object can either be a reference type or value
    //  type. If it is a reference type, then it can be mutated via enumerator::next().
    //  If it is a value type then a copy will be made and the reference returned by
    //  enumerator::next() will be to the copy, and any mutations will not affect
    //  the original object.
    //
    //  Parameters:
    //      [template] F
    //          Functor type.
    //      [template] T
    //          Return value of the functor; this is not deducible automatically,
    //          however specialization of make_enumerator_adapter for callable
    //          objects can deduce it via decltype.
    //
    template <typename F, typename T>
    class functional_enumerator_adapter
    {
    public:
        typedef functional_enumerator_adapter<F, T> this_type;
        typedef F function_type;
        typedef T return_type;
        typedef typename boost::remove_reference<T>::type value_type;

        functional_enumerator_adapter(const function_type& func)
        : m_func(func), m_hungry(true)
        {
        }

        functional_enumerator_adapter(this_type&& rhs)
        : m_func(std::move(rhs.m_func)), m_next(std::move(rhs.m_next)), m_hungry(std::move(rhs.m_hungry))
        {
        }

        this_type& operator=(this_type&& rhs)
        {
            m_func = std::move(rhs.m_func);
            m_next = std::move(rhs.m_next);
            m_hungry = std::move(rhs.m_hungry);
            return *this;
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

        value_type& next()
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

    private:
        function_type m_func;
        mutable boost::optional<return_type> m_next;
        mutable bool m_hungry;
    };

    namespace detail
    {
        //  Workaround for a limitation in Visual C++ making it impossible to do this:
        //      decltype(func())::value_type
        template <typename T>
        auto get_optional_internal_type(boost::optional<T>& t) -> T
        {
        }

        template <typename T>
        auto get_optional_internal_type(boost::optional<T&>& t) -> T&
        {
        }
    }

    template <typename T>
    inline auto make_enumerator_adapter(const T& func, decltype(func())* dummy = nullptr) -> 
        functional_enumerator_adapter<T, decltype(detail::get_optional_internal_type(func()))>
    {
        //  The dummy parameter prevents this from being instantiated for non-callable types.
        //  It's ugly, but it works.
        return functional_enumerator_adapter<T, decltype(detail::get_optional_internal_type(func()))>(func);
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ENUMERATOR_HPP
