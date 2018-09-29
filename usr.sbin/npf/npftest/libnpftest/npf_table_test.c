/*	$NetBSD: npf_table_test.c,v 1.10 2018/09/29 14:41:36 rmind Exp $	*/

/*
 * NPF tableset test.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/malloc.h>
#endif

#ifdef __linux__
#include <endian.h>
#else
#include <sys/endian.h>
#endif

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

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define	U16_TO_LE(x)	((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#else
#define	U16_TO_LE(x)	(x)
#endif

static const uint16_t ip6_list[][8] = {
	{
		U16_TO_LE(0xfe80), 0x0, 0x0, 0x0,
		U16_TO_LE(0x2a0), U16_TO_LE(0xc0ff),
		U16_TO_LE(0xfe10), U16_TO_LE(0x1234)
	},
	{
		U16_TO_LE(0xfe80), 0x0, 0x0, 0x0,
		U16_TO_LE(0x2a0), U16_TO_LE(0xc0ff), 0x00, 0x0
	},
	{
		U16_TO_LE(0xfe80), 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0
	},
	{
		U16_TO_LE(0xfe80), 0x0, 0x0, 0x0,
		U16_TO_LE(0x2a0), U16_TO_LE(0xc0ff),
		U16_TO_LE(0xfe10), U16_TO_LE(0x1230)
	}
};

#define	check_ok(x)	\
    ((x) ? true : (printf("fail at line %d\n", __LINE__), false))

#define	HASH_TID		"hash-table"
#define	TREE_TID		"tree-table"
#define	CDB_TID			"cdb-table"

static bool
npf_table_test_fill4(npf_tableset_t *tblset, npf_addr_t *addr)
{
	const int alen = sizeof(struct in_addr);
	const int nm = NPF_NO_NETMASK;
	bool fail = false;

	/* Fill both tables with IP addresses. */
	for (unsigned i = 0; i < __arraycount(ip_list); i++) {
		npf_table_t *t;
		int error;

		addr->word32[0] = inet_addr(ip_list[i]);

		t = npf_tableset_getbyname(tblset, HASH_TID);
		error = npf_table_insert(t, alen, addr, nm);
		fail |= !check_ok(error == 0);
		error = npf_table_insert(t, alen, addr, nm);
		fail |= !check_ok(error != 0);

		t = npf_tableset_getbyname(tblset, TREE_TID);
		error = npf_table_insert(t, alen, addr, nm);
		fail |= !check_ok(error == 0);
		error = npf_table_insert(t, alen, addr, nm);
		fail |= !check_ok(error != 0);
	}
	return fail;
}

