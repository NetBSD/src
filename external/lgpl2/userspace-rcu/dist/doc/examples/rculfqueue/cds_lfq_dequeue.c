// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to dequeue nodes from a RCU lock-free queue.
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
	struct rcu_head rcu_head;	/* For call_rcu() */
};

static
void free_node(struct rcu_head *head)
{
	struct mynode *node =
		caa_container_of(head, struct mynode, rcu_head);

	free(node);
}

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

	/*
	 * Dequeue each node from the queue. Those will be dequeued from
	 * the oldest (first enqueued) to the newest (last enqueued).
	 */
	printf("dequeued content:");
	for (;;) {
		struct cds_lfq_node_rcu *qnode;
		struct mynode *node;

		/*
		 * Both enqueue and dequeue need to be called within RCU
		 * read-side critical section.
		 */
		urcu_memb_read_lock();
		qnode = cds_lfq_dequeue_rcu(&myqueue);
		urcu_memb_read_unlock();
		if (!qnode) {
			break;	/* Queue is empty. */
		}
		/* Getting the container structure from the node */
		node = caa_container_of(qnode, struct mynode, node);
		printf(" %d", node->value);
		urcu_memb_call_rcu(&node->rcu_head, free_node);
	}
	printf("\n");
	/*
	 * Release memory used by the queue.
	 */
	ret = cds_lfq_destroy_rcu(&myqueue);
	if (ret) {
		printf("Error destroying queue (non-empty)\n");
	}
end:
	urcu_memb_unregister_thread();
	return ret;
}
