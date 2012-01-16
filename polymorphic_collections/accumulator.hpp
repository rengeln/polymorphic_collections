//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP
#define POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP

#include <algorithm>
#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/type_traits.hpp>

namespace polymorphic_collections
{
    //
    //  Interface for an accumulator for a collection.
    //  
    //  Parameters:
    //      [template] T
    //          Value type held by the collection.
    //
    template <typename T>
    class accumulator
    {
    public:
        //
        //  Copies an item into the collection.
        //
        virtual accumulator<T>& add(const T& value) = 0;

        //
        //  Moves an item into the collection.
        //
        virtual accumulator<T>& add(T&& value) = 0;
    };

    //
    //  Accumulator which encapsulates an STL-compatible collection implementing a push_back and emplace_back method.
    //  
    //  Parameters:
    //      [template] T
    //          Collection type.
    //
    template <typename T>
    class push_back_accumulator : public accumulator<typename T::value_type>
    {
    public:
        typedef T collection_type;
        typedef typename T::value_type value_type;

        //
        //  Constructor.
        //
        //  Parameters:
        //      [in] collection
        //          Collection to encapsulate.
        //
        push_back_accumulator(T& collection);

        //
        //  From accumulator<>.
        //  
        virtual accumulator<value_type>& add(const value_type& value);
        virtual accumulator<value_type>& add(value_type&& value);

    private:
        //
        //  Properties.
        //  
        T& m_collection;
    };

    template <typename T>
    inline push_back_accumulator<T>::push_back_accumulator(T& collection)
    : m_collection(collection)
    {
    }

    template <typename T>
    inline accumulator<typename T::value_type>& push_back_accumulator<T>::add(const typename T::value_type& value)
    {
        m_collection.push_back(value);
        return *this;
    }

    template <typename T>
    inline accumulator<typename T::value_type>& push_back_accumulator<T>::add(typename T::value_type&& value)
    {
        m_collection.emplace_back(std::move(value));
    }

    //
    //  Returns an accumulator encapsulating an STL-compatible container.
    //
    template <typename T>
    inline push_back_accumulator<T> make_accumulator(T& collection)
    {
        return push_back_accumulator<T>(collection);
    }

    //
    //  Accumulator which encapsulates a functor.
    //
    //  Parameters:
    //      [template] T
    //          Value type accepted by the functor; this is deduced
    //          by the make_accumulator() specialization for function-like
    //          objects (with some help from boost::function_types.)
    //      [template] F
    //          Functor type.
    //
    template <typename T, typename F>
    class functional_accumulator : public accumulator<T>
    {
    public:
        typedef T value_type;
        typedef F function_type;

        //
        //  Constructs the accumulator from a functor.
        //  
        //  Parameters:
        //      [in] functor
        //          The functor which will receive accumulated values.
        //          
        functional_accumulator(const F& func);

        //
        //  From accumulator<>.
        //
        virtual accumulator<T>& add(const T& value);
        virtual accumulator<T>& add(T&& value);

    private:
        //
        //  Properties.
        //
        const F& m_func;
    };

    template <typename T, typename F>
    inline functional_accumulator<T, F>::functional_accumulator(const F& func)
    : m_func(func)
    {
    }

    template <typename T, typename F>
    inline accumulator<T>& functional_accumulator<T, F>::add(const T& value)
    {
        m_func(value);
        return *this;
    }

    template <typename T, typename F>
    inline accumulator<T>& functional_accumulator<T, F>::add(T&& value)
    {
        m_func(std::move(value));
        return *this;
    }

    //
    //  Helper class for dealing with template spew required by make_accumulator().
    //
    namespace detail
    {
        template <typename T>
        struct make_functional_accumulator
        {
            typedef typename boost::mpl::begin<
                typename boost::function_types::parameter_types<T>::type> parameter_type;
            typedef typename boost::remove_reference<
                typename boost::remove_cv<parameter_type>::type> stripped_parameter_type;
        };
    }
    
    //
    //  Returns an accumulator wrapping a functor.
    //
    template <typename T>
    inline functional_accumulator<typename detail::make_functional_accumulator<T>::stripped_parameter_type, T> make_accumulator(const T& func)
    {
        return functional_accumulator<typename detail::make_functional_accumulator<T>::stripped_parameter_type, T>(func);
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ACCUMULATOR_HPP

