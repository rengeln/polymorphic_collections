//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_ENUMERATOR_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_ENUMERATOR_HPP

#include "common.hpp"

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

            virtual ~enumerator_adapter_interface() = 0;
            virtual this_type* move(void* ptr) = 0;
            virtual boost::optional<T&> next() = 0;
        };
        
        template <typename T>
        inline enumerator_adapter_interface<T>::~enumerator_adapter_interface()
        {
        }

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
                auto value = m_adapter.next();
                if (value)
                {
                    return *value;
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
                    T cur = m_begin++;
                    return *cur;
                }
                else
                {
                    return boost::none;
                }
            }

        private:
            T m_begin, m_end;
        };

        template <typename T, typename C>
        struct supports_iterator_enumerator_adapter
        {
            static const bool value = has_iterator<C>::value;
        };

        template <typename T, typename C>
        inline auto make_enumerator_adapter(C& collection)
            -> typename boost::enable_if<supports_iterator_enumerator_adapter<T, C>,
                                         iterator_enumerator_adapter<typename C::iterator>>::type
        {
            return iterator_enumerator_adapter<typename C::iterator>(std::begin(collection), std::end(collection));
        }

        template <typename T, typename C>
        inline auto make_enumerator_adapter(const C& collection)
            -> typename boost::enable_if<supports_iterator_enumerator_adapter<T, C>,
                                         iterator_enumerator_adapter<typename C::const_iterator>>::type
        {
            return iterator_enumerator_adapter<typename C::const_iterator>(collection.cbegin(), collection.cend());
        }

        template <typename T, size_t N>
        inline auto make_enumerator_adapter(T (&ar)[N])
            -> iterator_enumerator_adapter<T*>
        {
            return iterator_enumerator_adapter<T*>(std::begin(ar), std::end(ar));
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

        template <typename T, typename C>
        struct supports_embedded_enumerator_adapter
        {
            static const bool value = has_iterator<C>::value;
        };
        
        template <typename T, typename C>
        inline auto make_enumerator_adapter(C&& collection, typename C::iterator* dummy = nullptr)
            -> typename boost::enable_if<supports_embedded_enumerator_adapter<T, typename boost::remove_reference<C>::type>,
                                         embedded_enumerator_adapter<typename boost::remove_reference<C>::type>>::type
        {
            // FIXME: the dummy parameter should not be necessary but Visual C++ chokes if it isn't present.
            return embedded_enumerator_adapter<typename boost::remove_reference<C>::type>(std::forward<C>(collection));
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
        //
        template <typename F>
        class functional_enumerator_adapter
        {
        public:
            typedef functional_enumerator_adapter<F> this_type;
            typedef F function_type;
            typedef typename is_callable<F>::return_type::value_type return_type;
            typedef typename boost::remove_reference<return_type>::type value_type;

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

            boost::optional<value_type&> next()
            {
                m_value = m_func();
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

        template <typename T, typename F>
        struct supports_functional_enumerator_adapter
        {
            static const bool value = is_callable<F>::value;
        };

        template <typename T, typename F>
        inline auto make_enumerator_adapter(F&& func)
            -> typename boost::enable_if<supports_functional_enumerator_adapter<T, typename boost::remove_reference<F>::type>,
                                         functional_enumerator_adapter<typename boost::remove_reference<F>::type>>::type
        {
            return functional_enumerator_adapter<typename boost::remove_reference<F>::type>(std::forward<F>(func));
        }
        
        //  FIXME: Visual C++ has issues with decltype resolving to int if template
        //  substitution fails instead of giving an error message; typically this
        //  results in errors much deeper in the template code, which are extremely
        //  unclear. This workaround, while hacky, makes error messages MUCH cleaner.
        //  See the commented out code below for the original version.
        template <typename T, typename U>
        struct get_enumerator_adapter_type
        {
            typedef decltype(make_enumerator_adapter<T>(declval<U>())) type;
        };

        template <typename T, typename U>
        inline auto make_enumerator_adapter_proxy(U&& param) 
            -> enumerator_adapter_proxy<T, typename get_enumerator_adapter_type<T, U>::type>
        {
            return enumerator_adapter_proxy<T, typename get_enumerator_adapter_type<T, U>::type>
                (make_enumerator_adapter<T>(std::forward<U>(param)));
        }

/*        
        template <typename T, typename U>
        inline auto make_enumerator_adapter_proxy(U&& param) 
            -> enumerator_adapter_proxy<T, decltype(make_enumerator_adapter<T>(std::forward<U>(param)))>
        {
            return enumerator_adapter_proxy<T, decltype(make_enumerator_adapter<T>(std::forward<U>(param)))>(make_enumerator_adapter<T>(std::forward<U>(param)));
        }
*/               
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_ENUMERATOR_HPP
