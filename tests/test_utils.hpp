//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <boost/utility.hpp>

class NoCopyOrMove : public boost::noncopyable
{
public:
    int value;

    NoCopyOrMove(int v)
    : value(v)
    {
    }
};

class MoveOnly : public boost::noncopyable
{
public:
    int value;

    MoveOnly(int v)
    : value(v)
    {
    }

    MoveOnly(MoveOnly&& rhs)
    : value(rhs.value)
    {
        rhs.value = -1;
    }

    MoveOnly& operator=(MoveOnly&& rhs)
    {
        value = rhs.value;
        rhs.value = -1;
    }
};

class CopyOnly
{
public:
    int value;

    CopyOnly(int v)
    : value(v)
    {
    }
};

#endif  // TEST_UTILS_HPP
