//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <string>

//       reverse_iterator rend();
// const_reverse_iterator rend() const;

#include <string>
#include <cassert>

template <class S>
void
test(S s)
{
    const S& cs = s;
    typename S::reverse_iterator e = s.rend();
    typename S::const_reverse_iterator ce = cs.rend();
    if (s.empty())
    {
        assert(e == s.rbegin());
        assert(ce == cs.rbegin());
    }
    assert(e - s.rbegin() == s.size());
    assert(ce - cs.rbegin() == cs.size());
}

int main()
{
    typedef std::string S;
    test(S());
    test(S("123"));
}
