/*	$NetBSD: msg_247.c,v 1.5 2021/03/14 22:24:24 rillig Exp $	*/
# 3 "msg_247.c"

// Test for message: pointer cast from '%s' to '%s' may be troublesome [247]

/* lint1-extra-flags: -c */

/* example taken from Xlib.h */
typedef struct {
	int id;
} *PDisplay;

struct Other {
	int id;
};

void
example(struct Other *arg)
{
	PDisplay display;

	/*
	 * XXX: The target type is reported as 'struct <unnamed>'.  In cases
	 *  like these, it would be helpful to print at least the type name
	 *  of the pointer.  This type name though is discarded immediately
	 *  when the parser reduces 'T_TYPENAME clrtyp' to 'clrtyp_typespec'.
	 *  After that, the target type of the cast is just an unnamed struct,
	 *  with no hint at all that there is a typedef for a pointer to the
	 *  struct.
	 */
	display = (PDisplay)arg;	/* expect: 247 */
}
