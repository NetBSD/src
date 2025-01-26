/*	$NetBSD: rbt.c,v 1.17 2025/01/26 16:25:24 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <isc/crc64.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/refcount.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/util.h>

/*%
 * This define is so dns/name.h (included by dns/fixedname.h) uses more
 * efficient macro calls instead of functions for a few operations.
 */
#include <unistd.h>

#include <isc/result.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/rbt.h>

#define CHECK(x)                             \
	do {                                 \
		result = (x);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

#define RBT_MAGIC      ISC_MAGIC('R', 'B', 'T', '+')
#define VALID_RBT(rbt) ISC_MAGIC_VALID(rbt, RBT_MAGIC)

/*
 * XXXDCL Since parent pointers were added in again, I could remove all of the
 * chain junk, and replace with dns_rbt_firstnode, _previousnode, _nextnode,
 * _lastnode.  This would involve pretty major change to the API.
 */
#define CHAIN_MAGIC	   ISC_MAGIC('0', '-', '0', '-')
#define VALID_CHAIN(chain) ISC_MAGIC_VALID(chain, CHAIN_MAGIC)

#define RBT_HASH_NEXTTABLE(hindex) ((hindex == 0) ? 1 : 0)

struct dns_rbt {
	unsigned int magic;
	isc_mem_t *mctx;
	dns_rbtnode_t *root;
	void (*data_deleter)(void *, void *);
	void *deleter_arg;
	unsigned int nodecount;
	uint8_t hashbits[2];
	dns_rbtnode_t **hashtable[2];
	uint8_t hindex;
	uint32_t hiter;
};

#define IS_EMPTY(node) ((node)->data == NULL)

#define WANTEMPTYDATA_OR_DATA(options, node) \
	((options & DNS_RBTFIND_EMPTYDATA) != 0 || node->data != NULL)

/*%
 * The variable length stuff stored after the node has the following
 * structure.
 *
 *	NAME_DATA{1..255} OLDOFFSETLEN{1} OFFSETS{1..128}
 *
 * NAME_DATA contains the name of the node when it was created.
 * OLDOFFSETLEN contains the length of OFFSETS when the node was created.
 * OFFSETS contains the offsets into name for each label when the node
 * was created.
 */

#define NAME(node)	   ((unsigned char *)((node) + 1))
#define OFFSETS(node)	   (NAME(node) + node->oldnamelen + 1)
#define OLDOFFSETLEN(node) (OFFSETS(node)[-1])

#define NODE_SIZE(node) \
	(sizeof(*node) + node->oldnamelen + OLDOFFSETLEN(node) + 1)

/*%
 * Color management.
 */
#define RED	       0
#define BLACK	       1
#define IS_RED(node)   ((node) != NULL && (node)->color == RED)
#define IS_BLACK(node) ((node) == NULL || (node)->color == BLACK)

/*%
 * Chain management.
 *
 * The "ancestors" member of chains were removed, with their job now
 * being wholly handled by parent pointers (which didn't exist, because
 * of memory concerns, when chains were first implemented).
 */
#define ADD_LEVEL(chain, node)                                     \
	do {                                                       \
		INSIST((chain)->level_count < DNS_RBT_LEVELBLOCK); \
		(chain)->levels[(chain)->level_count++] = (node);  \
	} while (0)

/*
 * Initialize a dns_name_t that refers to a node's name.
 */
static void
node_name(dns_rbtnode_t *node, dns_name_t *name) {
	name->length = node->namelen;
	name->labels = node->offsetlen;
	name->ndata = NAME(node);
	name->offsets = OFFSETS(node);
	name->attributes = (struct dns_name_attrs){ .absolute = node->absolute,
						    .readonly = true };
}

#ifdef DEBUG
/*
 * A little something to help out in GDB.
 */
dns_name_t
Name(dns_rbtnode_t *node);
dns_name_t
Name(dns_rbtnode_t *node) {
	dns_name_t name;

	dns_name_init(&name, NULL);
	if (node != NULL) {
		node_name(node, &name);
	}

	return name;
}
#endif /* DEBUG */

/*
 * Upper node is the parent of the root of the passed node's
 * subtree. The passed node must not be NULL.
 */
static dns_rbtnode_t *
get_upper_node(dns_rbtnode_t *node) {
	return node->uppernode;
}

size_t
dns__rbtnode_getdistance(dns_rbtnode_t *node) {
	size_t nodes = 1;

	while (node != NULL) {
		if (node->is_root) {
			break;
		}
		nodes++;
		node = node->parent;
	}

	return nodes;
}

/*
 * Forward declarations.
 */
static dns_rbtnode_t *
rbtnode_new(isc_mem_t *mctx, const dns_name_t *name);

static void
hashtable_new(dns_rbt_t *rbt, uint8_t index, uint8_t bits);
static void
hashtable_free(dns_rbt_t *rbt, uint8_t index);

static void
hash_node(dns_rbt_t *rbt, dns_rbtnode_t *node, const dns_name_t *name);

static void
unhash_node(dns_rbt_t *rbt, dns_rbtnode_t *node);

static uint32_t
rehash_bits(dns_rbt_t *rbt, size_t newcount);
static void
hashtable_rehash(dns_rbt_t *rbt, uint32_t newbits);
static void
hashtable_rehash_one(dns_rbt_t *rbt);
static void
maybe_rehash(dns_rbt_t *rbt, size_t size);
static bool
rehashing_in_progress(dns_rbt_t *rbt);

#define TRY_NEXTTABLE(hindex, rbt) \
	(hindex == rbt->hindex && rehashing_in_progress(rbt))

static void
rotate_left(dns_rbtnode_t *node, dns_rbtnode_t **rootp);
static void
rotate_right(dns_rbtnode_t *node, dns_rbtnode_t **rootp);

static void
addonlevel(dns_rbtnode_t *node, dns_rbtnode_t *current, int order,
	   dns_rbtnode_t **rootp);

static void
deletefromlevel(dns_rbtnode_t *item, dns_rbtnode_t **rootp);

static void
deletetreeflat(dns_rbt_t *rbt, unsigned int quantum, bool unhash,
	       dns_rbtnode_t **nodep);

static void
printnodename(dns_rbtnode_t *node, bool quoted, FILE *f);

static void
freenode(dns_rbt_t *rbt, dns_rbtnode_t **nodep);

unsigned int
dns__rbtnode_namelen(dns_rbtnode_t *node) {
	dns_name_t current;
	unsigned int len = 0;

	REQUIRE(DNS_RBTNODE_VALID(node));

	dns_name_init(&current, NULL);

	do {
		if (node != NULL) {
			node_name(node, &current);
			len += current.length;
		} else {
			len += 1;
			break;
		}

		node = get_upper_node(node);
	} while (!dns_name_isabsolute(&current));

	return len;
}

unsigned int
dns__rbtnode_getsize(dns_rbtnode_t *node) {
	REQUIRE(DNS_RBTNODE_VALID(node));

	return NODE_SIZE(node);
}

/*
 * Initialize a red/black tree of trees.
 */
isc_result_t
dns_rbt_create(isc_mem_t *mctx, dns_rbtdeleter_t deleter, void *deleter_arg,
	       dns_rbt_t **rbtp) {
	dns_rbt_t *rbt;

	REQUIRE(mctx != NULL);
	REQUIRE(rbtp != NULL && *rbtp == NULL);
	REQUIRE(deleter == NULL ? deleter_arg == NULL : 1);

	rbt = isc_mem_get(mctx, sizeof(*rbt));
	*rbt = (dns_rbt_t){
		.data_deleter = deleter,
		.deleter_arg = deleter_arg,
	};

	isc_mem_attach(mctx, &rbt->mctx);

	hashtable_new(rbt, 0, ISC_HASH_MIN_BITS);

	rbt->magic = RBT_MAGIC;

	*rbtp = rbt;

	return ISC_R_SUCCESS;
}

/*
 * Deallocate a red/black tree of trees.
 */
isc_result_t
dns_rbt_destroy(dns_rbt_t **rbtp, unsigned int quantum) {
	dns_rbt_t *rbt;

	REQUIRE(rbtp != NULL && VALID_RBT(*rbtp));

	rbt = *rbtp;

	deletetreeflat(rbt, quantum, false, &rbt->root);
	if (rbt->root != NULL) {
		return ISC_R_QUOTA;
	}

	*rbtp = NULL;

	INSIST(rbt->nodecount == 0);

	if (rbt->hashtable[0] != NULL) {
		hashtable_free(rbt, 0);
	}
	if (rbt->hashtable[1] != NULL) {
		hashtable_free(rbt, 1);
	}

	rbt->magic = 0;

	isc_mem_putanddetach(&rbt->mctx, rbt, sizeof(*rbt));
	return ISC_R_SUCCESS;
}

unsigned int
dns_rbt_nodecount(dns_rbt_t *rbt) {
	REQUIRE(VALID_RBT(rbt));

	return rbt->nodecount;
}

size_t
dns_rbt_hashsize(dns_rbt_t *rbt) {
	REQUIRE(VALID_RBT(rbt));

	uint8_t hashbits = (rbt->hashbits[0] > rbt->hashbits[1])
				   ? rbt->hashbits[0]
				   : rbt->hashbits[1];

	return 1 << hashbits;
}

static isc_result_t
chain_name(dns_rbtnodechain_t *chain, dns_name_t *name,
	   bool include_chain_end) {
	dns_name_t nodename;
	isc_result_t result = ISC_R_SUCCESS;
	int i;

	dns_name_init(&nodename, NULL);

	if (include_chain_end && chain->end != NULL) {
		node_name(chain->end, &nodename);
		dns_name_copy(&nodename, name);
	} else {
		dns_name_reset(name);
	}

	for (i = (int)chain->level_count - 1; i >= 0; i--) {
		node_name(chain->levels[i], &nodename);
		result = dns_name_concatenate(name, &nodename, name, NULL);

		if (result != ISC_R_SUCCESS) {
			return result;
		}
	}
	return result;
}

static isc_result_t
move_chain_to_last(dns_rbtnodechain_t *chain, dns_rbtnode_t *node) {
	do {
		/*
		 * Go as far right and then down as much as possible,
		 * as long as the rightmost node has a down pointer.
		 */
		while (node->right != NULL) {
			node = node->right;
		}

		if (node->down == NULL) {
			break;
		}

		ADD_LEVEL(chain, node);
		node = node->down;
	} while (1);

	chain->end = node;

	return ISC_R_SUCCESS;
}

/*
 * Add 'name' to tree, initializing its data pointer with 'data'.
 */

isc_result_t
dns_rbt_addnode(dns_rbt_t *rbt, const dns_name_t *name, dns_rbtnode_t **nodep) {
	/*
	 * Does this thing have too many variables or what?
	 */
	dns_rbtnode_t **root, *parent, *child, *current, *new_current;
	dns_name_t *add_name, *new_name, current_name, *prefix, *suffix;
	dns_fixedname_t fixedcopy, fixedprefix, fixedsuffix, fnewname;
	dns_offsets_t current_offsets;
	dns_namereln_t compared;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int level_count;
	unsigned int common_labels;
	unsigned int nlabels, hlabels;
	int order;

	REQUIRE(VALID_RBT(rbt));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(nodep != NULL && *nodep == NULL);

	/*
	 * Dear future BIND developer,
	 *
	 * After you have tried attempting to optimize this routine by
	 * using the hashtable and have realized your folly, please
	 * append another cross ("X") below as a warning to the next
	 * future BIND developer:
	 *
	 * Number of victim developers: X
	 *
	 * I wish the past developer had included such a notice.
	 *
	 * Long form: Unlike dns_rbt_findnode(), this function does not
	 * lend itself to be optimized using the hashtable:
	 *
	 * 1. In the subtree where the insertion occurs, this function
	 * needs to have the insertion point and the order where the
	 * lookup terminated (i.e., at the insertion point where left or
	 * right child is NULL). This cannot be determined from the
	 * hashtable, so at least in that subtree, a BST O(log N) lookup
	 * is necessary.
	 *
	 * 2. Our RBT nodes contain not only single labels but label
	 * sequences to optimize space usage. So at every level, we have
	 * to look for a match in the hashtable for all superdomains in
	 * the rest of the name we're searching. This is an O(N)
	 * operation at least, here N being the label size of name, each
	 * of which is a hashtable lookup involving dns_name_equal()
	 * comparisons.
	 */

	/*
	 * Create a copy of the name so the original name structure is
	 * not modified.
	 */
	add_name = dns_fixedname_initname(&fixedcopy);
	INSIST(add_name != NULL);
	dns_name_clone(name, add_name);

	if (rbt->root == NULL) {
		new_current = rbtnode_new(rbt->mctx, add_name);
		rbt->nodecount++;
		new_current->is_root = 1;
		new_current->uppernode = NULL;
		rbt->root = new_current;
		*nodep = new_current;
		hash_node(rbt, new_current, name);
		return ISC_R_SUCCESS;
	}

	level_count = 0;

	prefix = dns_fixedname_initname(&fixedprefix);
	suffix = dns_fixedname_initname(&fixedsuffix);

	INSIST(prefix != NULL);
	INSIST(suffix != NULL);

	root = &rbt->root;
	INSIST((*root)->is_root);
	parent = NULL;
	current = NULL;
	child = *root;
	dns_name_init(&current_name, current_offsets);
	new_name = dns_fixedname_initname(&fnewname);
	nlabels = dns_name_countlabels(name);
	hlabels = 0;

	do {
		current = child;

		node_name(current, &current_name);
		compared = dns_name_fullcompare(add_name, &current_name, &order,
						&common_labels);

		if (compared == dns_namereln_equal) {
			*nodep = current;
			result = ISC_R_EXISTS;
			break;
		}

		if (compared == dns_namereln_none) {
			if (order < 0) {
				parent = current;
				child = current->left;
			} else if (order > 0) {
				parent = current;
				child = current->right;
			}
		} else {
			/*
			 * This name has some suffix in common with the
			 * name at the current node.  If the name at
			 * the current node is shorter, that means the
			 * new name should be in a subtree.  If the
			 * name at the current node is longer, that means
			 * the down pointer to this tree should point
			 * to a new tree that has the common suffix, and
			 * the non-common parts of these two names should
			 * start a new tree.
			 */
			hlabels += common_labels;
			if (compared == dns_namereln_subdomain) {
				/*
				 * All of the existing labels are in common,
				 * so the new name is in a subtree.
				 * Whack off the common labels for the
				 * not-in-common part to be searched for
				 * in the next level.
				 */
				dns_name_split(add_name, common_labels,
					       add_name, NULL);

				/*
				 * Follow the down pointer (possibly NULL).
				 */
				root = &current->down;

				INSIST(*root == NULL ||
				       ((*root)->is_root &&
					(*root)->parent == current));

				parent = NULL;
				child = current->down;

				INSIST(level_count < DNS_RBT_LEVELBLOCK);
				level_count++;
			} else {
				/*
				 * The number of labels in common is fewer
				 * than the number of labels at the current
				 * node, so the current node must be adjusted
				 * to have just the common suffix, and a down
				 * pointer made to a new tree.
				 */

				INSIST(compared ==
					       dns_namereln_commonancestor ||
				       compared == dns_namereln_contains);

				/*
				 * Ensure the number of levels in the tree
				 * does not exceed the number of logical
				 * levels allowed by DNSSEC.
				 *
				 * XXXDCL need a better error result?
				 */
				if (level_count >= DNS_RBT_LEVELBLOCK) {
					result = ISC_R_NOSPACE;
					break;
				}

				/*
				 * Split the name into two parts, a prefix
				 * which is the not-in-common parts of the
				 * two names and a suffix that is the common
				 * parts of them.
				 */
				dns_name_split(&current_name, common_labels,
					       prefix, suffix);
				new_current = rbtnode_new(rbt->mctx, suffix);

				/*
				 * Reproduce the tree attributes of the
				 * current node.
				 */
				new_current->is_root = current->is_root;
				if (current->nsec == DNS_DB_NSEC_HAS_NSEC) {
					new_current->nsec = DNS_DB_NSEC_NORMAL;
				} else {
					new_current->nsec = current->nsec;
				}
				new_current->parent = current->parent;
				new_current->left = current->left;
				new_current->right = current->right;
				new_current->color = current->color;

				/*
				 * Fix pointers that were to the current node.
				 */
				if (parent != NULL) {
					if (parent->left == current) {
						parent->left = new_current;
					} else {
						parent->right = new_current;
					}
				}
				if (new_current->left != NULL) {
					new_current->left->parent = new_current;
				}
				if (new_current->right != NULL) {
					new_current->right->parent =
						new_current;
				}
				if (*root == current) {
					*root = new_current;
				}

				current->namelen = prefix->length;
				current->offsetlen = prefix->labels;

				/*
				 * Set up the new root of the next level.
				 * By definition it will not be the top
				 * level tree, so clear the absolute flag.
				 */
				current->is_root = 1;
				current->parent = new_current;
				new_current->down = current;
				root = &new_current->down;

				new_current->uppernode = current->uppernode;
				current->uppernode = new_current;

				INSIST(level_count < DNS_RBT_LEVELBLOCK);
				level_count++;

				current->left = NULL;
				current->right = NULL;

				current->color = BLACK;
				current->absolute = false;

				rbt->nodecount++;
				dns_name_getlabelsequence(name,
							  nlabels - hlabels,
							  hlabels, new_name);
				hash_node(rbt, new_current, new_name);

				if (common_labels ==
				    dns_name_countlabels(add_name))
				{
					/*
					 * The name has been added by pushing
					 * the not-in-common parts down to
					 * a new level.
					 */
					*nodep = new_current;
					return ISC_R_SUCCESS;
				} else {
					/*
					 * The current node has no data,
					 * because it is just a placeholder.
					 * Its data pointer is already NULL
					 * from rbtnode_new()), so there's
					 * nothing more to do to it.
					 *
					 * The not-in-common parts of the new
					 * name will be inserted into the new
					 * level following this loop.
					 */
					dns_name_split(add_name, common_labels,
						       add_name, NULL);
					result = ISC_R_SUCCESS;
					break;
				}
			}
		}
	} while (child != NULL);

	if (result == ISC_R_SUCCESS) {
		new_current = rbtnode_new(rbt->mctx, add_name);
	}

	if (result == ISC_R_SUCCESS) {
		if (*root == NULL) {
			new_current->uppernode = current;
		} else {
			new_current->uppernode = (*root)->parent;
		}

		addonlevel(new_current, current, order, root);
		rbt->nodecount++;
		*nodep = new_current;
		hash_node(rbt, new_current, name);
	}

	return result;
}

/*
 * Find the node for "name" in the tree of trees.
 */
isc_result_t
dns__rbt_findnode(dns_rbt_t *rbt, const dns_name_t *name, dns_name_t *foundname,
		  dns_rbtnode_t **node, dns_rbtnodechain_t *chain,
		  unsigned int options, dns_rbtfindcallback_t callback,
		  void *callback_arg DNS__DB_FLARG) {
	dns_rbtnode_t *current, *last_compared;
	dns_rbtnodechain_t localchain;
	dns_name_t *search_name, current_name, *callback_name;
	dns_fixedname_t fixedcallbackname, fixedsearchname;
	dns_namereln_t compared;
	isc_result_t result, saved_result;
	unsigned int common_labels;
	unsigned int hlabels = 0;
	int order;
	uint8_t hindex;

	REQUIRE(VALID_RBT(rbt));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(node != NULL && *node == NULL);
	REQUIRE((options & (DNS_RBTFIND_NOEXACT | DNS_RBTFIND_NOPREDECESSOR)) !=
		(DNS_RBTFIND_NOEXACT | DNS_RBTFIND_NOPREDECESSOR));

	/*
	 * If there is a chain it needs to appear to be in a sane state,
	 * otherwise a chain is still needed to generate foundname and
	 * callback_name.
	 */
	if (chain == NULL) {
		options |= DNS_RBTFIND_NOPREDECESSOR;
		chain = &localchain;
		dns_rbtnodechain_init(chain);
	} else {
		dns_rbtnodechain_reset(chain);
	}

	if (rbt->root == NULL) {
		return ISC_R_NOTFOUND;
	}

	/*
	 * Appease GCC about variables it incorrectly thinks are
	 * possibly used uninitialized.
	 */
	compared = dns_namereln_none;
	last_compared = NULL;
	order = 0;

	callback_name = dns_fixedname_initname(&fixedcallbackname);

	/*
	 * search_name is the name segment being sought in each tree level.
	 * By using a fixedname, the search_name will definitely have offsets
	 * for use by any splitting. By using dns_name_clone, no name data
	 * should be copied.
	 */
	search_name = dns_fixedname_initname(&fixedsearchname);
	INSIST(search_name != NULL);
	dns_name_clone(name, search_name);

	dns_name_init(&current_name, NULL);

	saved_result = ISC_R_SUCCESS;
	current = rbt->root;

	while (current != NULL) {
		node_name(current, &current_name);
		compared = dns_name_fullcompare(search_name, &current_name,
						&order, &common_labels);
		/*
		 * last_compared is used as a shortcut to start (or
		 * continue rather) finding the stop-node of the search
		 * when hashing was used (see much below in this
		 * function).
		 */
		last_compared = current;

		if (compared == dns_namereln_equal) {
			break;
		}

		if (compared == dns_namereln_none) {
			/*
			 * Here, current is pointing at a subtree root
			 * node. We try to find a matching node using
			 * the hashtable. We can get one of 3 results
			 * here: (a) we locate the matching node, (b) we
			 * find a node to which the current node has a
			 * subdomain relation, (c) we fail to find (a)
			 * or (b).
			 */

			dns_name_t hash_name;
			dns_rbtnode_t *hnode;
			dns_rbtnode_t *up_current;
			unsigned int nlabels;
			unsigned int tlabels = 1;
			uint32_t hashval;
			uint32_t hash;

			/*
			 * The case of current not being a subtree root,
			 * that means a left or right pointer was
			 * followed, only happens when the algorithm
			 * fell through to the traditional binary search
			 * because of a bitstring label.  Since we
			 * dropped the bitstring support, this should
			 * not happen.
			 */
			INSIST(current->is_root);

			nlabels = dns_name_countlabels(search_name);

			/*
			 * current is the root of the current level, so
			 * its parent is the same as its "up" pointer.
			 */
			up_current = current->parent;
			dns_name_init(&hash_name, NULL);

		hashagain:
			hindex = rbt->hindex;
			/*
			 * Compute the hash over the full absolute
			 * name. Look for the smallest suffix match at
			 * this tree level (hlevel), and then at every
			 * iteration, look for the next smallest suffix
			 * match (add another subdomain label to the
			 * absolute name being hashed).
			 */
			dns_name_getlabelsequence(name, nlabels - tlabels,
						  hlabels + tlabels,
						  &hash_name);
			hashval = dns_name_hash(&hash_name);

			dns_name_getlabelsequence(search_name,
						  nlabels - tlabels, tlabels,
						  &hash_name);

		nexttable:
			/*
			 * Walk all the nodes in the hash bucket pointed
			 * by the computed hash value.
			 */

			hash = isc_hash_bits32(hashval, rbt->hashbits[hindex]);

			for (hnode = rbt->hashtable[hindex][hash];
			     hnode != NULL; hnode = hnode->hashnext)
			{
				dns_name_t hnode_name;

				if (hashval != hnode->hashval) {
					continue;
				}
				/*
				 * This checks that the hashed label sequence
				 * being looked up is at the same tree level, so
				 * that we don't match a labelsequence from some
				 * other subdomain.
				 */
				if (get_upper_node(hnode) != up_current) {
					continue;
				}

				dns_name_init(&hnode_name, NULL);
				node_name(hnode, &hnode_name);
				if (dns_name_equal(&hnode_name, &hash_name)) {
					break;
				}
			}

			if (hnode != NULL) {
				current = hnode;
				/*
				 * This is an optimization.  If hashing found
				 * the right node, the next call to
				 * dns_name_fullcompare() would obviously
				 * return _equal or _subdomain.  Determine
				 * which of those would be the case by
				 * checking if the full name was hashed.  Then
				 * make it look like dns_name_fullcompare
				 * was called and jump to the right place.
				 */
				if (tlabels == nlabels) {
					compared = dns_namereln_equal;
					break;
				} else {
					common_labels = tlabels;
					compared = dns_namereln_subdomain;
					goto subdomain;
				}
			}

			if (TRY_NEXTTABLE(hindex, rbt)) {
				/*
				 * Rehashing in progress, check the other table
				 */
				hindex = RBT_HASH_NEXTTABLE(rbt->hindex);
				goto nexttable;
			}

			if (tlabels++ < nlabels) {
				goto hashagain;
			}

			/*
			 * All of the labels have been tried against the hash
			 * table.
			 */
			current = NULL;
			continue;
		} else {
			/*
			 * The names have some common suffix labels.
			 *
			 * If the number in common are equal in length to
			 * the current node's name length, then follow the
			 * down pointer and search in the new tree.
			 */
			if (compared == dns_namereln_subdomain) {
			subdomain:
				/*
				 * Whack off the current node's common parts
				 * for the name to search in the next level.
				 */
				dns_name_split(search_name, common_labels,
					       search_name, NULL);
				hlabels += common_labels;
				/*
				 * This might be the closest enclosing name.
				 */
				if (WANTEMPTYDATA_OR_DATA(options, current)) {
					*node = current;
				}

				/*
				 * Point the chain to the next level.   This
				 * needs to be done before 'current' is pointed
				 * there because the callback in the next
				 * block of code needs the current 'current',
				 * but in the event the callback requests that
				 * the search be stopped then the
				 * DNS_R_PARTIALMATCH code at the end of this
				 * function needs the chain pointed to the
				 * next level.
				 */
				ADD_LEVEL(chain, current);

				/*
				 * The caller may want to interrupt the
				 * downward search when certain special nodes
				 * are traversed.  If this is a special node,
				 * the callback is used to learn what the
				 * caller wants to do.
				 */
				if (callback != NULL && current->find_callback)
				{
					result = chain_name(
						chain, callback_name, false);
					if (result != ISC_R_SUCCESS) {
						dns_rbtnodechain_reset(chain);
						return result;
					}

					result =
						(callback)(current,
							   callback_name,
							   callback_arg
								   DNS__DB_FLARG_PASS);
					if (result != DNS_R_CONTINUE) {
						saved_result = result;
						/*
						 * Treat this node as if it
						 * had no down pointer.
						 */
						current = NULL;
						break;
					}
				}

				/*
				 * Finally, head to the next tree level.
				 */
				current = current->down;
			} else {
				/*
				 * Though there are labels in common, the
				 * entire name at this node is not common
				 * with the search name so the search
				 * name does not exist in the tree.
				 */
				INSIST(compared ==
					       dns_namereln_commonancestor ||
				       compared == dns_namereln_contains);

				current = NULL;
			}
		}
	}

	/*
	 * If current is not NULL, NOEXACT is not disallowing exact matches,
	 * and either the node has data or an empty node is ok, return
	 * ISC_R_SUCCESS to indicate an exact match.
	 */
	if (current != NULL && (options & DNS_RBTFIND_NOEXACT) == 0 &&
	    WANTEMPTYDATA_OR_DATA(options, current))
	{
		/*
		 * Found an exact match.
		 */
		chain->end = current;
		chain->level_matches = chain->level_count;

		if (foundname != NULL) {
			result = chain_name(chain, foundname, true);
		} else {
			result = ISC_R_SUCCESS;
		}

		if (result == ISC_R_SUCCESS) {
			*node = current;
			result = saved_result;
		} else {
			*node = NULL;
		}
	} else {
		/*
		 * Did not find an exact match (or did not want one).
		 */
		if (*node != NULL) {
			/*
			 * ... but found a partially matching superdomain.
			 * Unwind the chain to the partial match node
			 * to set level_matches to the level above the node,
			 * and then to derive the name.
			 *
			 * chain->level_count is guaranteed to be at least 1
			 * here because by definition of finding a superdomain,
			 * the chain is pointed to at least the first subtree.
			 */
			chain->level_matches = chain->level_count - 1;

			while (chain->levels[chain->level_matches] != *node) {
				INSIST(chain->level_matches > 0);
				chain->level_matches--;
			}

			if (foundname != NULL) {
				unsigned int saved_count = chain->level_count;

				chain->level_count = chain->level_matches + 1;

				result = chain_name(chain, foundname, false);

				chain->level_count = saved_count;
			} else {
				result = ISC_R_SUCCESS;
			}

			if (result == ISC_R_SUCCESS) {
				result = DNS_R_PARTIALMATCH;
			}
		} else {
			result = ISC_R_NOTFOUND;
		}

		if (current != NULL) {
			/*
			 * There was an exact match but either
			 * DNS_RBTFIND_NOEXACT was set, or
			 * DNS_RBTFIND_EMPTYDATA was set and the node had no
			 * data.  A policy decision was made to set the
			 * chain to the exact match, but this is subject
			 * to change if it becomes apparent that something
			 * else would be more useful.  It is important that
			 * this case is handled here, because the predecessor
			 * setting code below assumes the match was not exact.
			 */
			INSIST(((options & DNS_RBTFIND_NOEXACT) != 0) ||
			       ((options & DNS_RBTFIND_EMPTYDATA) == 0 &&
				current->data == NULL));
			chain->end = current;
		} else if ((options & DNS_RBTFIND_NOPREDECESSOR) != 0) {
			/*
			 * Ensure the chain points nowhere.
			 */
			chain->end = NULL;
		} else {
			/*
			 * Since there was no exact match, the chain argument
			 * needs to be pointed at the DNSSEC predecessor of
			 * the search name.
			 */
			if (compared == dns_namereln_subdomain) {
				/*
				 * Attempted to follow a down pointer that was
				 * NULL, which means the searched for name was
				 * a subdomain of a terminal name in the tree.
				 * Since there are no existing subdomains to
				 * order against, the terminal name is the
				 * predecessor.
				 */
				INSIST(chain->level_count > 0);
				INSIST(chain->level_matches <
				       chain->level_count);
				chain->end =
					chain->levels[--chain->level_count];
			} else {
				isc_result_t result2;

				/*
				 * Point current to the node that stopped
				 * the search.
				 *
				 * With the hashing modification that has been
				 * added to the algorithm, the stop node of a
				 * standard binary search is not known.  So it
				 * has to be found.  There is probably a more
				 * clever way of doing this.
				 *
				 * The assignment of current to NULL when
				 * the relationship is *not* dns_namereln_none,
				 * even though it later gets set to the same
				 * last_compared anyway, is simply to not push
				 * the while loop in one more level of
				 * indentation.
				 */
				if (compared == dns_namereln_none) {
					current = last_compared;
				} else {
					current = NULL;
				}

				while (current != NULL) {
					node_name(current, &current_name);
					compared = dns_name_fullcompare(
						search_name, &current_name,
						&order, &common_labels);
					POST(compared);

					last_compared = current;

					/*
					 * Standard binary search movement.
					 */
					if (order < 0) {
						current = current->left;
					} else {
						current = current->right;
					}
				}

				current = last_compared;

				/*
				 * Reached a point within a level tree that
				 * positively indicates the name is not
				 * present, but the stop node could be either
				 * less than the desired name (order > 0) or
				 * greater than the desired name (order < 0).
				 *
				 * If the stop node is less, it is not
				 * necessarily the predecessor.  If the stop
				 * node has a down pointer, then the real
				 * predecessor is at the end of a level below
				 * (not necessarily the next level).
				 * Move down levels until the rightmost node
				 * does not have a down pointer.
				 *
				 * When the stop node is greater, it is
				 * the successor.  All the logic for finding
				 * the predecessor is handily encapsulated
				 * in dns_rbtnodechain_prev.  In the event
				 * that the search name is less than anything
				 * else in the tree, the chain is reset.
				 * XXX DCL What is the best way for the caller
				 *         to know that the search name has
				 *         no predecessor?
				 */

				if (order > 0) {
					if (current->down != NULL) {
						ADD_LEVEL(chain, current);

						result2 = move_chain_to_last(
							chain, current->down);

						if (result2 != ISC_R_SUCCESS) {
							result = result2;
						}
					} else {
						/*
						 * Ah, the pure and simple
						 * case.  The stop node is the
						 * predecessor.
						 */
						chain->end = current;
					}
				} else {
					INSIST(order < 0);

					chain->end = current;

					result2 = dns_rbtnodechain_prev(
						chain, NULL, NULL);
					if (result2 == ISC_R_SUCCESS ||
					    result2 == DNS_R_NEWORIGIN)
					{
						/* Nothing. */
					} else if (result2 == ISC_R_NOMORE) {
						/*
						 * There is no predecessor.
						 */
						dns_rbtnodechain_reset(chain);
					} else {
						result = result2;
					}
				}
			}
		}
	}

	ENSURE(*node == NULL || DNS_RBTNODE_VALID(*node));

	return result;
}

/*
 * Remove a node from the tree of trees.
 *
 * NOTE WELL: deletion is *not* symmetric with addition; that is, reversing
 * a sequence of additions to be deletions will not generally get the
 * tree back to the state it started in.  For example, if the addition
 * of "b.c" caused the node "a.b.c" to be split, pushing "a" to its own level,
 * then the subsequent deletion of "b.c" will not cause "a" to be pulled up,
 * restoring "a.b.c".  The RBT *used* to do this kind of rejoining, but it
 * turned out to be a bad idea because it could corrupt an active nodechain
 * that had "b.c" as one of its levels -- and the RBT has no idea what
 * nodechains are in use by callers, so it can't even *try* to helpfully
 * fix them up (which would probably be doomed to failure anyway).
 *
 * Similarly, it is possible to leave the tree in a state where a supposedly
 * deleted node still exists.  The first case of this is obvious; take
 * the tree which has "b.c" on one level, pointing to "a".  Now deleted "b.c".
 * It was just established in the previous paragraph why we can't pull "a"
 * back up to its parent level.  But what happens when "a" then gets deleted?
 * "b.c" is left hanging around without data or children.  This condition
 * is actually pretty easy to detect, but ... should it really be removed?
 * Is a chain pointing to it?  An iterator?  Who knows!  (Note that the
 * references structure member cannot be looked at because it is private to
 * rbtdb.)  This is ugly and makes me unhappy, but after hours of trying to
 * make it more aesthetically proper and getting nowhere, this is the way it
 * is going to stay until such time as it proves to be a *real* problem.
 *
 * Finally, for reference, note that the original routine that did node
 * joining was called join_nodes().  It has been excised, living now only
 * in the CVS history, but comments have been left behind that point to it just
 * in case someone wants to muck with this some more.
 *
 * The one positive aspect of all of this is that joining used to have a
 * case where it might fail.  Without trying to join, now this function always
 * succeeds. It still returns isc_result_t, though, so the API wouldn't change.
 */
isc_result_t
dns_rbt_deletenode(dns_rbt_t *rbt, dns_rbtnode_t *node, bool recurse) {
	dns_rbtnode_t *parent;

	REQUIRE(VALID_RBT(rbt));
	REQUIRE(DNS_RBTNODE_VALID(node));
	INSIST(rbt->nodecount != 0);

	if (node->down != NULL) {
		if (recurse) {
			node->down->parent = NULL;
			deletetreeflat(rbt, 0, true, &node->down);
		} else {
			if (node->data != NULL && rbt->data_deleter != NULL) {
				rbt->data_deleter(node->data, rbt->deleter_arg);
			}
			node->data = NULL;

			/*
			 * Since there is at least one node below this one and
			 * no recursion was requested, the deletion is
			 * complete.  The down node from this node might be all
			 * by itself on a single level, so join_nodes() could
			 * be used to collapse the tree (with all the caveats
			 * of the comment at the start of this function).
			 * But join_nodes() function has now been removed.
			 */
			return ISC_R_SUCCESS;
		}
	}

	/*
	 * Note the node that points to the level of the node
	 * that is being deleted.  If the deleted node is the
	 * top level, parent will be set to NULL.
	 */
	parent = get_upper_node(node);

	/*
	 * This node now has no down pointer, so now it needs
	 * to be removed from this level.
	 */
	deletefromlevel(node, parent == NULL ? &rbt->root : &parent->down);

	if (node->data != NULL && rbt->data_deleter != NULL) {
		rbt->data_deleter(node->data, rbt->deleter_arg);
	}

	unhash_node(rbt, node);
#if DNS_RBT_USEMAGIC
	node->magic = 0;
#endif /* if DNS_RBT_USEMAGIC */
	isc_refcount_destroy(&node->references);

	freenode(rbt, &node);

	/*
	 * This function never fails.
	 */
	return ISC_R_SUCCESS;
}

void
dns_rbt_namefromnode(dns_rbtnode_t *node, dns_name_t *name) {
	REQUIRE(DNS_RBTNODE_VALID(node));
	REQUIRE(name != NULL);
	REQUIRE(name->offsets == NULL);

	node_name(node, name);
}

isc_result_t
dns_rbt_fullnamefromnode(dns_rbtnode_t *node, dns_name_t *name) {
	dns_name_t current;
	isc_result_t result;

	REQUIRE(DNS_RBTNODE_VALID(node));
	REQUIRE(name != NULL);
	REQUIRE(name->buffer != NULL);

	dns_name_init(&current, NULL);
	dns_name_reset(name);

	do {
		INSIST(node != NULL);

		node_name(node, &current);

		result = dns_name_concatenate(name, &current, name, NULL);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		node = get_upper_node(node);
	} while (!dns_name_isabsolute(name));

	return result;
}

char *
dns_rbt_formatnodename(dns_rbtnode_t *node, char *printname,
		       unsigned int size) {
	dns_fixedname_t fixedname;
	dns_name_t *name;
	isc_result_t result;

	REQUIRE(DNS_RBTNODE_VALID(node));
	REQUIRE(printname != NULL);

	name = dns_fixedname_initname(&fixedname);
	result = dns_rbt_fullnamefromnode(node, name);
	if (result == ISC_R_SUCCESS) {
		dns_name_format(name, printname, size);
	} else {
		snprintf(printname, size, "<error building name: %s>",
			 isc_result_totext(result));
	}

	return printname;
}

static dns_rbtnode_t *
rbtnode_new(isc_mem_t *mctx, const dns_name_t *name) {
	dns_rbtnode_t *node = NULL;
	isc_region_t region;
	unsigned int labels;
	size_t nodelen;

	REQUIRE(name->offsets != NULL);

	dns_name_toregion(name, &region);
	labels = dns_name_countlabels(name);
	ENSURE(labels > 0);

	/*
	 * Allocate space for the node structure, the name, and the offsets.
	 */
	nodelen = sizeof(dns_rbtnode_t) + region.length + labels + 1;
	node = isc_mem_get(mctx, nodelen);
	*node = (dns_rbtnode_t){
		.color = BLACK,
		.nsec = DNS_DB_NSEC_NORMAL,
	};

	ISC_LINK_INIT(node, deadlink);

	isc_refcount_init(&node->references, 0);

	/*
	 * The following is stored to make reconstructing a name from the
	 * stored value in the node easy:  the length of the name, the number
	 * of labels, whether the name is absolute or not, the name itself,
	 * and the name's offsets table.
	 *
	 * XXX RTH
	 *      The offsets table could be made smaller by eliminating the
	 *      first offset, which is always 0.  This requires changes to
	 *      lib/dns/name.c.
	 *
	 * Note: OLDOFFSETLEN *must* be assigned *after* OLDNAMELEN is assigned
	 * 	 as it uses OLDNAMELEN.
	 */
	node->oldnamelen = node->namelen = region.length;
	OLDOFFSETLEN(node) = node->offsetlen = labels;
	node->absolute = name->attributes.absolute;

	memmove(NAME(node), region.base, region.length);
	memmove(OFFSETS(node), name->offsets, labels);

#if DNS_RBT_USEMAGIC
	node->magic = DNS_RBTNODE_MAGIC;
#endif /* if DNS_RBT_USEMAGIC */
	return node;
}

/*
 * Add a node to the hash table
 */
static void
hash_add_node(dns_rbt_t *rbt, dns_rbtnode_t *node, const dns_name_t *name) {
	uint32_t hash;

	REQUIRE(name != NULL);

	node->hashval = dns_name_hash(name);

	hash = isc_hash_bits32(node->hashval, rbt->hashbits[rbt->hindex]);
	node->hashnext = rbt->hashtable[rbt->hindex][hash];

	rbt->hashtable[rbt->hindex][hash] = node;
}

/*
 * Initialize hash table
 */
static void
hashtable_new(dns_rbt_t *rbt, uint8_t index, uint8_t bits) {
	REQUIRE(rbt->hashbits[index] == 0U);
	REQUIRE(rbt->hashtable[index] == NULL);
	REQUIRE(bits >= ISC_HASH_MIN_BITS);
	REQUIRE(bits < ISC_HASH_MAX_BITS);

	rbt->hashbits[index] = bits;

	rbt->hashtable[index] = isc_mem_cget(rbt->mctx,
					     ISC_HASHSIZE(rbt->hashbits[index]),
					     sizeof(dns_rbtnode_t *));
}

static void
hashtable_free(dns_rbt_t *rbt, uint8_t index) {
	isc_mem_cput(rbt->mctx, rbt->hashtable[index],
		     ISC_HASHSIZE(rbt->hashbits[index]),
		     sizeof(dns_rbtnode_t *));

	rbt->hashbits[index] = 0U;
	rbt->hashtable[index] = NULL;
}

static uint32_t
rehash_bits(dns_rbt_t *rbt, size_t newcount) {
	uint32_t newbits = rbt->hashbits[rbt->hindex];

	while (newcount >= ISC_HASHSIZE(newbits) && newbits < ISC_HASH_MAX_BITS)
	{
		newbits += 1;
	}

	return newbits;
}

/*
 * Rebuild the hashtable to reduce the load factor
 */
static void
hashtable_rehash(dns_rbt_t *rbt, uint32_t newbits) {
	uint8_t oldindex = rbt->hindex;
	uint32_t oldbits = rbt->hashbits[oldindex];
	uint8_t newindex = RBT_HASH_NEXTTABLE(oldindex);

	REQUIRE(rbt->hashbits[oldindex] >= ISC_HASH_MIN_BITS);
	REQUIRE(rbt->hashbits[oldindex] <= ISC_HASH_MAX_BITS);
	REQUIRE(rbt->hashtable[oldindex] != NULL);

	REQUIRE(newbits <= ISC_HASH_MAX_BITS);
	REQUIRE(rbt->hashbits[newindex] == 0U);
	REQUIRE(rbt->hashtable[newindex] == NULL);

	REQUIRE(newbits > oldbits);

	hashtable_new(rbt, newindex, newbits);

	rbt->hindex = newindex;

	hashtable_rehash_one(rbt);
}

static void
hashtable_rehash_one(dns_rbt_t *rbt) {
	dns_rbtnode_t **newtable = rbt->hashtable[rbt->hindex];
	uint32_t oldsize =
		ISC_HASHSIZE(rbt->hashbits[RBT_HASH_NEXTTABLE(rbt->hindex)]);
	dns_rbtnode_t **oldtable =
		rbt->hashtable[RBT_HASH_NEXTTABLE(rbt->hindex)];
	dns_rbtnode_t *node = NULL;
	dns_rbtnode_t *nextnode;

	/* Find first non-empty node */
	while (rbt->hiter < oldsize && oldtable[rbt->hiter] == NULL) {
		rbt->hiter++;
	}

	/* Rehashing complete */
	if (rbt->hiter == oldsize) {
		hashtable_free(rbt, RBT_HASH_NEXTTABLE(rbt->hindex));
		rbt->hiter = 0;
		return;
	}

	/* Move the first non-empty node from old hashtable to new hashtable */
	for (node = oldtable[rbt->hiter]; node != NULL; node = nextnode) {
		uint32_t hash = isc_hash_bits32(node->hashval,
						rbt->hashbits[rbt->hindex]);
		nextnode = node->hashnext;
		node->hashnext = newtable[hash];
		newtable[hash] = node;
	}

	oldtable[rbt->hiter] = NULL;

	rbt->hiter++;
}

static void
maybe_rehash(dns_rbt_t *rbt, size_t newcount) {
	uint32_t newbits = rehash_bits(rbt, newcount);

	if (rbt->hashbits[rbt->hindex] < newbits &&
	    newbits <= ISC_HASH_MAX_BITS)
	{
		hashtable_rehash(rbt, newbits);
	}
}

static bool
rehashing_in_progress(dns_rbt_t *rbt) {
	return rbt->hashtable[RBT_HASH_NEXTTABLE(rbt->hindex)] != NULL;
}

static bool
hashtable_is_overcommited(dns_rbt_t *rbt) {
	return rbt->nodecount >=
	       (ISC_HASHSIZE(rbt->hashbits[rbt->hindex]) * ISC_HASH_OVERCOMMIT);
}

/*
 * Add a node to the hash table. Rehash the hashtable if the node count
 * rises above a critical level.
 */
static void
hash_node(dns_rbt_t *rbt, dns_rbtnode_t *node, const dns_name_t *name) {
	REQUIRE(DNS_RBTNODE_VALID(node));

	if (rehashing_in_progress(rbt)) {
		/* Rehash in progress */
		hashtable_rehash_one(rbt);
	} else if (hashtable_is_overcommited(rbt)) {
		/* Rehash requested */
		maybe_rehash(rbt, rbt->nodecount);
	}

	hash_add_node(rbt, node, name);
}

/*
 * Remove a node from the hash table
 */
static void
unhash_node(dns_rbt_t *rbt, dns_rbtnode_t *dnode) {
	uint32_t hash;
	uint8_t hindex = rbt->hindex;
	dns_rbtnode_t *hnode;

	REQUIRE(DNS_RBTNODE_VALID(dnode));

	/*
	 * The node could be either in:
	 *  a) current table: no rehashing in progress, or
	 *  b) current table: the node has been already moved, or
	 *  c) other table: the node hasn't been moved yet.
	 */
nexttable:
	hash = isc_hash_bits32(dnode->hashval, rbt->hashbits[hindex]);

	hnode = rbt->hashtable[hindex][hash];

	if (hnode == dnode) {
		rbt->hashtable[hindex][hash] = hnode->hashnext;
		return;
	} else {
		for (; hnode != NULL; hnode = hnode->hashnext) {
			if (hnode->hashnext == dnode) {
				hnode->hashnext = dnode->hashnext;
				return;
			}
		}
	}

	if (TRY_NEXTTABLE(hindex, rbt)) {
		/* Rehashing in progress, delete from the other table */
		hindex = RBT_HASH_NEXTTABLE(hindex);
		goto nexttable;
	}

	/* We haven't found any matching node, this should not be possible. */
	UNREACHABLE();
}

static void
rotate_left(dns_rbtnode_t *node, dns_rbtnode_t **rootp) {
	dns_rbtnode_t *child;

	REQUIRE(DNS_RBTNODE_VALID(node));
	REQUIRE(rootp != NULL);

	child = node->right;
	INSIST(child != NULL);

	node->right = child->left;
	if (child->left != NULL) {
		child->left->parent = node;
	}
	child->left = node;

	child->parent = node->parent;

	if (node->is_root) {
		*rootp = child;
		child->is_root = 1;
		node->is_root = 0;
	} else {
		if (node->parent->left == node) {
			node->parent->left = child;
		} else {
			node->parent->right = child;
		}
	}

	node->parent = child;
}

static void
rotate_right(dns_rbtnode_t *node, dns_rbtnode_t **rootp) {
	dns_rbtnode_t *child;

	REQUIRE(DNS_RBTNODE_VALID(node));
	REQUIRE(rootp != NULL);

	child = node->left;
	INSIST(child != NULL);

	node->left = child->right;
	if (child->right != NULL) {
		child->right->parent = node;
	}
	child->right = node;

	child->parent = node->parent;

	if (node->is_root) {
		*rootp = child;
		child->is_root = 1;
		node->is_root = 0;
	} else {
		if (node->parent->left == node) {
			node->parent->left = child;
		} else {
			node->parent->right = child;
		}
	}

	node->parent = child;
}

/*
 * This is the real workhorse of the insertion code, because it does the
 * true red/black tree on a single level.
 */
static void
addonlevel(dns_rbtnode_t *node, dns_rbtnode_t *current, int order,
	   dns_rbtnode_t **rootp) {
	dns_rbtnode_t *child, *root, *parent, *grandparent;
	dns_name_t add_name, current_name;
	dns_offsets_t add_offsets, current_offsets;

	REQUIRE(rootp != NULL);
	REQUIRE(DNS_RBTNODE_VALID(node) && node->left == NULL &&
		node->right == NULL);
	REQUIRE(current != NULL);

	root = *rootp;
	if (root == NULL) {
		/*
		 * First node of a level.
		 */
		node->color = BLACK;
		node->is_root = 1;
		node->parent = current;
		*rootp = node;
		return;
	}

	child = root;
	POST(child);

	dns_name_init(&add_name, add_offsets);
	node_name(node, &add_name);

	dns_name_init(&current_name, current_offsets);
	node_name(current, &current_name);

	if (order < 0) {
		INSIST(current->left == NULL);
		current->left = node;
	} else {
		INSIST(current->right == NULL);
		current->right = node;
	}

	INSIST(node->parent == NULL);
	node->parent = current;

	node->color = RED;

	while (node != root && IS_RED(node->parent)) {
		/*
		 * XXXDCL could do away with separate parent and grandparent
		 * variables.  They are vestiges of the days before parent
		 * pointers.  However, they make the code a little clearer.
		 */

		parent = node->parent;
		grandparent = parent->parent;

		if (parent == grandparent->left) {
			child = grandparent->right;
			if (child != NULL && IS_RED(child)) {
				parent->color = BLACK;
				child->color = BLACK;
				grandparent->color = RED;
				node = grandparent;
			} else {
				if (node == parent->right) {
					rotate_left(parent, &root);
					node = parent;
					parent = node->parent;
					grandparent = parent->parent;
				}
				parent->color = BLACK;
				grandparent->color = RED;
				rotate_right(grandparent, &root);
			}
		} else {
			child = grandparent->left;
			if (child != NULL && IS_RED(child)) {
				parent->color = BLACK;
				child->color = BLACK;
				grandparent->color = RED;
				node = grandparent;
			} else {
				if (node == parent->left) {
					rotate_right(parent, &root);
					node = parent;
					parent = node->parent;
					grandparent = parent->parent;
				}
				parent->color = BLACK;
				grandparent->color = RED;
				rotate_left(grandparent, &root);
			}
		}
	}

	root->color = BLACK;
	ENSURE(root->is_root);
	*rootp = root;

	return;
}

/*
 * This is the real workhorse of the deletion code, because it does the
 * true red/black tree on a single level.
 */
static void
deletefromlevel(dns_rbtnode_t *item, dns_rbtnode_t **rootp) {
	dns_rbtnode_t *child, *sibling, *parent;
	dns_rbtnode_t *successor;

	REQUIRE(item != NULL);

	/*
	 * Verify that the parent history is (apparently) correct.
	 */
	INSIST((item->is_root && *rootp == item) ||
	       (!item->is_root &&
		(item->parent->left == item || item->parent->right == item)));

	child = NULL;

	if (item->left == NULL) {
		if (item->right == NULL) {
			if (item->is_root) {
				/*
				 * This is the only item in the tree.
				 */
				*rootp = NULL;
				return;
			}
		} else {
			/*
			 * This node has one child, on the right.
			 */
			child = item->right;
		}
	} else if (item->right == NULL) {
		/*
		 * This node has one child, on the left.
		 */
		child = item->left;
	} else {
		dns_rbtnode_t *saved_parent, *saved_right;
		int saved_color;

		/*
		 * This node has two children, so it cannot be directly
		 * deleted.  Find its immediate in-order successor and
		 * move it to this location, then do the deletion at the
		 * old site of the successor.
		 */
		successor = item->right;
		while (successor->left != NULL) {
			successor = successor->left;
		}

		/*
		 * The successor cannot possibly have a left child;
		 * if there is any child, it is on the right.
		 */
		if (successor->right != NULL) {
			child = successor->right;
		}

		/*
		 * Swap the two nodes; it would be simpler to just replace
		 * the value being deleted with that of the successor,
		 * but this rigamarole is done so the caller has complete
		 * control over the pointers (and memory allocation) of
		 * all of nodes.  If just the key value were removed from
		 * the tree, the pointer to the node would be unchanged.
		 */

		/*
		 * First, put the successor in the tree location of the
		 * node to be deleted.  Save its existing tree pointer
		 * information, which will be needed when linking up
		 * delete to the successor's old location.
		 */
		saved_parent = successor->parent;
		saved_right = successor->right;
		saved_color = successor->color;

		if (item->is_root) {
			*rootp = successor;
			successor->is_root = true;
			item->is_root = false;
		} else if (item->parent->left == item) {
			item->parent->left = successor;
		} else {
			item->parent->right = successor;
		}

		successor->parent = item->parent;
		successor->left = item->left;
		successor->right = item->right;
		successor->color = item->color;

		if (successor->left != NULL) {
			successor->left->parent = successor;
		}
		if (successor->right != successor) {
			successor->right->parent = successor;
		}

		/*
		 * Now relink the node to be deleted into the
		 * successor's previous tree location.
		 */
		INSIST(!item->is_root);

		if (saved_parent == item) {
			/*
			 * Node being deleted was successor's parent.
			 */
			successor->right = item;
			item->parent = successor;
		} else {
			saved_parent->left = item;
			item->parent = saved_parent;
		}

		/*
		 * Original location of successor node has no left.
		 */
		item->left = NULL;
		item->right = saved_right;
		item->color = saved_color;
	}

	/*
	 * Remove the node by removing the links from its parent.
	 */
	if (!item->is_root) {
		if (item->parent->left == item) {
			item->parent->left = child;
		} else {
			item->parent->right = child;
		}

		if (child != NULL) {
			child->parent = item->parent;
		}
	} else {
		/*
		 * This is the root being deleted, and at this point
		 * it is known to have just one child.
		 */
		*rootp = child;
		child->is_root = 1;
		child->parent = item->parent;
	}

	/*
	 * Fix color violations.
	 */
	if (IS_BLACK(item)) {
		parent = item->parent;

		while (child != *rootp && IS_BLACK(child)) {
			INSIST(child == NULL || !child->is_root);

			if (parent->left == child) {
				sibling = parent->right;

				if (IS_RED(sibling)) {
					sibling->color = BLACK;
					parent->color = RED;
					rotate_left(parent, rootp);
					sibling = parent->right;
				}

				INSIST(sibling != NULL);

				if (IS_BLACK(sibling->left) &&
				    IS_BLACK(sibling->right))
				{
					sibling->color = RED;
					child = parent;
				} else {
					if (IS_BLACK(sibling->right)) {
						sibling->left->color = BLACK;
						sibling->color = RED;
						rotate_right(sibling, rootp);
						sibling = parent->right;
					}

					sibling->color = parent->color;
					parent->color = BLACK;
					INSIST(sibling->right != NULL);
					sibling->right->color = BLACK;
					rotate_left(parent, rootp);
					child = *rootp;
				}
			} else {
				/*
				 * Child is parent's right child.
				 * Everything is done the same as above,
				 * except mirrored.
				 */
				sibling = parent->left;

				if (IS_RED(sibling)) {
					sibling->color = BLACK;
					parent->color = RED;
					rotate_right(parent, rootp);
					sibling = parent->left;
				}

				INSIST(sibling != NULL);

				if (IS_BLACK(sibling->left) &&
				    IS_BLACK(sibling->right))
				{
					sibling->color = RED;
					child = parent;
				} else {
					if (IS_BLACK(sibling->left)) {
						sibling->right->color = BLACK;
						sibling->color = RED;
						rotate_left(sibling, rootp);
						sibling = parent->left;
					}

					sibling->color = parent->color;
					parent->color = BLACK;
					INSIST(sibling->left != NULL);
					sibling->left->color = BLACK;
					rotate_right(parent, rootp);
					child = *rootp;
				}
			}

			parent = child->parent;
		}

		if (IS_RED(child)) {
			child->color = BLACK;
		}
	}
}

static void
freenode(dns_rbt_t *rbt, dns_rbtnode_t **nodep) {
	dns_rbtnode_t *node = *nodep;
	*nodep = NULL;

	isc_mem_put(rbt->mctx, node, NODE_SIZE(node));

	rbt->nodecount--;
}

static void
deletetreeflat(dns_rbt_t *rbt, unsigned int quantum, bool unhash,
	       dns_rbtnode_t **nodep) {
	dns_rbtnode_t *root = *nodep;

	while (root != NULL) {
		/*
		 * If there is a left, right or down node, walk into it
		 * and iterate.
		 */
		if (root->left != NULL) {
			dns_rbtnode_t *node = root;
			root = root->left;
			node->left = NULL;
		} else if (root->right != NULL) {
			dns_rbtnode_t *node = root;
			root = root->right;
			node->right = NULL;
		} else if (root->down != NULL) {
			dns_rbtnode_t *node = root;
			root = root->down;
			node->down = NULL;
		} else {
			/*
			 * There are no left, right or down nodes, so we
			 * can free this one and go back to its parent.
			 */
			dns_rbtnode_t *node = root;
			root = root->parent;

			if (rbt->data_deleter != NULL && node->data != NULL) {
				rbt->data_deleter(node->data, rbt->deleter_arg);
			}
			if (unhash) {
				unhash_node(rbt, node);
			}
			/*
			 * Note: we don't call unhash_node() here as we
			 * are destroying the complete RBT tree.
			 */
#if DNS_RBT_USEMAGIC
			node->magic = 0;
#endif /* if DNS_RBT_USEMAGIC */
			freenode(rbt, &node);
			if (quantum != 0 && --quantum == 0) {
				break;
			}
		}
	}

	*nodep = root;
}

static size_t
getheight_helper(dns_rbtnode_t *node) {
	size_t dl, dr;
	size_t this_height, down_height;

	if (node == NULL) {
		return 0;
	}

	dl = getheight_helper(node->left);
	dr = getheight_helper(node->right);

	this_height = ISC_MAX(dl + 1, dr + 1);
	down_height = getheight_helper(node->down);

	return ISC_MAX(this_height, down_height);
}

size_t
dns__rbt_getheight(dns_rbt_t *rbt) {
	return getheight_helper(rbt->root);
}

static bool
check_properties_helper(dns_rbtnode_t *node) {
	if (node == NULL) {
		return true;
	}

	if (IS_RED(node)) {
		/* Root nodes must be BLACK. */
		if (node->is_root) {
			return false;
		}

		/* Both children of RED nodes must be BLACK. */
		if (IS_RED(node->left) || IS_RED(node->right)) {
			return false;
		}
	}

	if ((node->down != NULL) && (!node->down->is_root)) {
		return false;
	}

	if (node->is_root) {
		if ((node->parent != NULL) && (node->parent->down != node)) {
			return false;
		}

		if (get_upper_node(node) != node->parent) {
			return false;
		}
	}

	/* If node is assigned to the down_ pointer of its parent, it is
	 * a subtree root and must have the flag set.
	 */
	if (((!node->parent) || (node->parent->down == node)) &&
	    (!node->is_root))
	{
		return false;
	}

	/* Repeat tests with this node's children. */
	return check_properties_helper(node->left) &&
	       check_properties_helper(node->right) &&
	       check_properties_helper(node->down);
}

static bool
check_black_distance_helper(dns_rbtnode_t *node, size_t *distance) {
	size_t dl, dr, dd;

	if (node == NULL) {
		*distance = 1;
		return true;
	}

	if (!check_black_distance_helper(node->left, &dl)) {
		return false;
	}

	if (!check_black_distance_helper(node->right, &dr)) {
		return false;
	}

	if (!check_black_distance_helper(node->down, &dd)) {
		return false;
	}

	/* Left and right side black node counts must match. */
	if (dl != dr) {
		return false;
	}

	if (IS_BLACK(node)) {
		dl++;
	}

	*distance = dl;

	return true;
}

bool
dns__rbt_checkproperties(dns_rbt_t *rbt) {
	size_t dd;

	if (!check_properties_helper(rbt->root)) {
		return false;
	}

	/* Path from a given node to all its leaves must contain the
	 * same number of BLACK child nodes. This is done separately
	 * here instead of inside check_properties_helper() as
	 * it would take (n log n) complexity otherwise.
	 */
	return check_black_distance_helper(rbt->root, &dd);
}

static void
dns_rbt_indent(FILE *f, int depth) {
	int i;

	fprintf(f, "%4d ", depth);

	for (i = 0; i < depth; i++) {
		fprintf(f, "- ");
	}
}

void
dns_rbt_printnodeinfo(dns_rbtnode_t *n, FILE *f) {
	if (n == NULL) {
		fprintf(f, "Null node\n");
		return;
	}

	fprintf(f, "Node info for nodename: ");
	printnodename(n, true, f);
	fprintf(f, "\n");

	fprintf(f, "n = %p\n", n);

	fprintf(f, "node lock address = %u\n", n->locknum);

	fprintf(f, "Parent: %p\n", n->parent);
	fprintf(f, "Right: %p\n", n->right);
	fprintf(f, "Left: %p\n", n->left);
	fprintf(f, "Down: %p\n", n->down);
	fprintf(f, "Data: %p\n", n->data);
}

static void
printnodename(dns_rbtnode_t *node, bool quoted, FILE *f) {
	isc_region_t r;
	dns_name_t name;
	char buffer[DNS_NAME_FORMATSIZE];
	dns_offsets_t offsets;

	r.length = node->namelen;
	r.base = NAME(node);

	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &r);

	dns_name_format(&name, buffer, sizeof(buffer));

	if (quoted) {
		fprintf(f, "\"%s\"", buffer);
	} else {
		fprintf(f, "%s", buffer);
	}
}

