// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to enqueue nodes into a RCU lock-free queue.
 * This queue requires using a RCU scheme.
 */

#include <stdio.h>
#include <stdlib.h>

#include <urcu/urcu-memb.h>	/* RCU flavor */
#include <urcu/rculfqueue.h>	/* RCU Lock-free queue */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */

/*
 * Nodes populated into the queue.
 */
struct mynode {
	int value;			/* Node content */
	struct cds_lfq_node_rcu node;	/* Chaining in queue */
};

int main(void)
{
	int values[] = { -5, 42, 36, 24, };
	struct cds_lfq_queue_rcu myqueue;	/* Queue */
	unsigned int i;
	int ret = 0;

	/*
	 * Each thread need using RCU read-side need to be explicitly
	 * registered.
	 */
	urcu_memb_register_thread();

	cds_lfq_init_rcu(&myqueue, urcu_memb_call_rcu);

	/*
	 * Enqueue nodes.
	 */
	for (i = 0; i < CAA_ARRAY_SIZE(values); i++) {
		struct mynode *node;

		node = malloc(sizeof(*node));
		if (!node) {
			ret = -1;
			goto end;
		}

		cds_lfq_node_init_rcu(&node->node);
		node->value = values[i];
		/*
		 * Both enqueue and dequeue need to be called within RCU
		 * read-side critical section.
		 */
		urcu_memb_read_lock();
		cds_lfq_enqueue_rcu(&myqueue, &node->node);
		urcu_memb_read_unlock();
	}

end:
	urcu_memb_unregister_thread();
	return ret;
}
