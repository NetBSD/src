/*	$NetBSD: npftest.h,v 1.3.2.3 2012/08/13 17:49:52 riz Exp $	*/

/*
 * Public Domain.
 */

#ifndef _NPF_TEST_H_
#define _NPF_TEST_H_

#include <stdint.h>
#include <stdbool.h>

int		rumpns_npf_test_load(const void *);
unsigned	rumpns_npf_test_addif(const char *, unsigned, bool);
unsigned	rumpns_npf_test_getif(const char *);
int		rumpns_npf_test_handlepkt(const void *, size_t,
		    unsigned, bool, int64_t *);

bool		rumpns_npf_nbuf_test(bool);
bool		rumpns_npf_processor_test(bool);
bool		rumpns_npf_table_test(bool);
bool		rumpns_npf_state_test(bool);

bool		rumpns_npf_rule_test(bool);
bool		rumpns_npf_nat_test(bool);

int		process_stream(const char *, const char *, unsigned);

#endif
