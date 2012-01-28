//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_ENUMERATOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_ENUMERATOR_HPP

namespace polymorphic_collections
{
    namespace detail
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

            virtual ~enumerator_adapter_interface() = 0 { }
            virtual this_type* move(void* ptr) = 0;
            virtual boost::optional<T&> next() = 0;
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

            virtual ~enumerator_adapter_proxy()
            {
            }

            virtual boost::optional<value_type&> next()
            {
                auto& value = m_adapter.next();
                if (value)
                {
                    value_type& ref = *value;
                    return boost::optional<value_type&>(ref);
                }
                else
                {
                    return boost::none;
                }
            }

            virtual enumerator_adapter_interface<value_type>* move(void* ptr)
            {
                return new(ptr) this_type(std::move(m_adapter));
            }

        private:
            A m_adapter;
        };

        template <typename T, typename U>
        inline auto make_enumerator_adapter_proxy(U&& param) -> enumerator_adapter_proxy<T, decltype(make_enumerator_adapter(std::forward<U>(param)))>
        {
            return enumerator_adapter_proxy<T, decltype(make_enumerator_adapter(std::forward<U>(param)))>(make_enumerator_adapter(std::forward<U>(param)));
        }

        //
        //  Workaround for a limitation in Visual C++ to do
        //      declspec(foo())::value_type
        //
        template <typename T>
        inline auto get_enumerator_value_type(const T& adapter) -> typename T::value_type
        {
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
            typedef typename std::iterator_traits<T>::value_type iterator_value_type;
            typedef iterator_enumerator_adapter<T> this_type;
            //  iterator_traits<const T*>::value_type = T, we need to re-add the const
            //  but only if iterator_type is a const pointer.
            typedef typename boost::mpl::if_<
                boost::is_const<typename boost::remove_pointer<T>::type>,
                const iterator_value_type, 
                iterator_value_type>::type value_type;

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

            boost::optional<value_type&> next()
            {
                if (m_begin != m_end)
                {
                    return *m_begin++;
                }
                else
                {
                    return boost::none;
                }
            }

        private:
            T m_begin, m_end;
        };

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

            boost::optional<value_type&> next()
            {
                if (m_begin != m_end)
                {
                    return *m_begin++;
                }
                else
                {
                    return boost::none;
                }
            }

        private:
            std::unique_ptr<collection_type> m_collection;
            iterator_type m_begin, m_end;
        };

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
            : m_func(func)
            {
            }

            functional_enumerator_adapter(this_type&& rhs)
            : m_func(std::move(rhs.m_func))
            {
            }

            this_type& operator=(this_type&& rhs)
            {
                m_func = std::move(rhs.m_func);
                return *this;
            }

            boost::optional<return_type>& next()
            {
                m_value = m_func();
                return m_value;
            }

        private:
            function_type m_func;
            boost::optional<return_type> m_value;
        };

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

        //
        //  Tests if a type is suitable for encapsulation in an iterator_enumerator_adapter.
        //
        template <typename T>
        struct supports_iterator_enumerator_adapter
        {
            static const bool value = has_iterator<T>::value;
        };

        //
        //  Tests if a type is suitable for encapsulation in a embedded_enumerator_adapter.
        //
        template <typename T>
        struct supports_embedded_enumerator_adapter
        {
            static const bool value = has_iterator<T>::value;
        };

        //
        //  Tests if a type is suitable for encapsulation in a functional_enumerator_adapter.
        //
        template <typename T>
        struct supports_functional_enumerator_adapter
        {
            static const bool value = is_callable<T>::value;
            typedef typename is_callable<T>::return_type return_type;
        };

        //
        //  Makes an iterator_enumerator_adapter out of an STL-compatible collection.
        //
        //  It would be nice to be able to do this:
        //      auto make_enumerator_adapter(T& t) -> iterator_enumerator_adapter<decltype(std::begin(t))>
        //
        //  Unfortunately that requires the generic implementation of std::begin() to be defined as:
        //      auto std::begin(T& t) -> decltype(t.begin())
        //
        //  Otherwise, even types that don't have a begin() method or std::begin() specialization
        //  will be matched, breaking the other make_enumerator_adapter specializations.
        //
        //  So for now we have specializations that explicitly refer to T::iterator (to match STL collections)
        //  and specializations explicitly for arrays.
        //
        template <typename T>
        inline typename boost::enable_if<supports_iterator_enumerator_adapter<T>, iterator_enumerator_adapter<typename T::iterator>>::type
            make_enumerator_adapter(T& collection)
        {
            return iterator_enumerator_adapter<typename T::iterator>(collection.begin(), collection.end());
        }

        template <typename T>
        inline typename boost::enable_if<supports_iterator_enumerator_adapter<T>, iterator_enumerator_adapter<typename T::const_iterator>>::type
            make_enumerator_adapter(const T& collection)
        {
            return iterator_enumerator_adapter<typename T::const_iterator>(collection.cbegin(), collection.cend());
        }

        template <typename T, size_t N>
        inline iterator_enumerator_adapter<T*> make_enumerator_adapter(T (&ar)[N])
        {
            return iterator_enumerator_adapter<T*>(std::begin(ar), std::end(ar));
        }

        template <typename T>
        inline iterator_enumerator_adapter<T*> make_enumerator_adapter(T* begin, T* end)
        {
            return iterator_enumerator_adapter<T*>(begin, end);
        }

        template <typename T>
        inline typename boost::enable_if<supports_embedded_enumerator_adapter<T>, embedded_enumerator_adapter<T>>::type 
            make_enumerator_adapter(T&& collection)
        {
            return embedded_enumerator_adapter<T>(std::move(collection));
        }

        template <typename T>
        inline auto make_enumerator_adapter(T&& func, 
            decltype(func())* dummy = nullptr) -> 
            functional_enumerator_adapter<T, decltype(detail::get_optional_internal_type(func()))>
        {
            //  The dummy parameter prevents this from being instantiated for non-callable types.
            //  Haven't been able to get enable_if<is_callable> to work.
            return functional_enumerator_adapter<T, decltype(detail::get_optional_internal_type(func()))>(std::forward<T>(func));
        }
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_ENUMERATOR_HPP
