//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_COMMON_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_COMMON_HPP

#include <boost/mpl/has_xxx.hpp>
#include <boost/type_traits.hpp>

namespace polymorphic_collections
{
    namespace detail
    {
        struct yes_tag
        {
            char _[1];
        };

        struct no_tag
        {
            char _[2];
        };

        template <typename T>
        struct is_callable
        {
            template <typename U>
            static auto check_type(U* t) -> decltype((*t)())
            {
            }
            static void check_type(...)
            {
            }
            template <typename U>
            static yes_tag test(U* t, decltype((*t)()))
            {
            }
            static no_tag test(...)
            {
            }
            static const bool value = sizeof(is_callable::test((T*)0)) == sizeof(yes_tag);
            typedef decltype(is_callable::check_type((T*)0)) return_type;
        };

        template <typename T, typename A>
        struct is_callable_1
        {
            template <typename U>
            static auto check_type(U* t) -> decltype((*t)(A()))
            {
            }
            static void check_type(...)
            {
            }
            template <typename U>
            static yes_tag test(U* t, decltype((*t)(A())))
            {
            }
            static no_tag test(...)
            {
            }
            static const bool value = sizeof(is_callable::test((T*)0)) == sizeof(yes_tag);
            typedef decltype(is_callable::check_type((T*)0)) return_type;
        };

#define HAS_METHOD_DEF_0(x)                                                             \
        template <typename T>                                                           \
        struct has_##x                                                                  \
        {                                                                               \
            template <typename U>                                                       \
            static auto check_type(U* t) -> decltype(t->x(a()))                         \
            {                                                                           \
            }                                                                           \
            static void check_type(...)                                                 \
            {                                                                           \
            }                                                                           \
            template <typename U>                                                       \
            static yes_tag test(U* t, decltype(t->x())* dummy = 0)                      \
            {                                                                           \
            }                                                                           \
            static no_tag test(...)                                                     \
            {                                                                           \
            }                                                                           \
            static const bool value = sizeof(has_##x::test((T*)0)) == sizeof(yes_tag);  \
            typedef decltype(has_##x::check_type((T*)0)) return_type;                   \
        };
#define HAS_METHOD_DEF_1(x, a)                                                          \
        template <typename T>                                                           \
        struct has_##x                                                                  \
        {                                                                               \
            typedef typename boost::remove_reference<T>::type naked_type;               \
            typedef typename boost::remove_reference<a>::type arg1_type;                \
            template <typename U>                                                       \
            static auto check_type(U* t) -> decltype(t->x(*(arg1_type*)0))                         \
            {                                                                           \
            }                                                                           \
            static void check_type(...)                                                 \
            {                                                                           \
            }                                                                           \
            template <typename U>                                                       \
            static yes_tag test(U* t, decltype(t->x(*(arg1_type*)0))* dummy = 0)                   \
            {                                                                           \
            }                                                                           \
            static no_tag test(...)                                                     \
            {                                                                           \
            }                                                                           \
            static const bool value = sizeof(has_##x::test((naked_type*)0)) == sizeof(yes_tag);  \
            typedef decltype(has_##x::check_type((naked_type*)0)) return_type;                   \
        };

        HAS_METHOD_DEF_1(push_back, typename T::value_type)
        HAS_METHOD_DEF_1(find, typename T::key_type)

        BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_COMMON_HPP
