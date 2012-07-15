/*	$NetBSD: npf_table_test.c,v 1.4 2012/07/15 00:22:59 rmind Exp $	*/

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

static const uint32_t ip6_list[][4] = {
	{ 0x000080fe, 0x00000000, 0xffc0a002, 0x341210fe },
	{ 0x000080fe, 0x00000000, 0xffc0a002, 0x00000000 },
	{ 0x000080fe, 0x00000000, 0x00000000, 0x00000000 },
	{ 0x000080fe, 0x00000000, 0xffc0a002, 0x301210fe },
};

#define	HASH_TID		1
#define	TREE_TID		2

bool
npf_table_test(bool verbose)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int nm = NPF_NO_NETMASK;
	npf_tableset_t *tblset;
	npf_table_t *t1, *t2;
	int error, alen;
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
	alen = sizeof(struct in_addr);

	error = npf_table_lookup(tblset, HASH_TID, alen, addr);
	assert(error != 0);

	error = npf_table_lookup(tblset, TREE_TID, alen, addr);
	assert(error != 0);

	/* Fill both tables with IP addresses. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->s6_addr32[0] = inet_addr(ip_list[i]);

		error = npf_table_insert(tblset, HASH_TID, alen, addr, nm);
		assert(error == 0);
		error = npf_table_insert(tblset, HASH_TID, alen, addr, nm);
		assert(error != 0);

		error = npf_table_insert(tblset, TREE_TID, alen, addr, nm);
		assert(error == 0);
		error = npf_table_insert(tblset, TREE_TID, alen, addr, nm);
		assert(error != 0);
	}

	/* Attempt to add duplicates - should fail. */
	addr->s6_addr32[0] = inet_addr(ip_list[0]);
	alen = sizeof(struct in_addr);

	error = npf_table_insert(tblset, HASH_TID, alen, addr, nm);
	assert(error != 0);

	error = npf_table_insert(tblset, TREE_TID, alen, addr, nm);
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

		error = npf_table_lookup(tblset, HASH_TID, alen, addr);
		assert(error == 0);

		error = npf_table_lookup(tblset, TREE_TID, alen, addr);
		assert(error == 0);
	}

	/* IPv6 addresses. */
	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	alen = sizeof(struct in6_addr);

	error = npf_table_insert(tblset, HASH_TID, alen, addr, nm);
	assert(error == 0);
	error = npf_table_lookup(tblset, HASH_TID, alen, addr);
	assert(error == 0);
	error = npf_table_remove(tblset, HASH_TID, alen, addr, nm);
	assert(error == 0);

	error = npf_table_insert(tblset, TREE_TID, alen, addr, nm);
	assert(error == 0);
	error = npf_table_lookup(tblset, TREE_TID, alen, addr);
	assert(error == 0);
	error = npf_table_remove(tblset, TREE_TID, alen, addr, nm);
	assert(error == 0);

	/*
	 * Masking: 96, 32, 127.
	 */

	memcpy(addr, ip6_list[1], sizeof(ip6_list[1]));
	error = npf_table_insert(tblset, TREE_TID, alen, addr, 96);
	assert(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(tblset, TREE_TID, alen, addr);
	assert(error == 0);

	memcpy(addr, ip6_list[1], sizeof(ip6_list[1]));
	error = npf_table_remove(tblset, TREE_TID, alen, addr, 96);
	assert(error == 0);


	memcpy(addr, ip6_list[2], sizeof(ip6_list[2]));
	error = npf_table_insert(tblset, TREE_TID, alen, addr, 32);
	assert(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(tblset, TREE_TID, alen, addr);
	assert(error == 0);

	memcpy(addr, ip6_list[2], sizeof(ip6_list[2]));
	error = npf_table_remove(tblset, TREE_TID, alen, addr, 32);
	assert(error == 0);


	memcpy(addr, ip6_list[3], sizeof(ip6_list[3]));
	error = npf_table_insert(tblset, TREE_TID, alen, addr, 126);
	assert(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(tblset, TREE_TID, alen, addr);
	assert(error != 0);

	memcpy(addr, ip6_list[3], sizeof(ip6_list[3]));
	error = npf_table_remove(tblset, TREE_TID, alen, addr, 126);
	assert(error == 0);


	alen = sizeof(struct in_addr);

	/* Remove all IPv4 entries. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->s6_addr32[0] = inet_addr(ip_list[i]);

		error = npf_table_remove(tblset, HASH_TID, alen, addr, nm);
		assert(error == 0);

		error = npf_table_remove(tblset, TREE_TID, alen, addr, nm);
		assert(error == 0);
	}

	npf_tableset_destroy(tblset);
	npf_tableset_sysfini();

	return true;
}