static void
print_text_helper(dns_rbtnode_t *root, dns_rbtnode_t *parent, int depth,
		  const char *direction, void (*data_printer)(FILE *, void *),
		  FILE *f) {
	dns_rbt_indent(f, depth);

	if (root != NULL) {
		printnodename(root, true, f);
		fprintf(f, " (%s, %s", direction,
			root->color == RED ? "RED" : "BLACK");

		if ((!root->is_root && root->parent != parent) ||
		    (root->is_root && depth > 0 && root->parent->down != root))
		{
			fprintf(f, " (BAD parent pointer! -> ");
			if (root->parent != NULL) {
				printnodename(root->parent, true, f);
			} else {
				fprintf(f, "NULL");
			}
			fprintf(f, ")");
		}

		fprintf(f, ")");

		if (root->data != NULL && data_printer != NULL) {
			fprintf(f, " data@%p: ", root->data);
			data_printer(f, root->data);
		}
		fprintf(f, "\n");

		depth++;

		if (root->color == RED && IS_RED(root->left)) {
			fprintf(f, "** Red/Red color violation on left\n");
		}
		print_text_helper(root->left, root, depth, "left", data_printer,
				  f);

		if (root->color == RED && IS_RED(root->right)) {
			fprintf(f, "** Red/Red color violation on right\n");
		}
		print_text_helper(root->right, root, depth, "right",
				  data_printer, f);

		print_text_helper(root->down, NULL, depth, "down", data_printer,
				  f);
	} else {
		fprintf(f, "NULL (%s)\n", direction);
	}
}

