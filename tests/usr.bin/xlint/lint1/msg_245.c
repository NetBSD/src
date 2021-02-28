/*	$NetBSD: msg_245.c,v 1.3 2021/02/28 01:53:49 rillig Exp $	*/
# 3 "msg_245.c"

// Test for message: illegal structure pointer combination, op %s [245]

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
	sink_bool(&both == &only_tag);		/* expect: 245 */
	sink_bool(&both == &only_typedef);	/* expect: 245 */
	sink_bool(&both == &unnamed);		/* expect: 245 */
}
