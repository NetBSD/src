// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to replace a node within a linked-list safely
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
	struct rcu_head rcu_head;	/* For call_rcu() */
};

static
void free_node_rcu(struct rcu_head *head)
{
	struct mynode *node = caa_container_of(head, struct mynode, rcu_head);

	free(node);
}

int main(void)
{
	int values[] = { -5, 42, 36, 24, };
	CDS_LIST_HEAD(mylist);		/* Defines an empty list head */
	unsigned int i;
	int ret = 0;
	struct mynode *node, *n;

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
	 * Replacing all values by their negated value. Safe against
	 * concurrent RCU traversals, require mutual exclusion with list
	 * updates. Notice the "safe" iteration: it is safe against
	 * removal (and thus replacement) of nodes as we iterate on the
	 * list.
	 */
	cds_list_for_each_entry_safe(node, n, &mylist, node) {
		struct mynode *new_node;

		new_node = malloc(sizeof(*node));
		if (!new_node) {
			ret = -1;
			goto end;
		}
		/* Replacement node value is negated original value. */
		new_node->value = -node->value;
		cds_list_replace_rcu(&node->node, &new_node->node);
		urcu_memb_call_rcu(&node->rcu_head, free_node_rcu);
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