void
dns_rbt_printtext(dns_rbt_t *rbt, void (*data_printer)(FILE *, void *),
		  FILE *f) {
	REQUIRE(VALID_RBT(rbt));

	print_text_helper(rbt->root, NULL, 0, "root", data_printer, f);
}

static int
print_dot_helper(dns_rbtnode_t *node, unsigned int *nodecount,
		 bool show_pointers, FILE *f) {
	unsigned int l, r, d;

	if (node == NULL) {
		return 0;
	}

	l = print_dot_helper(node->left, nodecount, show_pointers, f);
	r = print_dot_helper(node->right, nodecount, show_pointers, f);
	d = print_dot_helper(node->down, nodecount, show_pointers, f);

	*nodecount += 1;

	fprintf(f, "node%u[label = \"<f0> |<f1> ", *nodecount);
	printnodename(node, false, f);
	fprintf(f, "|<f2>");

	if (show_pointers) {
		fprintf(f, "|<f3> n=%p|<f4> p=%p", node, node->parent);
	}

	fprintf(f, "\"] [");

	if (IS_RED(node)) {
		fprintf(f, "color=red");
	} else {
		fprintf(f, "color=black");
	}

	/* XXXMUKS: verify that IS_ROOT() indicates subtree root and not
	 * forest root.
	 */
	if (node->is_root) {
		fprintf(f, ",penwidth=3");
	}

	if (node->data == NULL) {
		fprintf(f, ",style=filled,fillcolor=lightgrey");
	}

	fprintf(f, "];\n");

	if (node->left != NULL) {
		fprintf(f, "\"node%u\":f0 -> \"node%u\":f1;\n", *nodecount, l);
	}

	if (node->down != NULL) {
		fprintf(f, "\"node%u\":f1 -> \"node%u\":f1 [penwidth=5];\n",
			*nodecount, d);
	}
	if (node->right != NULL) {
		fprintf(f, "\"node%u\":f2 -> \"node%u\":f1;\n", *nodecount, r);
	}

	return *nodecount;
}

