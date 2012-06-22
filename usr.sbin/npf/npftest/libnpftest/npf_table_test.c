/*	$NetBSD: npf_table_test.c,v 1.2 2012/06/22 13:43:17 rmind Exp $	*/

/*
 * NPF tableset test.
 *
 * Public Domain.
 */

#include <sys/types.h>

#include "npf_impl.h"
#include "npf_test.h"

static const char *ip_list[] = {
	"192.168.1.1",
	"10.0.0.1",
	"192.168.2.1",
	"10.1.0.1",
	"192.168.100.253",
	"10.0.5.1",
	"192.168.128.127",
	"10.0.0.2",
};

#define	HASH_TID		1
#define	TREE_TID		2

bool
npf_table_test(bool verbose)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	npf_tableset_t *tblset;
	npf_table_t *t1, *t2;
	int error;
	u_int i;

	npf_tableset_sysinit();

	tblset = npf_tableset_create();
	assert(tblset != NULL);

	/* Table ID 1, using hash table with 256 lists. */
	t1 = npf_table_create(HASH_TID, NPF_TABLE_HASH, 256);
	assert(t1 != NULL);
	error = npf_tableset_insert(tblset, t1);
	assert(error == 0);

	/* Check for double-insert. */
	error = npf_tableset_insert(tblset, t1);
	assert(error != 0);

	/* Table ID 2, using RB-tree. */
	t2 = npf_table_create(TREE_TID, NPF_TABLE_TREE, 0);
	assert(t2 != NULL);
	error = npf_tableset_insert(tblset, t2);
	assert(error == 0);

	/* Attempt to match non-existing entries - should fail. */
	addr->s6_addr32[0] = inet_addr(ip_list[0]);

	error = npf_table_match_addr(tblset, HASH_TID, addr);
	assert(error != 0);

	error = npf_table_match_addr(tblset, TREE_TID, addr);
	assert(error != 0);

	/* Fill both tables with IP addresses. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->s6_addr32[0] = inet_addr(ip_list[i]);

		error = npf_table_add_cidr(tblset, HASH_TID, addr, 32);
		assert(error == 0);
		error = npf_table_add_cidr(tblset, HASH_TID, addr, 32);
		assert(error != 0);

		error = npf_table_add_cidr(tblset, TREE_TID, addr, 32);
		assert(error == 0);
		error = npf_table_add_cidr(tblset, TREE_TID, addr, 32);
		assert(error != 0);
	}

	/* Attempt to add duplicates - should fail. */
	addr->s6_addr32[0] = inet_addr(ip_list[0]);

	error = npf_table_add_cidr(tblset, HASH_TID, addr, 32);
	assert(error != 0);

	error = npf_table_add_cidr(tblset, TREE_TID, addr, 32);
	assert(error != 0);

	/* Reference checks. */
	t1 = npf_table_get(tblset, HASH_TID);
	assert(t1 != NULL);
	npf_table_put(t1);

	t2 = npf_table_get(tblset, TREE_TID);
	assert(t2 != NULL);
	npf_table_put(t2);

	/* Match (validate) each IP entry. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->s6_addr32[0] = inet_addr(ip_list[i]);

		error = npf_table_match_addr(tblset, HASH_TID, addr);
		assert(error == 0);

		error = npf_table_match_addr(tblset, TREE_TID, addr);
		assert(error == 0);
	}

	/* Remove all entries. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->s6_addr32[0] = inet_addr(ip_list[i]);

		error = npf_table_rem_cidr(tblset, HASH_TID, addr, 32);
		assert(error == 0);

		error = npf_table_rem_cidr(tblset, TREE_TID, addr, 32);
		assert(error == 0);
	}

	npf_tableset_destroy(tblset);
	npf_tableset_sysfini();

	return true;
}
