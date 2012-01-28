////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
//  See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_ALGORITHMS_HPP
#define POLYMORPHIC_COLLECTIONS_ALGORITHMS_HPP

#include "enumerator.hpp"

namespace polymorphic_collections
{
    template <typename T, typename F>
    inline F for_each(enumerator<T>& e, F func)
    {
        while (e.is_valid())
        {
            func(e.next());
        }
        return func;
    }

    template <typename T>
    inline boost::optional<T&> find(enumerator<T>& e, const T& value)
    {
        while (e.is_valid())
        {
            T& ref = e.next();
            if (ref == value)
            {
                return boost::optional<T&>(ref);
            }
        }
        return boost::none;
    }

    template <typename T, typename F>
    inline boost::optional<T&> find_if(enumerator<T>& e, F pred)
    {
        while (e.is_valid())
        {
            T& ref = e.next();
            if (pred(ref))
            {
                return boost::optional<T&>(ref);
            }
        }
        return boost::none;
    }

    template <typename T>
    inline size_t count(enumerator<T>& e, const T& value)
    {
        size_t result = 0;
        while (e.is_valid())
        {
            if (e.next() == value)
            {
                ++result;
            }
        }
        return result;
    }
    
    template <typename T, typename F>
    inline size_t count_if(enumerator<T>& e, F pred)
    {
        size_t result = 0;
        while (e.is_valid())
        {
            if (pred(e.next())
            {
                ++result;
            }
        }
        return result;
    }

    template <typename T>
    inline bool equal(enumerator<T>& lhs, enumerator<T>& rhs)
    {
        while (lhs.is_valid() && rhs.is_valid())
        {
            if (lhs.next() != rhs.next())
            {
                return false;
            }
        }
        return !lhs.is_valid() && !rhs.is_valid();
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_ALGORITHMS_HPP