void
dns_rbt_printdot(dns_rbt_t *rbt, bool show_pointers, FILE *f) {
	unsigned int nodecount = 0;

	REQUIRE(VALID_RBT(rbt));

	fprintf(f, "digraph g {\n");
	fprintf(f, "node [shape = record,height=.1];\n");
	print_dot_helper(rbt->root, &nodecount, show_pointers, f);
	fprintf(f, "}\n");
}

/*
 * Chain Functions
 */

void
dns_rbtnodechain_init(dns_rbtnodechain_t *chain) {
	REQUIRE(chain != NULL);

	/*
	 * Initialize 'chain'.
	 */
	chain->end = NULL;
	chain->level_count = 0;
	chain->level_matches = 0;
	memset(chain->levels, 0, sizeof(chain->levels));

	chain->magic = CHAIN_MAGIC;
}

isc_result_t
dns_rbtnodechain_current(dns_rbtnodechain_t *chain, dns_name_t *name,
			 dns_name_t *origin, dns_rbtnode_t **node) {
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_CHAIN(chain));

	SET_IF_NOT_NULL(node, chain->end);

	if (chain->end == NULL) {
		return ISC_R_NOTFOUND;
	}

	if (name != NULL) {
		node_name(chain->end, name);

		if (chain->level_count == 0) {
			/*
			 * Names in the top level tree are all absolute.
			 * Always make 'name' relative.
			 */
			INSIST(dns_name_isabsolute(name));

			/*
			 * This is cheaper than
			 * dns_name_getlabelsequence().
			 */
			name->labels--;
			name->length--;
			name->attributes.absolute = false;
		}
	}

	if (origin != NULL) {
		if (chain->level_count > 0) {
			result = chain_name(chain, origin, false);
		} else {
			dns_name_copy(dns_rootname, origin);
		}
	}

	return result;
}

