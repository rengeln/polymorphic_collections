//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <vector>
#include <gtest/gtest.h>
#include "polymorphic_collections/enumerator.hpp"

using namespace polymorphic_collections;

TEST(EnumeratorTests, DefaultConstructedEnumeratorIsNotValid)
{
    enumerator<int> e;
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorIsNotValidForEmptyVector)
{
    std::vector<int> v;
    enumerator<int> e = v;
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorCanEncapsulateVectorOfInts)
{
    std::vector<int> v;
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);

    enumerator<int> e = v;
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 2);
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorCanEncapsulateVectorOfStrings)
{
    std::vector<std::string> v;
    v.push_back("one");
    v.push_back("two");
    v.push_back("three");

    enumerator<std::string> e = v;
    ASSERT_TRUE(e.is_valid());
    ASSERT_STREQ(e.next().c_str(), "one");
    ASSERT_TRUE(e.is_valid());
    ASSERT_STREQ(e.next().c_str(), "two");
    ASSERT_TRUE(e.is_valid());
    ASSERT_STREQ(e.next().c_str(), "three");
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorAllowsModificationsOfItemsInVector)
{
    std::vector<int> v;
    v.resize(3, 0);

    enumerator<int> e = v;
    int n = 0;
    while (e.is_valid())
    {
        e.next() = ++n;
    }

    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 3);
}

TEST(EnumeratorTests, MutableEnumeratorCanBeAssignedToConstEnumerator)
{
    std::vector<int> v;
    v.push_back(0);
    enumerator<int> e = v;
    enumerator<const int> f = std::move(v);
}

TEST(EnumeratorTests, EnumeratorMoveAssignmentInvalidatesSource)
{
    std::vector<int> v;
    v.push_back(0);
    enumerator<int> e = v;
    ASSERT_TRUE(e.is_valid());
    enumerator<int> f = std::move(e);
    ASSERT_FALSE(e.is_valid());
    ASSERT_TRUE(f.is_valid());
}

boost::optional<int> CountToThree()
{
    static int x = 0;
    if (x < 3)
    {
        return boost::optional<int>(++x);
    }
    else
    {
        return boost::optional<int>();
    }
}

TEST(EnumeratorTests, EnumeratorCanEncapsulateFunctionPointer)
{
    //  The following will not work under Visual C++ due to a bug:
    //      Foo f = CountToThree;
    //  This is a workaround:
    //auto fn = CountToThree;
    enumerator<int> e = &CountToThree;
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 2);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 3);
    ASSERT_FALSE(e.is_valid());

}


TEST(EnumeratorTests, EnumeratorCanEncapsulateLambda)
{
    int x = 0;
    enumerator<int> e = [&]() -> boost::optional<int>
    {
        if (x < 3)
        {
            return boost::optional<int>(++x);
        }
        else
        {
            return boost::optional<int>();
        }
    };
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 2);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 3);
    ASSERT_FALSE(e.is_valid());
}

