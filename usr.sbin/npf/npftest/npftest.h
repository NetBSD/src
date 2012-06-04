/*	$NetBSD$	*/

/*
 * Public Domain.
 */

#ifndef _NPF_TEST_H_
#define _NPF_TEST_H_

#include <stdint.h>
#include <stdbool.h>

int		rumpns_npf_test_load(const void *);
int		rumpns_npf_test_handlepkt(const void *, size_t,
		    unsigned, bool, int64_t *);

bool		rumpns_npf_nbuf_test(bool);
bool		rumpns_npf_processor_test(bool);
bool		rumpns_npf_table_test(bool);
bool		rumpns_npf_state_test(bool);

int		process_stream(const char *, const char *, unsigned);

#endif
