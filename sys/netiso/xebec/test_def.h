/* $Id: test_def.h,v 1.3 1994/05/13 06:10:42 mycroft Exp $ */

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
