//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_UTILITY_HPP
#define POLYMORPHIC_COLLECTIONS_UTILITY_HPP

#include <boost/mpl/front.hpp>
#include <boost/function_types/function_arity.hpp>

namespace polymorphic_collections
{
    namespace detail
    {
        template <typename T>
        struct has_push_back
        {
            template<void(T::*)(typename T::value_type&&)> struct wrap;
            template<typename U>
            static char(&test(U*, wrap<&U::push_back>* = 0))[1];
            static char(&test(...))[2];

            static const bool value = 1 == sizeof(has_push_back::test((T*)0));
        };

        BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_UTILITY_HPP
