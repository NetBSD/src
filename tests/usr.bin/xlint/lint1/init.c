/*	$NetBSD: init.c,v 1.6 2021/12/17 10:51:45 rillig Exp $	*/
# 3 "init.c"

/*
 * Tests for initialization.
 *
 * C99 6.7.8
 */

/*
 * C99 does not allow empty initializer braces syntactically.
 * Lint allows this syntactically, it just complains if the resulting
 * object is empty.
 */
/* expect+1: error: empty array declaration: empty_array_with_initializer [190] */
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
 *
 * TODO: Properly handle this situation; as of init.c 1.214 from 2021-12-17,
 *  the below initialization sets in->in_err but shouldn't.
 */
const histogram_entry hgr[] = {
	"odd", 5,
	"even", 5,
};


/*
 * Initialization with fewer braces than usual, must still be accepted.
 *
 * TODO: Properly handle this situation; as of init.c 1.214 from 2021-12-17,
 *  the below initialization sets in->in_err but shouldn't.
 */
struct {
	int x, y;
} points[] = {
	0, 0, 3, 0, 0, 4, 3, 4
};


/*
 * Initialization with fewer braces than usual, must still be accepted.
 *
 * TODO: Properly handle this situation; as of init.c 1.214 from 2021-12-17,
 *  the below initialization sets in->in_err but shouldn't.
 */
void do_nothing(void);

struct {
	void (*action_1) (void);
	void (*action_2) (void);
} actions[1] = {
	/* expect+1: error: cannot initialize 'struct <unnamed>' from 'pointer to function(void) returning void' [185] */
	do_nothing,
	/* expect+2: error: too many array initializers, expected 1 [173] */
	/* expect+1: error: cannot initialize 'struct <unnamed>' from 'pointer to function(void) returning void' [185] */
	do_nothing,
};
