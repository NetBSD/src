// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to do a non-circular RCU linked list
 * traversal, safely against concurrent RCU updates.
 * cds_hlist_for_each_rcu() iterates on struct cds_hlist_node, and thus,
 * either caa_container_of() or cds_hlist_entry() are needed to access
 * the container structure.
 */

#include <stdio.h>

#include <urcu/urcu-memb.h>	/* Userspace RCU flavor */
#include <urcu/rcuhlist.h>	/* RCU hlist */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */

/*
 * Nodes populated into the list.
 */
struct mynode {
	int value;			/* Node content */
	struct cds_hlist_node node;	/* Linked-list chaining */
};

int main(void)
{
	int values[] = { -5, 42, 36, 24, };
	CDS_HLIST_HEAD(mylist);		/* Defines an empty hlist head */
	unsigned int i;
	int ret = 0;
	struct cds_hlist_node *pos;

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
		cds_hlist_add_head_rcu(&node->node, &mylist);
	}

	/*
	 * RCU-safe iteration on the list.
	 */
	printf("mylist content:");

	/*
	 * Surround the RCU read-side critical section with rcu_read_lock()
	 * and rcu_read_unlock().
	 */
	urcu_memb_read_lock();

	/*
	 * This traversal can be performed concurrently with RCU
	 * updates.
	 */
	cds_hlist_for_each_rcu(pos, &mylist) {
		struct mynode *node = cds_hlist_entry(pos, struct mynode, node);

		printf(" %d", node->value);
	}

	urcu_memb_read_unlock();

	printf("\n");
end:
	urcu_memb_unregister_thread();
	return ret;
}
