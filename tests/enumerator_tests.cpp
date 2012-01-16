//////////////////////////////////////////////////////////////////////////////// 
// Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
// See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include "polymorphic_collections/enumerator.hpp"

using polymorphic_collections;

void print_collection(enumerator<int>& e)
{
    while (e.is_valid())
    {
        std::cout << e.next() << std::endl;
    }
}

int main()
{    
    std::vector<int> v;
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    print_collection(v);
}