isc_result_t
dns_rbtnodechain_prev(dns_rbtnodechain_t *chain, dns_name_t *name,
		      dns_name_t *origin) {
	dns_rbtnode_t *current, *previous, *predecessor;
	isc_result_t result = ISC_R_SUCCESS;
	bool new_origin = false;

	REQUIRE(VALID_CHAIN(chain) && chain->end != NULL);

	predecessor = NULL;

	current = chain->end;

	if (current->left != NULL) {
		/*
		 * Moving left one then right as far as possible is the
		 * previous node, at least for this level.
		 */
		current = current->left;

		while (current->right != NULL) {
			current = current->right;
		}

		predecessor = current;
	} else {
		/*
		 * No left links, so move toward the root.  If at any
		 * point on the way there the link from parent to child
		 * is a right link, then the parent is the previous
		 * node, at least for this level.
		 */
		while (!current->is_root) {
			previous = current;
			current = current->parent;

			if (current->right == previous) {
				predecessor = current;
				break;
			}
		}
	}

	if (predecessor != NULL) {
		/*
		 * Found a predecessor node in this level.  It might not
		 * really be the predecessor, however.
		 */
		if (predecessor->down != NULL) {
			/*
			 * The predecessor is really down at least one
			 * level. Go down and as far right as possible,
			 * and repeat as long as the rightmost node has
			 * a down pointer.
			 */
			do {
				/*
				 * XXX DCL Need to do something about
				 * origins here. See whether to go down,
				 * and if so whether it is truly what
				 * Bob calls a new origin.
				 */
				ADD_LEVEL(chain, predecessor);
				predecessor = predecessor->down;

				/* XXX DCL duplicated from above; clever
				 * way to unduplicate? */

				while (predecessor->right != NULL) {
					predecessor = predecessor->right;
				}
			} while (predecessor->down != NULL);

			/* XXX DCL probably needs work on the concept */
			if (origin != NULL) {
				new_origin = true;
			}
		}
	} else if (chain->level_count > 0) {
		/*
		 * Dang, didn't find a predecessor in this level.
		 * Got to the root of this level without having
		 * traversed any right links.  Ascend the tree one
		 * level; the node that points to this tree is the
		 * predecessor.
		 */
		INSIST(chain->level_count > 0 && current->is_root);
		predecessor = chain->levels[--chain->level_count];

		/* XXX DCL probably needs work on the concept */
		/*
		 * Don't declare an origin change when the new origin is
		 * "." at the top level tree, because "." is declared as
		 * the origin for the second level tree.
		 */
		if (origin != NULL &&
		    (chain->level_count > 0 || predecessor->offsetlen > 1))
		{
			new_origin = true;
		}
	}

	if (predecessor != NULL) {
		chain->end = predecessor;

		if (new_origin) {
			result = dns_rbtnodechain_current(chain, name, origin,
							  NULL);
			if (result == ISC_R_SUCCESS) {
				result = DNS_R_NEWORIGIN;
			}
		} else {
			result = dns_rbtnodechain_current(chain, name, NULL,
							  NULL);
		}
	} else {
		result = ISC_R_NOMORE;
	}

	return result;
}

