// $NetBSD: construct.cc,v 1.2 2003/09/03 20:53:16 drochner Exp $

// check constructor / destructor calls

#include <iostream>

using namespace std;

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
