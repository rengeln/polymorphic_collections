//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <vector>
#include <gtest/gtest.h>
#include "polymorphic_collections/enumerator.hpp"
#include "test_utils.hpp"

using namespace polymorphic_collections;

TEST(EnumeratorTests, EnumeratorIs32Bytes)
{
    ASSERT_EQ(sizeof(enumerator<int>), 32);
}

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

TEST(EnumeratorTests, EnumeratorThrowsOutOfRangeWhenEmpty)
{
    enumerator<int> e;
    ASSERT_FALSE(e.is_valid());
    ASSERT_THROW(e.next(), std::out_of_range);
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
        return ++x;
    }
    else
    {
        return boost::none;
    }
}

TEST(EnumeratorTests, EnumeratorCanEncapsulateFunctionPointer)
{
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
            return ++x;
        }
        else
        {
            return boost::none;
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

TEST(EnumeratorTests, FunctionalEnumeratorReturningByValueDoesNotAllowModification)
{
    std::vector<int> v;
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    auto i = v.begin();
    enumerator<int> e = [&]() -> boost::optional<int>
    {
        if (i != v.end())
        {
            return *i++;
        }
        else
        {
            return boost::none;
        }
    };
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 2);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 3);
    ASSERT_FALSE(e.is_valid());

    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 1);
    ASSERT_EQ(v[2], 2);
}

TEST(EnumeratorTests, FunctionalEnumeratorReturningByReferenceAllowsModification)
{
    std::vector<int> v;
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    auto i = v.begin();
    enumerator<int> e = [&]() -> boost::optional<int&>
    {
        if (i != v.end())
        {
            return *i++;
        }
        else
        {
            return boost::none;
        }
    };
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 2);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 3);
    ASSERT_FALSE(e.is_valid());

    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 3);
}

TEST(EnumeratorTests, EnumeratorCanHandleMoveOnlyTypes)
{
    std::vector<MoveOnly> v;
    v.push_back(MoveOnly(0));
    v.push_back(MoveOnly(1));
    v.push_back(MoveOnly(2));
    enumerator<MoveOnly> e = v;
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value, 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value, 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value, 2);
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, CanMoveObjectsFromEnumerator)
{
    std::vector<MoveOnly> v;
    v.push_back(MoveOnly(0));
    v.push_back(MoveOnly(1));
    v.push_back(MoveOnly(2));
    enumerator<MoveOnly> e = v;
    ASSERT_TRUE(e.is_valid());
    MoveOnly a = std::move(e.next());
    ASSERT_EQ(a.value, 0);
    ASSERT_TRUE(e.is_valid());
    MoveOnly b = std::move(e.next());
    ASSERT_EQ(b.value, 1);
    ASSERT_TRUE(e.is_valid());
    MoveOnly c = std::move(e.next());
    ASSERT_EQ(c.value, 2);
    ASSERT_FALSE(e.is_valid());

    ASSERT_EQ(v[0].value, -1);
    ASSERT_EQ(v[1].value, -1);
    ASSERT_EQ(v[2].value, -1);
}

TEST(EnumeratorTests, EnumeratorOfConcreteBaseTypeCanEncapsulateCollectionOfDerivedType)
{
    struct Foo
    {
        int value;
        Foo(int v) : value(v) { }
    };

    struct Bar : Foo
    {
        Bar(int v) : Foo(v) { }
    };

    std::vector<Bar> v;
    v.push_back(Bar(0));
    v.push_back(Bar(1));
    v.push_back(Bar(2));
    enumerator<Foo> e = v;

    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value, 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value, 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value, 2);
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorOfAbstractBaseTypeCanEncapsulateCollectionOfDerivedType)
{
    struct Foo
    {
        virtual ~Foo() = 0 { }
        virtual int value() = 0;
    };

    struct Bar : Foo
    {
        Bar(int v) : _v(v) { }
        virtual ~Bar() { }
        virtual int value() { return _v; }

    private:
        int _v;
    };

    std::vector<Bar> v;
    v.push_back(Bar(0));
    v.push_back(Bar(1));
    v.push_back(Bar(2));
    enumerator<Foo> e = v;

    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value(), 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next().value(), 2);
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorCanEncapsulateArray)
{
    int ar[] = {0, 1, 2};
    enumerator<int> e = ar;

    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 2);
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorCanEncapsulateConstArray)
{
    const int ar[] = {0, 1, 2};
    enumerator<const int> e = ar;

    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 2);
    ASSERT_FALSE(e.is_valid());
}

TEST(EnumeratorTests, EnumeratorAllowsMutationOfArrayContents)
{
    int ar[] = {0, 1, 2};
    enumerator<int> e = ar;

    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 2);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(++e.next(), 3);
    ASSERT_FALSE(e.is_valid());

    ASSERT_EQ(ar[0], 1);
    ASSERT_EQ(ar[1], 2);
    ASSERT_EQ(ar[2], 3);
}

TEST(EnumeratorTests, EnumeratorCanEncapsulatePointerPlusSize)
{
    std::unique_ptr<int[]> v(new int[3]);
    v[0] = 0;
    v[1] = 1;
    v[2] = 2;
    auto e = make_enumerator(v.get(), 3);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 0);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 1);
    ASSERT_TRUE(e.is_valid());
    ASSERT_EQ(e.next(), 2);
    ASSERT_FALSE(e.is_valid());
}
