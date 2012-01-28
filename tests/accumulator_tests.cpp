//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include <boost/utility.hpp>
#include <array>
#include <vector>
#include <gtest/gtest.h>
#include "polymorphic_collections/accumulator.hpp"
#include "test_utils.hpp"

using namespace polymorphic_collections;

TEST(AccumulatorTests, AccumulatorCanEncapsulateVectors)
{
    std::vector<int> v;
    accumulator<int> a = v;
    a.add(0);
    a.add(1);
    a.add(2);
    
    ASSERT_EQ(v.size(), 3);
    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 1);
    ASSERT_EQ(v[2], 2);
}

TEST(AccumulatorTests, AccumulatorCanEncapsulateArray)
{
    int v[3];
    accumulator<int> a = v;
    a.add(0);
    a.add(1);
    a.add(2);

    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 1);
    ASSERT_EQ(v[2], 2);
}

TEST(AccumulatorTests, AccumulatorThrowsOverflowErrorWhenArraySizeIsExceeded)
{
    int v[3];
    accumulator<int> a = v;
    a.add(0);
    a.add(1);
    a.add(2);
    ASSERT_THROW(a.add(3), std::overflow_error);
}

TEST(AccumulatorTests, AccumulatorCanEncapsulateArrayObject)
{
    std::array<int, 3> v;
    accumulator<int> a = v;
    a.add(0);
    a.add(1);
    a.add(2);

    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 1);
    ASSERT_EQ(v[2], 2);
}

TEST(AccumulatorTests, AccumulatorThrowsOverflowErrorWhenArrayObjectSizeIsExceeded)
{
    std::array<int, 3> v;
    accumulator<int> a = v;
    a.add(0);
    a.add(1);
    a.add(2);
    ASSERT_THROW(a.add(3), std::overflow_error); 
}

void TestFunc(int x)
{
    ASSERT_EQ(x, 1);
}

TEST(AccumulatorTests, AccumulatorCanEncapsulateFunctionPointer)
{
    accumulator<int> a = &TestFunc;
    a.add(1);
    a.add(1);
}

TEST(AccumulatorTests, AccumulatorCanEncapsulateLambda)
{
    int x = 0;
    accumulator<int> a = make_accumulator<int>([&] (int y) -> void
    {
        ASSERT_EQ(y, x);
    });
    a.add(++x);
    a.add(++x);
    a.add(++x);
}

TEST(AccumulatorTests, AccumulatorCanEncapsulateRangeSpecifiedByIterators)
{
    std::array<int, 5> v;
    std::fill(v.begin(), v.end(), 0);
    auto begin = v.begin() + 1;
    auto end = begin + 3;
    accumulator<int> a = make_accumulator(begin, end);
    ASSERT_NO_THROW(a.add(1));
    ASSERT_NO_THROW(a.add(2));
    ASSERT_NO_THROW(a.add(3));
    ASSERT_THROW(a.add(4), std::overflow_error);
    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 1);
    ASSERT_EQ(v[2], 2);
    ASSERT_EQ(v[3], 3);
    ASSERT_EQ(v[4], 0);
}

TEST(AccumulatorTests, AccumulatorCanEnforceSorting)
{
    std::vector<int> v;
    accumulator<int> a = make_accumulator<int>([&] (int x) -> void
    {
        v.push_back(x);
        std::sort(v.begin(), v.end());
    });
    ASSERT_TRUE(std::is_sorted(v.begin(), v.end()));
    a.add(1);
    ASSERT_TRUE(std::is_sorted(v.begin(), v.end()));
    a.add(3);
    ASSERT_TRUE(std::is_sorted(v.begin(), v.end()));
    a.add(2);
    ASSERT_TRUE(std::is_sorted(v.begin(), v.end()));
    a.add(-5);
    ASSERT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(AccumulatorTests, AccumulatorWorksWithMoveOnlyTypes)
{
    std::vector<MoveOnly> v;
    accumulator<MoveOnly> a = v;
    a.add(MoveOnly(0));
    a.add(MoveOnly(1));
    a.add(MoveOnly(2));
    ASSERT_EQ(v.size(), 3);
    ASSERT_EQ(v[0].value, 0);
    ASSERT_EQ(v[1].value, 1);
    ASSERT_EQ(v[2].value, 2);
}

TEST(AccumulatorTests, AccumulatorCanEncapsulatePointerPlusSize)
{
    std::unique_ptr<int[]> v(new int[3]);
    auto a = make_accumulator(v.get(), 3);
    ASSERT_NO_THROW(a.add(0));
    ASSERT_NO_THROW(a.add(1));
    ASSERT_NO_THROW(a.add(2));
    ASSERT_THROW(a.add(3), std::overflow_error);
    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 1);
    ASSERT_EQ(v[2], 2);
}

TEST(AccumulatorTests, AccumulatorCanSpecifyAtomicPolicy)
{
    std::vector<int> v;
    accumulator<int, atomic> a = v;
}
