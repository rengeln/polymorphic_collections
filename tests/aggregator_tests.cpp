//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include <map>
#include <gtest/gtest.h>
#include "polymorphic_collections/aggregator.hpp"
#include "test_utils.hpp"

using namespace polymorphic_collections;

TEST(AggregatorTests, AggregatorCanEncapsulateMap)
{
    std::map<int, int> m;
    aggregator<int, int> a = m;
    a.add(1, 2);
    a.add(2, 5);
    ASSERT_EQ(m[1], 2);
    ASSERT_EQ(m[2], 5);
}

std::map<int, std::string> g_globalMap;
void AggregateGlobalMap(int k, const std::string& v)
{
    g_globalMap[k] = v;
}

TEST(AggregatorTests, AggregatorCanEncapsulateFunctionPointer)
{
    g_globalMap.clear();
    aggregator<int, std::string> a = &AggregateGlobalMap;
    a.add(1, "one");
    a.add(2, "two");
    a.add(3, "three");
    ASSERT_STREQ(g_globalMap[1].c_str(), "one");
    ASSERT_STREQ(g_globalMap[2].c_str(), "two");
    ASSERT_STREQ(g_globalMap[3].c_str(), "three");
}

TEST(AggregatorTests, AggregatorCanEncapsulateLambda)
{
    std::map<int, std::string> m;
    aggregator<int, std::string> a = [&] (const int& k, std::string v) -> void
    {
        m[k] = v;
    };
    a.add(1, "one");
    a.add(2, "two");
    a.add(3, "three");
    ASSERT_STREQ(m[1].c_str(), "one");
    ASSERT_STREQ(m[2].c_str(), "two");
    ASSERT_STREQ(m[3].c_str(), "three");
}
