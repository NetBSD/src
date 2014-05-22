//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <thread>

// class thread

// thread& operator=(thread&& t);

#include <thread>
#include <new>
#include <cstdlib>
#include <cassert>

class G
{
    int alive_;
public:
    static int n_alive;
    static bool op_run;

    G() : alive_(1) {++n_alive;}
    G(const G& g) : alive_(g.alive_) {++n_alive;}
    ~G() {alive_ = 0; --n_alive;}

    void operator()()
    {
        assert(alive_ == 1);
        assert(n_alive >= 1);
        op_run = true;
    }

    void operator()(int i, double j)
    {
        assert(alive_ == 1);
        assert(n_alive >= 1);
        assert(i == 5);
        assert(j == 5.5);
        op_run = true;
    }
};

int G::n_alive = 0;
bool G::op_run = false;

void f1()
{
    std::exit(0);
}

int main()
{
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    std::set_terminate(f1);
    {
        assert(G::n_alive == 0);
        assert(!G::op_run);
        std::thread t0(G(), 5, 5.5);
        std::thread::id id = t0.get_id();
        std::thread t1;
        t1 = std::move(t0);
        assert(t1.get_id() == id);
        assert(t0.get_id() == std::thread::id());
        t1.join();
        assert(G::n_alive == 0);
        assert(G::op_run);
    }
    {
        std::thread t0(G(), 5, 5.5);
        std::thread::id id = t0.get_id();
        std::thread t1;
        t0 = std::move(t1);
        assert(false);
    }
#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
}
