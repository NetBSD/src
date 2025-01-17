// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to add into tail of a linked-list safely
 * against concurrent RCU traversals.
 */

#include <stdio.h>

#include <urcu/urcu-memb.h>	/* Userspace RCU flavor */
#include <urcu/rculist.h>	/* RCU list */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */

/*
 * Nodes populated into the list.
 */
struct mynode {
	int value;			/* Node content */
	struct cds_list_head node;	/* Linked-list chaining */
};

int main(void)
{
	int values[] = { -5, 42, 36, 24, };
	CDS_LIST_HEAD(mylist);		/* Defines an empty list head */
	unsigned int i;
	int ret = 0;
	struct mynode *node;

	/*
	 * Adding nodes to the linked-list. Safe against concurrent
	 * RCU traversals, require mutual exclusion with list updates.
	 */
	for (i = 0; i < CAA_ARRAY_SIZE(values); i++) {
		node = malloc(sizeof(*node));
		if (!node) {
			ret = -1;
			goto end;
		}
		node->value = values[i];
		cds_list_add_tail_rcu(&node->node, &mylist);
	}

	/*
	 * Just show the list content. This is _not_ an RCU-safe
	 * iteration on the list.
	 */
	printf("mylist content:");
	cds_list_for_each_entry(node, &mylist, node) {
		printf(" %d", node->value);
	}
	printf("\n");
end:
	return ret;
}
