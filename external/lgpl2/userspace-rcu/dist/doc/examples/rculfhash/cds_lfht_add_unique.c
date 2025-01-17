// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * This example shows how to add unique nodes into a RCU lock-free hash
 * table. We use a "seqnum" field to show which node is staying in the
 * hash table. This hash table requires using a RCU scheme.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <urcu/urcu-memb.h>	/* RCU flavor */
#include <urcu/rculfhash.h>	/* RCU Lock-free hash table */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */
#include "jhash.h"		/* Example hash function */

/*
 * Nodes populated into the hash table.
 */
struct mynode {
	int value;			/* Node content */
	int seqnum;			/* Our node sequence number */
	struct cds_lfht_node node;	/* Chaining in hash table */
};

static
int match(struct cds_lfht_node *ht_node, const void *_key)
{
	struct mynode *node =
		caa_container_of(ht_node, struct mynode, node);
	const int *key = _key;

	return *key == node->value;
}

int main(void)
{
	int values[] = { -5, 42, 42, 36, 24, };	/* 42 is duplicated */
	struct cds_lfht *ht;	/* Hash table */
	unsigned int i;
	int ret = 0, seqnum = 0;
	uint32_t seed;
	struct cds_lfht_iter iter;	/* For iteration on hash table */
	struct cds_lfht_node *ht_node;
	struct mynode *node;

	/*
	 * Each thread need using RCU read-side need to be explicitly
	 * registered.
	 */
	urcu_memb_register_thread();

	/* Use time as seed for hash table hashing. */
	seed = (uint32_t) time(NULL);

	/*
	 * Allocate hash table.
	 */
	ht = cds_lfht_new_flavor(1, 1, 0,
		CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING,
		&urcu_memb_flavor, NULL);
	if (!ht) {
		printf("Error allocating hash table\n");
		ret = -1;
		goto end;
	}

	/*
	 * Add nodes to hash table.
	 */
	for (i = 0; i < CAA_ARRAY_SIZE(values); i++) {
		unsigned long hash;
		int value;

		node = malloc(sizeof(*node));
		if (!node) {
			ret = -1;
			goto end;
		}

		cds_lfht_node_init(&node->node);
		value = values[i];
		node->value = value;
		node->seqnum = seqnum++;
		hash = jhash(&value, sizeof(value), seed);

		/*
		 * cds_lfht_add() needs to be called from RCU read-side
		 * critical section.
		 */
		urcu_memb_read_lock();
		ht_node = cds_lfht_add_unique(ht, hash, match, &value,
			&node->node);
		/*
		 * cds_lfht_add_unique() returns the added node if not
		 * already present, else returns the node matching the
		 * key.
		 */
		if (ht_node != &node->node) {
			/*
			 * We found a match. Therefore, to ensure we
			 * don't add duplicates, cds_lfht_add_unique()
			 * acted as a RCU lookup, returning the found
			 * match. It did not add the new node to the
			 * hash table, so we can free it on the spot.
			 */
			printf("Not adding duplicate (key: %d, seqnum: %d)\n",
				node->value, node->seqnum);
			free(node);
		} else {
			printf("Add (key: %d, seqnum: %d)\n",
				node->value, node->seqnum);
		}
		urcu_memb_read_unlock();
	}

	/*
	 * Iterate over each hash table node. Those will appear in
	 * random order, depending on the hash seed. Iteration needs to
	 * be performed within RCU read-side critical section.
	 */
	printf("hash table content (random order):");
	urcu_memb_read_lock();
	cds_lfht_for_each_entry(ht, &iter, node, node) {
		printf(" (key: %d, seqnum: %d)",
			node->value, node->seqnum);
	}
	urcu_memb_read_unlock();
	printf("\n");

end:
	urcu_memb_unregister_thread();
	return ret;
}
