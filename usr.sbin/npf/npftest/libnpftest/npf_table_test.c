/*
 * NPF tableset tests.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/kmem.h>
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

static const uint8_t ip6_list[][16] = {
	{
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0xa0, 0xc0, 0xff, 0xfe, 0x10, 0x12, 0x34
	},
	{
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0xa0, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00,
	},
	{
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0xa0, 0xc0, 0xff, 0xfe, 0x10, 0x12, 0x30
	}
};

#define	IPSET_TID		0
#define	IPSET_NAME		"ipset-table"

#define	LPM_TID			1
#define	LPM_NAME		"lpm-table"

#define	CDB_TID			2
#define	CDB_NAME		"cdb-table"

#define	IFADDR_TID		3
#define	IFADDR_NAME		".ifaddr-eth0"

///////////////////////////////////////////////////////////////////////////

static bool
check_ip4(const npf_addr_t *addr, const char *ipstr)
{
	npf_addr_t addr_storage, *test_addr = &addr_storage;
	const int alen = sizeof(struct in_addr);
	test_addr->word32[0] = inet_addr(ipstr);
	return memcmp(addr, test_addr, alen) == 0;
}

static bool
ip4list_insert_lookup(npf_table_t *t, unsigned i)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int alen = sizeof(struct in_addr);
	int error;

	addr->word32[0] = inet_addr(ip_list[i]);
	error = npf_table_insert(t, alen, addr, NPF_NO_NETMASK);
	CHECK_TRUE(error == 0);
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);
	return true;
}

static bool
fill_with_ip4(npf_tableset_t *tblset)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int alen = sizeof(struct in_addr);
	const int nm = NPF_NO_NETMASK;

	for (unsigned i = 0; i < __arraycount(ip_list); i++) {
		npf_table_t *t;
		int error;

		addr->word32[0] = inet_addr(ip_list[i]);

		t = npf_tableset_getbyname(tblset, IPSET_NAME);
		error = npf_table_insert(t, alen, addr, nm);
		CHECK_TRUE(error == 0);
		error = npf_table_insert(t, alen, addr, nm);
		CHECK_TRUE(error != 0); // duplicate

		t = npf_tableset_getbyname(tblset, LPM_NAME);
		error = npf_table_insert(t, alen, addr, nm);
		CHECK_TRUE(error == 0);
		error = npf_table_insert(t, alen, addr, nm);
		CHECK_TRUE(error != 0); // duplicate
	}
	return true;
}

static bool
verify_ip4(npf_tableset_t *tblset)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const size_t alen = sizeof(struct in_addr);
	const int nm = NPF_NO_NETMASK;
	npf_table_t *t;
	int error;

	/* Attempt to add duplicates - should fail. */
	addr->word32[0] = inet_addr(ip_list[0]);

	t = npf_tableset_getbyname(tblset, IPSET_NAME);
	error = npf_table_insert(t, alen, addr, nm);
	CHECK_TRUE(error != 0);

	t = npf_tableset_getbyname(tblset, LPM_NAME);
	error = npf_table_insert(t, alen, addr, nm);
	CHECK_TRUE(error != 0);

	/* Match (validate) each IP entry. */
	for (unsigned i = 0; i < __arraycount(ip_list); i++) {
		addr->word32[0] = inet_addr(ip_list[i]);

		t = npf_tableset_getbyname(tblset, IPSET_NAME);
		error = npf_table_lookup(t, alen, addr);
		CHECK_TRUE(error == 0);

		t = npf_tableset_getbyname(tblset, LPM_NAME);
		error = npf_table_lookup(t, alen, addr);
		CHECK_TRUE(error == 0);
	}
	return true;
}

