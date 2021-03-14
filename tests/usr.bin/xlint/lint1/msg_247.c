/*	$NetBSD: msg_247.c,v 1.4 2021/03/14 21:44:35 rillig Exp $	*/
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
	PDisplay display = (PDisplay)arg;	/* expect: 247 */
}