bool
npf_table_test(bool verbose, void *blob, size_t size)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int nm = NPF_NO_NETMASK;
	npf_table_t *t, *t1, *t2, *t3;
	npf_tableset_t *tblset;
	int error, alen;
	bool fail = false;
	unsigned i;

	(void)verbose;

	tblset = npf_tableset_create(3);
	fail |= !check_ok(tblset != NULL);

	/* Table ID 1, using hash table with 256 lists. */
	t1 = npf_table_create(HASH_TID, 0, NPF_TABLE_HASH, NULL, 256);
	fail |= !check_ok(t1 != NULL);
	error = npf_tableset_insert(tblset, t1);
	fail |= !check_ok(error == 0);

	/* Check for double-insert. */
	error = npf_tableset_insert(tblset, t1);
	fail |= !check_ok(error != 0);

	/* Table ID 2, using a prefix tree. */
	t2 = npf_table_create(TREE_TID, 1, NPF_TABLE_TREE, NULL, 0);
	fail |= !(t2 != NULL);
	error = npf_tableset_insert(tblset, t2);
	fail |= !check_ok(error == 0);

	/* Table ID 3, using a CDB. */
	t3 = npf_table_create(CDB_TID, 2, NPF_TABLE_CDB, blob, size);
	fail |= !(t3 != NULL);
	error = npf_tableset_insert(tblset, t3);
	fail |= !check_ok(error == 0);

	/* Attempt to match non-existing entries - should fail. */
	addr->word32[0] = inet_addr(ip_list[0]);
	alen = sizeof(struct in_addr);

	t = npf_tableset_getbyname(tblset, HASH_TID);
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error != 0);

	t = npf_tableset_getbyname(tblset, TREE_TID);
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error != 0);

	/* Fill both tables with IP addresses. */
	fail |= npf_table_test_fill4(tblset, addr);

	/* Attempt to add duplicates - should fail. */
	addr->word32[0] = inet_addr(ip_list[0]);
	alen = sizeof(struct in_addr);

	t = npf_tableset_getbyname(tblset, HASH_TID);
	error = npf_table_insert(t, alen, addr, nm);
	fail |= !check_ok(error != 0);

	t = npf_tableset_getbyname(tblset, TREE_TID);
	error = npf_table_insert(t, alen, addr, nm);
	fail |= !check_ok(error != 0);

	/* Match (validate) each IP entry. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->word32[0] = inet_addr(ip_list[i]);

		t = npf_tableset_getbyname(tblset, HASH_TID);
		error = npf_table_lookup(t, alen, addr);
		fail |= !check_ok(error == 0);

		t = npf_tableset_getbyname(tblset, TREE_TID);
		error = npf_table_lookup(t, alen, addr);
		fail |= !check_ok(error == 0);
	}

	/* IPv6 addresses. */
	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	alen = sizeof(struct in6_addr);

	t = npf_tableset_getbyname(tblset, HASH_TID);
	error = npf_table_insert(t, alen, addr, nm);
	fail |= !check_ok(error == 0);
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error == 0);
	error = npf_table_remove(t, alen, addr, nm);
	fail |= !check_ok(error == 0);

	t = npf_tableset_getbyname(tblset, TREE_TID);
	error = npf_table_insert(t, alen, addr, nm);
	fail |= !check_ok(error == 0);
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error == 0);
	error = npf_table_remove(t, alen, addr, nm);
	fail |= !check_ok(error == 0);

	/*
	 * Masking: 96, 32, 127.
	 */

	memcpy(addr, ip6_list[1], sizeof(ip6_list[1]));
	error = npf_table_insert(t, alen, addr, 96);
	fail |= !check_ok(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error == 0);

	memcpy(addr, ip6_list[1], sizeof(ip6_list[1]));
	error = npf_table_remove(t, alen, addr, 96);
	fail |= !check_ok(error == 0);


	memcpy(addr, ip6_list[2], sizeof(ip6_list[2]));
	error = npf_table_insert(t, alen, addr, 32);
	fail |= !check_ok(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error == 0);

	memcpy(addr, ip6_list[2], sizeof(ip6_list[2]));
	error = npf_table_remove(t, alen, addr, 32);
	fail |= !check_ok(error == 0);


	memcpy(addr, ip6_list[3], sizeof(ip6_list[3]));
	error = npf_table_insert(t, alen, addr, 126);
	fail |= !check_ok(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(t, alen, addr);
	fail |= !check_ok(error != 0);

	memcpy(addr, ip6_list[3], sizeof(ip6_list[3]));
	error = npf_table_remove(t, alen, addr, 126);
	fail |= !check_ok(error == 0);


	alen = sizeof(struct in_addr);

	/* Remove all IPv4 entries. */
	for (i = 0; i < __arraycount(ip_list); i++) {
		addr->word32[0] = inet_addr(ip_list[i]);

		t = npf_tableset_getbyname(tblset, HASH_TID);
		error = npf_table_remove(t, alen, addr, nm);
		fail |= !check_ok(error == 0);

		t = npf_tableset_getbyname(tblset, TREE_TID);
		error = npf_table_remove(t, alen, addr, nm);
		fail |= !check_ok(error == 0);
	}

	/* Test CDB. */
	addr->word32[0] = inet_addr(ip_list[0]);
	alen = sizeof(struct in_addr);
	error = npf_table_lookup(t3, alen, addr);
	fail |= !check_ok(error == 0);

	for (i = 1; i < __arraycount(ip_list) - 1; i++) {
		addr->word32[0] = inet_addr(ip_list[i]);
		alen = sizeof(struct in_addr);
		error = npf_table_lookup(t3, alen, addr);
		fail |= !check_ok(error != 0);
	}

	npf_tableset_destroy(tblset);

	return !fail;
}
