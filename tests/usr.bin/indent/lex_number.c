/* $NetBSD: lex_number.c,v 1.2 2023/05/13 13:31:37 rillig Exp $ */

/*
 * Test lexing of numbers.
 */

//indent input
int integers[] = {
	0,			// octal zero
	1000,			// decimal
	10000000000000000000000000000000000000000000,	// big decimal
	100000000000000000000000000000000000000000LU,	// suffix
	0x12345678,		// hex
};

double floats[] = {
	0.0,
	0.0f,
	0.0F,
	1.0e-1000,
	1.0e+1000,
};
//indent end

//indent run-equals-input -di0


//indent input
int wrapped = 0\
x\
12\
3456\
78;
//indent end

//indent run -di0
int wrapped = 0x12345678;
//indent end
