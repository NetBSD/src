/*	$NetBSD$	*/

/*
 * Public Domain.
 */

#ifndef _NPF_TEST_H_
#define _NPF_TEST_H_

#include <inttypes.h>
#include <stdbool.h>

void		rumpns_npf_test_init(void);
int		rumpns_npf_test_load(const void *);
unsigned	rumpns_npf_test_addif(const char *, unsigned, bool);
unsigned	rumpns_npf_test_getif(const char *);

int		rumpns_npf_test_statetrack(const void *, size_t,
		    unsigned, bool, int64_t *);
void		rumpns_npf_test_conc(bool, unsigned);

bool		rumpns_npf_nbuf_test(bool);
bool		rumpns_npf_bpf_test(bool);
bool		rumpns_npf_table_test(bool);
bool		rumpns_npf_state_test(bool);

bool		rumpns_npf_rule_test(bool);
bool		rumpns_npf_nat_test(bool);

int		process_stream(const char *, const char *, unsigned);

#endif
