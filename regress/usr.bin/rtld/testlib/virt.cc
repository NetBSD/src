// $NetBSD: virt.cc,v 1.1 2000/12/08 19:21:28 drochner Exp $

// g++ generates a reference to "__pure_virtual" with this code

class mist {
public:
	virtual void vv(void) = 0;
};

class mast: public mist {
public:
	void vv(void) {};
};

static mast virt;
