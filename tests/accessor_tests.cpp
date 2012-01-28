//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include <boost/utility.hpp>
#include <array>
#include <map>
#include <vector>
#include <gtest/gtest.h>
#include "polymorphic_collections/accessor.hpp"
#include "test_utils.hpp"

using namespace polymorphic_collections;

TEST(AccessorTests, DefaultConstructedAccessorReturnsEmptyResults)
{
    accessor<int, int> a;
    ASSERT_FALSE(a[0]);
}

TEST(AccessorTests, AccessorCanEncapsulateMap)
{
    std::map<std::string, std::string> m;
    m["a"] = "one";
    m["b"] = "two";
    m["c"] = "three";
    accessor<std::string, std::string> a = m;
    ASSERT_STREQ(a["a"]->c_str(), "one");
    ASSERT_STREQ(a["b"]->c_str(), "two");
    ASSERT_STREQ(a["c"]->c_str(), "three");
    ASSERT_FALSE(a["d"]);
}

TEST(AccessorTests, AccessorCanEmbedMap)
{
    std::map<std::string, std::string> m;
    m["a"] = "one";
    m["b"] = "two";
    m["c"] = "three";
    accessor<std::string, std::string> a = std::move(m);
    ASSERT_TRUE(m.empty());
    ASSERT_STREQ(a["a"]->c_str(), "one");
    ASSERT_STREQ(a["b"]->c_str(), "two");
    ASSERT_STREQ(a["c"]->c_str(), "three");
    ASSERT_FALSE(a["d"]);
}

TEST(AccessorTests, AccessorCanEmbedMapWithMoveOnlyValueType)
{
    std::map<int, MoveOnly> m;
    m[1] = MoveOnly(1);
    m[2] = MoveOnly(2);
    m[3] = MoveOnly(3);
    accessor<int, MoveOnly> a = m;
    ASSERT_EQ(a[1]->value, 1);
    ASSERT_EQ(a[2]->value, 2);
    ASSERT_EQ(a[3]->value, 3);
}
