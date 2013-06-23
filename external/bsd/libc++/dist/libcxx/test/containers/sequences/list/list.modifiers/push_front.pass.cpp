//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <list>

// void push_front(const value_type& x);

#include <list>
#include <cassert>

int main()
{
    std::list<int> c;
    for (int i = 0; i < 5; ++i)
        c.push_front(i);
    int a[] = {4, 3, 2, 1, 0};
    assert(c == std::list<int>(a, a+5));
}
