/* $Id: test_def.h,v 1.2 1993/05/20 05:28:37 cgd Exp $ */

struct blah {
	unsigned int blahfield;
	int		dummyi;
	char 	dummyc;
};

struct test_pcbstruct {
	int test_pcbfield;
	int test_state;
};

#define MACRO1(arg) if(arg != 0) { printf("macro1\n"); }
