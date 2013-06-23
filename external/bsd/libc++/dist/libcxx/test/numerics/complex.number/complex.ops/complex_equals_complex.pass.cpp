//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <complex>

// template<class T>
//   bool
//   operator==(const complex<T>& lhs, const complex<T>& rhs);

#include <complex>
#include <cassert>

template <class T>
void
test(const std::complex<T>& lhs, const std::complex<T>& rhs, bool x)
{
    assert((lhs == rhs) == x);
}

template <class T>
void
test()
{
    {
    std::complex<T> lhs(1.5,  2.5);
    std::complex<T> rhs(1.5, -2.5);
    test(lhs, rhs, false);
    }
    {
    std::complex<T> lhs(1.5, 2.5);
    std::complex<T> rhs(1.5, 2.5);
    test(lhs, rhs, true);
    }
}

int main()
{
    test<float>();
    test<double>();
    test<long double>();
}
