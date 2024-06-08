/*	$NetBSD: init.c,v 1.16 2024/06/08 13:50:47 rillig Exp $	*/
# 3 "init.c"

/*
 * Tests for initialization.
 *
 * C99 6.7.8
 */

/* lint1-extra-flags: -X 351 */

/*
 * C99 does not allow empty initializer braces syntactically.
 * Lint allows this syntactically, it just complains if the resulting
 * object is empty.
 */
/* expect+1: error: empty array declaration for 'empty_array_with_initializer' [190] */
double empty_array_with_initializer[] = {};
double array_with_empty_initializer[3] = {};

/*
 * C99 does not allow empty initializer braces syntactically.
 */
struct {
	int member;
} empty_struct_initializer = {};


typedef struct {
	const char *key;
	int n;
} histogram_entry;

/*
 * The C standards allow omitting braces around the structural levels.  For
 * human readers, it is usually clearer to include them.
 *
 * Seen in external/ibm-public/postfix/dist/src/util/dict.c(624).
 */
const histogram_entry hgr[] = {
	"odd", 5,
	"even", 5,
};


/*
 * Initialization with fewer braces than usual, must still be accepted.
 */
struct {
	int x, y;
} points[] = {
	0, 0, 3, 0, 0, 4, 3, 4
};


/*
 * Initialization with fewer braces than usual, must still be accepted.
 */
void do_nothing(void);

struct {
	void (*action_1) (void);
	void (*action_2) (void);
} actions[1] = {
	do_nothing,
	do_nothing,
};


/* expect+1: error: initialization of incomplete type 'incomplete struct incomplete_struct' [175] */
struct incomplete_struct s1 = {
	1,
/* expect+1: error: 's1' has incomplete type 'incomplete struct incomplete_struct' [31] */
};

/* expect+1: error: initialization of incomplete type 'incomplete struct incomplete_struct' [175] */
struct incomplete_struct s2 = {
	.member = 1,
/* expect+1: error: 's2' has incomplete type 'incomplete struct incomplete_struct' [31] */
};

struct incomplete_struct {
	int num;
};


/* expect+1: error: initialization of incomplete type 'incomplete union incomplete_union' [175] */
union incomplete_union u1 = {
	1,
/* expect+1: error: 'u1' has incomplete type 'incomplete union incomplete_union' [31] */
};

/* expect+1: error: initialization of incomplete type 'incomplete union incomplete_union' [175] */
union incomplete_union u2 = {
	.member = 1,
/* expect+1: error: 'u2' has incomplete type 'incomplete union incomplete_union' [31] */
};

union incomplete_union {
	int num;
};


/* expect+1: warning: cannot initialize extern declaration 'extern_var' [26] */
extern int extern_var = 1;
int defined_var = 1;
/* expect+1: warning: static variable 'static_var' unused [226] */
static int static_var = 1;
/* expect+1: error: illegal storage class [8] */
register int register_var = 1;
/* expect+1: error: cannot initialize typedef 'typedef_var' [25] */
typedef int typedef_var = 1;


/*
 * In an array of unknown size that is declared using fewer braces than
 * recommended, ensure that the array size is updated at the end of the
 * initializer.
 */
struct {
	int x;
	int y;
} points_of_unknown_size[] = {
	3, 4,
};

void
init_string_via_assignment(void)
{
	const char *cs_match = "";
	const int *ws_match = L"";

	/* expect+1: warning: illegal combination of 'pointer to const char' and 'pointer to int', op 'init' [124] */
	const char *cs_mismatch = L"";
	/* expect+1: warning: illegal combination of 'pointer to const int' and 'pointer to char', op 'init' [124] */
	const int *ws_mismatch = "";
}

void
init_pointer_in_struct(void)
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
init_array_in_struct(void)
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