isc_result_t
dns_rbtnodechain_down(dns_rbtnodechain_t *chain, dns_name_t *name,
		      dns_name_t *origin) {
	dns_rbtnode_t *current, *successor;
	isc_result_t result = ISC_R_SUCCESS;
	bool new_origin = false;

	REQUIRE(VALID_CHAIN(chain) && chain->end != NULL);

	successor = NULL;

	current = chain->end;

	if (current->down != NULL) {
		/*
		 * Don't declare an origin change when the new origin is
		 * "." at the second level tree, because "." is already
		 * declared as the origin for the top level tree.
		 */
		if (chain->level_count > 0 || current->offsetlen > 1) {
			new_origin = true;
		}

		ADD_LEVEL(chain, current);
		current = current->down;

		while (current->left != NULL) {
			current = current->left;
		}

		successor = current;
	}

	if (successor != NULL) {
		chain->end = successor;

		/*
		 * It is not necessary to use dns_rbtnodechain_current
		 * like the other functions because this function will
		 * never find a node in the topmost level.  This is
		 * because the root level will never be more than one
		 * name, and everything in the megatree is a successor
		 * to that node, down at the second level or below.
		 */

		if (name != NULL) {
			node_name(chain->end, name);
		}

		if (new_origin) {
			if (origin != NULL) {
				result = chain_name(chain, origin, false);
			}

			if (result == ISC_R_SUCCESS) {
				result = DNS_R_NEWORIGIN;
			}
		} else {
			result = ISC_R_SUCCESS;
		}
	} else {
		result = ISC_R_NOMORE;
	}

	return result;
}

