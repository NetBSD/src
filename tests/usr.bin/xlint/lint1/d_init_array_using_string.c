/*	$NetBSD: d_init_array_using_string.c,v 1.1 2021/03/23 21:19:08 rillig Exp $	*/
# 3 "d_init_array_using_string.c"

/*
 * Test initialization of arrays and pointers by string literals.
 */

void sink(const void *);

void
test_assignment_initialization(void)
{
	const char *cs_match = "";
	const int *ws_match = L"";

	const char *cs_mismatch = L"";	/* expect: illegal pointer combination */
	const int *ws_mismatch = "";	/* expect: illegal pointer combination */
}

void
test_pointer_initialization_in_struct(void)
{
	struct cs_ws {
		const char *cs;
		const int *ws;
	};

	struct cs_ws type_match = {
	    	"",
	    	L"",
	};

	struct cs_ws type_mismatch = {
	    	L"",		/* expect: illegal pointer combination */
	    	"",		/* expect: illegal pointer combination */
	};

	struct cs_ws extra_braces = {
		{ "" },
		{ L"" },
	};
}


void
test_array_initialization_in_struct(void)
{
	struct cs_ws {
		const char cs[10];
		const int ws[10];
	};

	struct cs_ws type_match = {
	    	"",
	    	L"",
	};

	struct cs_ws type_mismatch = {
	    	L"",		/* expect: illegal combination */
	    	"",		/* expect: illegal combination */
	};

	struct cs_ws no_terminating_null = {
	    	"0123456789",
	    	L"0123456789",
	};

	struct cs_ws too_many_characters = {
	    	"0123456789X",	/* expect: non-null byte ignored */
	    	L"0123456789X",	/* expect: non-null byte ignored */
	};

	struct cs_ws extra_braces = {
		{ "" },		/* expect: illegal combination *//*FIXME*/
		{ L"" },	/* expect: illegal combination *//*FIXME*/
	};
}
