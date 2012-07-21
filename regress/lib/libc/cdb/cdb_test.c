/*	$NetBSD: cdb_test.c,v 1.1 2012/07/21 22:22:55 rmind Exp $	*/

/*
 * This file is in the Public Domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>

#include "cdbr.h"
#include "cdbw.h"

#define	CDB_FILE		"/tmp/test.cdb"

#define DATASOURCE_SIZE		8192

static uint64_t			k[DATASOURCE_SIZE];
static uint64_t			d[DATASOURCE_SIZE];

static void
build_cdb(uint32_t (*seedgen)(void))
{
	struct cdbw *cdbw = cdbw_open();
	int i, fd, ret;

	for (i = 0; i < DATASOURCE_SIZE; i++) {
		k[i] = ((uint64_t)i << 32UL) | random();
		d[i] = random();
		ret = cdbw_put(cdbw, &k[i], sizeof(k[0]), &d[i], sizeof(d[0]));
		assert(ret == 0);
	}

	fd = open(CDB_FILE, O_RDWR | O_CREAT, 0644);
	assert(fd > 0);

	ret = cdbw_output(cdbw, fd, "test", seedgen);
	assert(ret == 0);

	cdbw_close(cdbw);
	close(fd);
}

static void
test_cdb(void)
{
	struct cdbr *cdbr = cdbr_open(CDB_FILE, CDBR_DEFAULT);
	int i, ret;

	assert(cdbr != NULL);
	assert(cdbr_entries(cdbr) == DATASOURCE_SIZE);

	for (i = 0; i < DATASOURCE_SIZE; i++) {
		const void *val;
		size_t len;

		ret = cdbr_find(cdbr, &k[i], sizeof(k[0]), &val, &len);
		assert(ret == 0);
		assert(len == sizeof(uint64_t));

		const uint64_t num = *(const uint64_t *)val;
		if (d[i] == num) {
			continue;
		}
		fprintf(stderr, "%d: 0x%"PRIu64" != 0x%"PRIu64"\n", i, d[i], num);
		abort();
	}
	cdbr_close(cdbr);
}

int
main(int argc, char **argv)
{
	int i;

	srandom(time(NULL) ^ getpid());

	for (i = 0; i < 64; i++) {
		build_cdb((uint32_t (*)(void))random);
		test_cdb();

		build_cdb(cdbw_stable_seeder);
		test_cdb();
	}

	return 0;
}