isc_result_t
dns_rbtnodechain_nextflat(dns_rbtnodechain_t *chain, dns_name_t *name) {
	dns_rbtnode_t *current, *previous, *successor;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_CHAIN(chain) && chain->end != NULL);

	successor = NULL;

	current = chain->end;

	if (current->right == NULL) {
		while (!current->is_root) {
			previous = current;
			current = current->parent;

			if (current->left == previous) {
				successor = current;
				break;
			}
		}
	} else {
		current = current->right;

		while (current->left != NULL) {
			current = current->left;
		}

		successor = current;
	}

	if (successor != NULL) {
		chain->end = successor;

		if (name != NULL) {
			node_name(chain->end, name);
		}

		result = ISC_R_SUCCESS;
	} else {
		result = ISC_R_NOMORE;
	}

	return result;
}

isc_result_t
dns_rbtnodechain_next(dns_rbtnodechain_t *chain, dns_name_t *name,
		      dns_name_t *origin) {
	dns_rbtnode_t *current, *previous, *successor;
	isc_result_t result = ISC_R_SUCCESS;
	bool new_origin = false;

	REQUIRE(VALID_CHAIN(chain) && chain->end != NULL);

	successor = NULL;

	current = chain->end;

	/*
	 * If there is a level below this node, the next node is the
	 * leftmost node of the next level.
	 */
	if (current->down != NULL) {
		/*
		 * Don't declare an origin change when the new origin is
		 * "." at the second level tree, because "." is already
		 * declared as the origin for the top level tree.
		 */
		if (chain->level_count > 0 || current->offsetlen > 1) {
			new_origin = true;
		}

		ADD_LEVEL(chain, current);
		current = current->down;

		while (current->left != NULL) {
			current = current->left;
		}

		successor = current;
	} else if (current->right == NULL) {
		/*
		 * The successor is up, either in this level or a
		 * previous one. Head back toward the root of the tree,
		 * looking for any path that was via a left link; the
		 * successor is the node that has that left link.  In
		 * the event the root of the level is reached without
		 * having traversed any left links, ascend one level and
		 * look for either a right link off the point of ascent,
		 * or search for a left link upward again, repeating
		 * ascends until either case is true.
		 */
		do {
			while (!current->is_root) {
				previous = current;
				current = current->parent;

				if (current->left == previous) {
					successor = current;
					break;
				}
			}

			if (successor == NULL) {
				/*
				 * Reached the root without having
				 * traversed any left pointers, so this
				 * level is done.
				 */
				if (chain->level_count == 0) {
					/*
					 * If the tree we are iterating
					 * over was modified since this
					 * chain was initialized in a
					 * way that caused node splits
					 * to occur, "current" may now
					 * be pointing to a root node
					 * which appears to be at level
					 * 0, but still has a parent. If
					 * that happens, abort.
					 * Otherwise, we are done
					 * looking for a successor as we
					 * really reached the root node
					 * on level 0.
					 */
					INSIST(current->parent == NULL);
					break;
				}

				current = chain->levels[--chain->level_count];
				new_origin = true;

				if (current->right != NULL) {
					break;
				}
			}
		} while (successor == NULL);
	}

	if (successor == NULL && current->right != NULL) {
		current = current->right;

		while (current->left != NULL) {
			current = current->left;
		}

		successor = current;
	}

	if (successor != NULL) {
		/*
		 * If we determine that the current node is the
		 * successor to itself, we will run into an infinite
		 * loop, so abort instead.
		 */
		INSIST(chain->end != successor);

		chain->end = successor;

		/*
		 * It is not necessary to use dns_rbtnodechain_current
		 * like the other functions because this function will
		 * never find a node in the topmost level.  This is
		 * because the root level will never be more than one
		 * name, and everything in the megatree is a successor
		 * to that node, down at the second level or below.
		 */

		if (name != NULL) {
			node_name(chain->end, name);
		}

		if (new_origin) {
			if (origin != NULL) {
				result = chain_name(chain, origin, false);
			}

			if (result == ISC_R_SUCCESS) {
				result = DNS_R_NEWORIGIN;
			}
		} else {
			result = ISC_R_SUCCESS;
		}
	} else {
		result = ISC_R_NOMORE;
	}

	return result;
}

