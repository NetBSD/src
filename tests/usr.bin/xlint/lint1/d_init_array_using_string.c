/*	$NetBSD: d_init_array_using_string.c,v 1.12 2022/06/22 19:23:18 rillig Exp $	*/
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

	/* expect+1: warning: illegal combination of 'pointer to const char' and 'pointer to int', op 'init' [124] */
	const char *cs_mismatch = L"";
	/* expect+1: warning: illegal combination of 'pointer to const int' and 'pointer to char', op 'init' [124] */
	const int *ws_mismatch = "";
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
		/* expect+1: warning: illegal combination of 'pointer to const char' and 'pointer to int', op 'init' [124] */
		L"",
		/* expect+1: warning: illegal combination of 'pointer to const int' and 'pointer to char', op 'init' [124] */
		"",
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
		/* expect+1: warning: illegal combination of integer 'char' and pointer 'pointer to int' [183] */
		L"",
		/* expect+1: warning: illegal combination of integer 'char' and pointer 'pointer to char' [183] */
		"",
	};

	struct cs_ws no_terminating_null = {
		"0123456789",
		L"0123456789",
	};

	struct cs_ws too_many_characters = {
		/* expect+1: warning: string literal too long (11) for target array (10) [187] */
		"0123456789X",
		/* expect+1: warning: string literal too long (11) for target array (10) [187] */
		L"0123456789X",
	};

	struct cs_ws extra_braces = {
		{ "" },
		{ L"" },
	};
}
