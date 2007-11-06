/* $NetBSD: static_destructor.cc,v 1.1.2.2 2007/11/06 23:12:28 matt Exp $ */

/*
 * Tests proper order of destructor calls
 * (must be in reverse order of constructor completion).
 * from the gcc mailing list
 */

#include <iostream>
using namespace std;

class A 
{
public:
	int     i;
	A(int j) : i(j) { cout << "constructor A" << endl; }
	~A() { cout << "destructor A : i = " << i << endl; }
};

class B 
{
public:
	A *n;
	B() {
		static A p(1);
		n = &p;
		cout << "constructor B" << endl;
	}
	~B() {
		cout << "destructor B : i = " << n->i << endl;
		n->i = 10;
	}
};

class B x1;

int main (void) { return 0; }
