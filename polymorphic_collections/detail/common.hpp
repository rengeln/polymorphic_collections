//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_DETAIL_COMMON_HPP
#define POLYMORPHIC_COLLECTIONS_DETAIL_COMMON_HPP

#include <utility>
#include <boost/optional.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/type_traits.hpp>

namespace polymorphic_collections
{
    namespace detail
    {
        //  Declval isn't included in Visual C++ 2010
        template <typename T>
        static typename boost::add_rvalue_reference<T>::type declval();

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
            static yes_tag test(U* t, decltype((*t)())* dummy = 0)
            {
            }
            static no_tag test(...)
            {
            }
            static const bool value = sizeof(is_callable::test((T*)0)) == sizeof(yes_tag);
            typedef decltype(0, is_callable::check_type((T*)0)) return_type;
        };

        template <typename T, typename A>
        struct is_callable_1
        {
            template <typename U>
            static auto check_type(U* t) -> decltype((*t)(declval<A>()))
            {
            }
            static void check_type(...)
            {
            }
            template <typename U>
            static yes_tag test(U* t, decltype((*t)(declval<A>()))* dummy = 0)
            {
            }
            static no_tag test(...)
            {
            }
            static const bool value = sizeof(is_callable_1::test((T*)0)) == sizeof(yes_tag);
            typedef decltype(0, is_callable_1::check_type((T*)0)) return_type;
        };

        template <typename T, typename A, typename B>
        struct is_callable_2
        {
            template <typename U>
            static auto check_type(U* t) -> decltype((*t)(declval<A>(), declval<B>()))
            {
            }
            static void check_type(...)
            {
            }
            template <typename U>
            static yes_tag test(U* t, decltype((*t)(declval<A>(), declval<B>()))* dummy = 0)
            {
            }
            static no_tag test(...)
            {
            }
            static const bool value = sizeof(is_callable_2::test((T*)0)) == sizeof(yes_tag);
            typedef decltype(0, is_callable_2::check_type((T*)0)) return_type;
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
/*
#define HAS_METHOD_DEF_1(x)                                                             \
        template <typename T, typename A>                                               \
        struct has_##x                                                                  \
        {                                                                               \
            typedef typename boost::remove_reference<T>::type naked_type;               \
            typedef typename boost::remove_reference<A>::type arg1_type;                \
            template <typename U>                                                       \
            static auto check_type(U* t) -> decltype(t->x(*(arg1_type*)0))              \
            {                                                                           \
            }                                                                           \
            static void check_type(...)                                                 \
            {                                                                           \
            }                                                                           \
            template <typename U>                                                       \
            static yes_tag test(U* t, decltype(t->x(*(arg1_type*)0))* dummy = 0)        \
            {                                                                           \
            }                                                                           \
            static no_tag test(...)                                                     \
            {                                                                           \
            }                                                                           \
            static const bool value = sizeof(has_##x::test((naked_type*)0)) == sizeof(yes_tag);  \
            typedef decltype(has_##x::check_type((naked_type*)0)) return_type;                   \
        };
*/
#define HAS_METHOD_DEF_1(x)                                                             \
        template <typename T, typename A>                                               \
        struct has_##x                                                                  \
        {                                                                               \
            template <typename U>                                                       \
            static auto check_type(U&&) -> decltype(declval<U>().x(declval<A>()))       \
            {                                                                           \
            }                                                                           \
            static void check_type(...)                                                 \
            {                                                                           \
            }                                                                           \
            template <typename U>                                                       \
            static yes_tag test(U&&, decltype(declval<U>().x(declval<A>()))* dummy = 0)        \
            {                                                                           \
            }                                                                           \
            static no_tag test(...)                                                     \
            {                                                                           \
            }                                                                           \
            static const bool value = sizeof(test(declval<T>())) == sizeof(yes_tag);  \
            typedef decltype(check_type(declval<T>())) return_type;                   \
        };
        HAS_METHOD_DEF_1(push_back)
        HAS_METHOD_DEF_1(find)
        HAS_METHOD_DEF_1(insert)

        BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(key_type)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(mapped_type)
    }
}

#endif  // POLYMORPHIC_COLLECTIONS_DETAIL_COMMON_HPP
