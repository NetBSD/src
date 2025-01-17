// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <urcu/urcu-bp.h>	/* Bulletproof RCU flavor */
#include <urcu/rculist.h>	/* List example */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */

/*
 * Example showing how to use the Bulletproof Userspace RCU flavor.
 *
 * This is a mock-up example where updates and RCU traversals are
 * performed by the same thread to keep things simple on purpose.
 */

static CDS_LIST_HEAD(mylist);

struct mynode {
	uint64_t value;
	struct cds_list_head node;	/* linked-list chaining */
	struct rcu_head rcu_head;	/* for call_rcu() */
};

static
int add_node(uint64_t v)
{
	struct mynode *node;

	node = calloc(1, sizeof(*node));
	if (!node)
		return -1;
	node->value = v;
	cds_list_add_rcu(&node->node, &mylist);
	return 0;
}

int main(void)
{
	uint64_t values[] = { 42, 36, 24, };
	unsigned int i;
	int ret;
	struct mynode *node, *n;

	/*
	 * Notice that with the bulletproof flavor, there is no need to
	 * register/unregister RCU reader threads.
	 */

	/*
	 * Adding nodes to the linked-list. Safe against concurrent
	 * RCU traversals, require mutual exclusion with list updates.
	 */
	for (i = 0; i < CAA_ARRAY_SIZE(values); i++) {
		ret = add_node(values[i]);
		if (ret)
			goto end;
	}

	/*
	 * We need to explicitly mark RCU read-side critical sections
	 * with rcu_read_lock() and rcu_read_unlock(). They can be
	 * nested. Those are no-ops for the QSBR flavor.
	 */
	urcu_bp_read_lock();

	/*
	 * RCU traversal of the linked list.
	 */
	cds_list_for_each_entry_rcu(node, &mylist, node) {
		printf("Value: %" PRIu64 "\n", node->value);
	}
	urcu_bp_read_unlock();

	/*
	 * Removing nodes from linked list. Safe against concurrent RCU
	 * traversals, require mutual exclusion with list updates.
	 */
	cds_list_for_each_entry_safe(node, n, &mylist, node) {
		cds_list_del_rcu(&node->node);

		/*
		 * Using synchronize_rcu() directly for synchronization
		 * so we keep a minimal impact on the system by not
		 * spawning any call_rcu() thread. It is slower though,
		 * since there is no batching.
		 */
		urcu_bp_synchronize_rcu();
		free(node);
	}

	sleep(1);

end:
	return ret;
}
