// $NetBSD: ccexc.cc,v 1.1 2000/12/08 19:21:28 drochner Exp $

// generate references to exception handling runtime code

extern "C" void ccexc(void);

void
ccexc(void)
{
	try {
		throw "mist";
	} catch (char *e) {
	}
}
