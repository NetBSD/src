// $NetBSD: construct.cc,v 1.1 2000/12/08 19:21:28 drochner Exp $

// check constructor / destructor calls

#include <iostream>

class mist {
public:
	mist(void);
	~mist();
};

mist::mist(void)
{
	cout << "constructor" << endl;
}

mist::~mist()
{
	cout << "destructor" << endl;
}

static mist construct;
