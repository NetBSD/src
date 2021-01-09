/*	$NetBSD: msg_108.c,v 1.2 2021/01/09 17:02:19 rillig Exp $	*/
# 3 "msg_108.c"

// Test for message: operand of '%s' has incompatible type (%s != %s) [108]

TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message.";

struct s {
	int member;
};

void
example(void)
{
	struct s s;

	// FIXME: msg_108.c(14): lint error: common/tyname.c, 190: tspec_name(0)
	// basic_type_name (t=NOTSPEC)
	// warn_incompatible_types (op=COMPL, lt=STRUCT, rt=NOTSPEC)
	//s = ~s;
}
