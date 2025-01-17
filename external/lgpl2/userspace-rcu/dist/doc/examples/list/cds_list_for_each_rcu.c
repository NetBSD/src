// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to do a RCU linked list traversal, safely
 * against concurrent RCU updates. cds_list_for_each_rcu() iterates on
 * struct cds_list_head, and thus, either caa_container_of() or
 * cds_list_entry() are needed to access the container structure.
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
	struct cds_list_head *pos;

	/*
	 * Each thread need using RCU read-side need to be explicitly
	 * registered.
	 */
	urcu_memb_register_thread();

	/*
	 * Adding nodes to the linked-list. Safe against concurrent
	 * RCU traversals, require mutual exclusion with list updates.
	 */
	for (i = 0; i < CAA_ARRAY_SIZE(values); i++) {
		struct mynode *node;

		node = malloc(sizeof(*node));
		if (!node) {
			ret = -1;
			goto end;
		}
		node->value = values[i];
		cds_list_add_tail_rcu(&node->node, &mylist);
	}

	/*
	 * RCU-safe iteration on the list.
	 */
	printf("mylist content:");

	/*
	 * Surround the RCU read-side critical section with urcu_memb_read_lock()
	 * and urcu_memb_read_unlock().
	 */
	urcu_memb_read_lock();

	/*
	 * This traversal can be performed concurrently with RCU
	 * updates.
	 */
	cds_list_for_each_rcu(pos, &mylist) {
		struct mynode *node = cds_list_entry(pos, struct mynode, node);

		printf(" %d", node->value);
	}

	urcu_memb_read_unlock();

	printf("\n");
end:
	urcu_memb_unregister_thread();
	return ret;
}
