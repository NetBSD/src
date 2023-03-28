/*	$NetBSD: msg_245.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_245.c"

// Test for message: incompatible structure pointers: '%s' '%s' '%s' [245]

/* lint1-extra-flags: -X 351 */

typedef struct tag_and_typedef_tag {
	int member;
} tag_and_typedef_typedef;

struct only_tag {
	int member;
};

typedef struct {
	int member;
} only_typedef;

struct {
	int member;
} unnamed;

void sink_bool(_Bool);

void
example(tag_and_typedef_typedef both,
	only_typedef only_typedef,
	struct only_tag only_tag)
{
	/* expect+1: warning: incompatible structure pointers: 'pointer to struct tag_and_typedef_tag' '==' 'pointer to struct only_tag' [245] */
	sink_bool(&both == &only_tag);
	/* expect+1: warning: incompatible structure pointers: 'pointer to struct tag_and_typedef_tag' '==' 'pointer to struct typedef only_typedef' [245] */
	sink_bool(&both == &only_typedef);
	/* expect+1: warning: incompatible structure pointers: 'pointer to struct tag_and_typedef_tag' '==' 'pointer to struct <unnamed>' [245] */
	sink_bool(&both == &unnamed);
}