isc_result_t
dns_rbtnodechain_first(dns_rbtnodechain_t *chain, dns_rbt_t *rbt,
		       dns_name_t *name, dns_name_t *origin)

{
	isc_result_t result;

	REQUIRE(VALID_RBT(rbt));
	REQUIRE(VALID_CHAIN(chain));

	dns_rbtnodechain_reset(chain);

	chain->end = rbt->root;

	result = dns_rbtnodechain_current(chain, name, origin, NULL);

	if (result == ISC_R_SUCCESS) {
		result = DNS_R_NEWORIGIN;
	}

	return result;
}

isc_result_t
dns_rbtnodechain_last(dns_rbtnodechain_t *chain, dns_rbt_t *rbt,
		      dns_name_t *name, dns_name_t *origin)

{
	isc_result_t result;

	REQUIRE(VALID_RBT(rbt));
	REQUIRE(VALID_CHAIN(chain));

	dns_rbtnodechain_reset(chain);

	result = move_chain_to_last(chain, rbt->root);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = dns_rbtnodechain_current(chain, name, origin, NULL);

	if (result == ISC_R_SUCCESS) {
		result = DNS_R_NEWORIGIN;
	}

	return result;
}

void
dns_rbtnodechain_reset(dns_rbtnodechain_t *chain) {
	REQUIRE(VALID_CHAIN(chain));

	/*
	 * Free any dynamic storage associated with 'chain', and then
	 * reinitialize 'chain'.
	 */
	chain->end = NULL;
	chain->level_count = 0;
	chain->level_matches = 0;
}

void
dns_rbtnodechain_invalidate(dns_rbtnodechain_t *chain) {
	/*
	 * Free any dynamic storage associated with 'chain', and then
	 * invalidate 'chain'.
	 */

	dns_rbtnodechain_reset(chain);

	chain->magic = 0;
}

/* XXXMUKS:
 *
 * - worth removing inline as static functions are inlined automatically
 *   where suitable by modern compilers.
 * - bump the size of dns_rbt.nodecount to size_t.
 * - the dumpfile header also contains a nodecount that is unsigned
 *   int. If large files (> 2^32 nodes) are to be supported, the
 *   allocation for this field should be increased.
 */