static bool
clear_ip4(npf_tableset_t *tblset)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int alen = sizeof(struct in_addr);
	const int nm = NPF_NO_NETMASK;

	for (unsigned i = 0; i < __arraycount(ip_list); i++) {
		npf_table_t *t;
		int error;

		addr->word32[0] = inet_addr(ip_list[i]);

		t = npf_tableset_getbyname(tblset, IPSET_NAME);
		error = npf_table_remove(t, alen, addr, nm);
		CHECK_TRUE(error == 0);

		error = npf_table_remove(t, alen, addr, nm);
		CHECK_TRUE(error != 0);

		t = npf_tableset_getbyname(tblset, LPM_NAME);
		error = npf_table_remove(t, alen, addr, nm);
		CHECK_TRUE(error == 0);

		error = npf_table_remove(t, alen, addr, nm);
		CHECK_TRUE(error != 0);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////

static bool
test_basic(npf_tableset_t *tblset)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int alen = sizeof(struct in_addr);
	npf_table_t *t;
	int error;

	/* Basic IP set. */
	t = npf_table_create(IPSET_NAME, IPSET_TID, NPF_TABLE_IPSET, NULL, 0);
	CHECK_TRUE(t != NULL);
	error = npf_tableset_insert(tblset, t);
	CHECK_TRUE(error == 0);

	/* Check for double-insert. */
	error = npf_tableset_insert(tblset, t);
	CHECK_TRUE(error != 0);

	/* Longest-prefix match (LPM). */
	t = npf_table_create(LPM_NAME, LPM_TID, NPF_TABLE_LPM, NULL, 0);
	CHECK_TRUE(t != NULL);
	error = npf_tableset_insert(tblset, t);
	CHECK_TRUE(error == 0);

	/* Table for interface addresses. */
	t = npf_table_create(IFADDR_NAME, IFADDR_TID, NPF_TABLE_IFADDR, NULL, 0);
	CHECK_TRUE(t != NULL);
	error = npf_tableset_insert(tblset, t);
	CHECK_TRUE(error == 0);

	/*
	 * Attempt to match some non-existing entries - should fail.
	 */
	addr->word32[0] = inet_addr(ip_list[0]);

	t = npf_tableset_getbyname(tblset, IPSET_NAME);
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error != 0);

	t = npf_tableset_getbyname(tblset, LPM_NAME);
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error != 0);

	return true;
}

static bool
test_nocopy(npf_tableset_t *tblset)
{
	const int alen = sizeof(struct in_addr);
	const char *tables[] = { IPSET_NAME, LPM_NAME, IFADDR_NAME };
	npf_addr_t *addr, lookup_addr;

	for (unsigned i = 0; i < __arraycount(tables); i++) {
		npf_table_t *t;
		int error;

		addr = kmem_zalloc(sizeof(npf_addr_t), KM_SLEEP);
		assert(addr != NULL);
		addr->word32[0] = inet_addr("172.16.90.10");

		t = npf_tableset_getbyname(tblset, tables[i]);
		(void)npf_table_flush(t);

		error = npf_table_insert(t, alen, addr, NPF_NO_NETMASK);
		CHECK_TRUE(error == 0);

		memcpy(&lookup_addr, addr, alen);
		memset(addr, 0xa5, alen); // explicit memset

		error = npf_table_lookup(t, alen, &lookup_addr);
		CHECK_TRUE(error == 0);

		CHECK_TRUE(*(volatile unsigned char *)addr == 0xa5);
		kmem_free(addr, sizeof(npf_addr_t));
	}
	return true;
}

static bool
test_ip6(npf_tableset_t *tblset)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const size_t alen = sizeof(struct in6_addr);
	const int nm = NPF_NO_NETMASK;
	npf_table_t *t;
	int error;

	/* IPv6 addresses. */
	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));

	t = npf_tableset_getbyname(tblset, IPSET_NAME);
	error = npf_table_insert(t, alen, addr, nm);
	CHECK_TRUE(error == 0);
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);
	error = npf_table_remove(t, alen, addr, nm);
	CHECK_TRUE(error == 0);

	t = npf_tableset_getbyname(tblset, LPM_NAME);
	error = npf_table_insert(t, alen, addr, nm);
	CHECK_TRUE(error == 0);
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);
	error = npf_table_remove(t, alen, addr, nm);
	CHECK_TRUE(error == 0);

	return true;
}

static bool
test_lpm_masks4(npf_tableset_t *tblset)
{
	npf_table_t *t = npf_tableset_getbyname(tblset, LPM_NAME);
	npf_addr_t addr_storage, *addr = &addr_storage;
	const size_t alen = sizeof(struct in_addr);
	int error;

	addr->word32[0] = inet_addr("172.16.90.0");
	error = npf_table_insert(t, alen, addr, 25);
	CHECK_TRUE(error == 0);

	addr->word32[0] = inet_addr("172.16.90.126");
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);

	addr->word32[0] = inet_addr("172.16.90.128");
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error != 0);

	return true;
}

static bool
test_lpm_masks6(npf_tableset_t *tblset)
{
	npf_table_t *t = npf_tableset_getbyname(tblset, LPM_NAME);
	npf_addr_t addr_storage, *addr = &addr_storage;
	const size_t alen = sizeof(struct in6_addr);
	int error;

	/*
	 * 96
	 */
	memcpy(addr, ip6_list[1], sizeof(ip6_list[1]));
	error = npf_table_insert(t, alen, addr, 96);
	CHECK_TRUE(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);

	memcpy(addr, ip6_list[1], sizeof(ip6_list[1]));
	error = npf_table_remove(t, alen, addr, 96);
	CHECK_TRUE(error == 0);

	/*
	 * 32
	 */
	memcpy(addr, ip6_list[2], sizeof(ip6_list[2]));
	error = npf_table_insert(t, alen, addr, 32);
	CHECK_TRUE(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);

	memcpy(addr, ip6_list[2], sizeof(ip6_list[2]));
	error = npf_table_remove(t, alen, addr, 32);
	CHECK_TRUE(error == 0);

	/*
	 * 126
	 */
	memcpy(addr, ip6_list[3], sizeof(ip6_list[3]));
	error = npf_table_insert(t, alen, addr, 126);
	CHECK_TRUE(error == 0);

	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error != 0);

	memcpy(addr, ip6_list[3], sizeof(ip6_list[3]));
	error = npf_table_remove(t, alen, addr, 126);
	CHECK_TRUE(error == 0);

	return true;
}

static bool
test_const_table(npf_tableset_t *tblset, void *blob, size_t size)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	const int alen = sizeof(struct in_addr);
	npf_table_t *t;
	int error;

	t = npf_table_create(CDB_NAME, CDB_TID, NPF_TABLE_CONST, blob, size);
	CHECK_TRUE(t != NULL);

	error = npf_tableset_insert(tblset, t);
	CHECK_TRUE(error == 0);

	addr->word32[0] = inet_addr(ip_list[0]);
	error = npf_table_lookup(t, alen, addr);
	CHECK_TRUE(error == 0);

	for (unsigned i = 1; i < __arraycount(ip_list) - 1; i++) {
		addr->word32[0] = inet_addr(ip_list[i]);
		error = npf_table_lookup(t, alen, addr);
		CHECK_TRUE(error != 0);
	}
	return true;
}

static bool
test_ifaddr_table(npf_tableset_t *tblset)
{
	npf_addr_t addr_storage, *addr = &addr_storage;
	npf_table_t *t = npf_tableset_getbyname(tblset, IFADDR_NAME);
	int error;
	bool ok;

	/* Two IPv4 addresses. */
	ok = ip4list_insert_lookup(t, 0);
	CHECK_TRUE(ok);

	ok = ip4list_insert_lookup(t, 1);
	CHECK_TRUE(ok);

	/* And one IPv6 address. */
	memcpy(addr, ip6_list[0], sizeof(ip6_list[0]));
	error = npf_table_insert(t, sizeof(struct in6_addr), addr, NPF_NO_NETMASK);
	CHECK_TRUE(error == 0);

	/*
	 * Get IPv4 addresses.
	 */
	addr = npf_table_getsome(t, sizeof(struct in_addr), 0);
	ok = check_ip4(addr, "192.168.1.1");
	CHECK_TRUE(ok);

	addr = npf_table_getsome(t, sizeof(struct in_addr), 1);
	ok = check_ip4(addr, "10.0.0.1");
	CHECK_TRUE(ok);

	addr = npf_table_getsome(t, sizeof(struct in_addr), 2);
	ok = check_ip4(addr, "192.168.1.1");
	CHECK_TRUE(ok);

	return true;
}

static void
test_ipset_gc(npf_tableset_t *tblset)
{
	npf_table_t *t = npf_tableset_getbyname(tblset, IPSET_NAME);
	npf_t *npf = npf_getkernctx();

	npf_config_enter(npf);
	npf_table_gc(npf, t);
	npf_table_flush(t);
	npf_config_exit(npf);
}

bool
npf_table_test(bool verbose, void *blob, size_t size)
{
	npf_tableset_t *tblset;
	bool ok;

	(void)verbose;

	tblset = npf_tableset_create(4);
	CHECK_TRUE(tblset != NULL);

	ok = test_basic(tblset);
	CHECK_TRUE(ok);

	/*
	 * Fill IPSET and LPM tables with IPv4 addresses.
	 * Keep them in the table during the other tests.
	 */
	ok = fill_with_ip4(tblset);
	CHECK_TRUE(ok);

	ok = verify_ip4(tblset);
	CHECK_TRUE(ok);

	ok = test_ip6(tblset);
	CHECK_TRUE(ok);

	ok = test_lpm_masks4(tblset);
	CHECK_TRUE(ok);

	ok = test_lpm_masks6(tblset);
	CHECK_TRUE(ok);

	ok = test_const_table(tblset, blob, size);
	CHECK_TRUE(ok);

	ok = test_ifaddr_table(tblset);
	CHECK_TRUE(ok);

	/*
	 * Remove the above IPv4 addresses -- they must have been untouched.
	 */
	ok = clear_ip4(tblset);
	CHECK_TRUE(ok);

	ok = test_nocopy(tblset);
	CHECK_TRUE(ok);

	test_ipset_gc(tblset);

	npf_tableset_destroy(tblset);
	return true;
}
