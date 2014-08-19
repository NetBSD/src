/*	$NetBSD$	*/

/*
 * Public Domain.
 */

#ifndef _NPF_TEST_H_
#define _NPF_TEST_H_

#include <inttypes.h>
#include <stdbool.h>

#include <net/if.h>

void		rumpns_npf_test_init(int (*)(int, const char *, void *),
		    const char *(*)(int, const void *, char *, socklen_t),
		    long (*)(void));
int		rumpns_npf_test_load(const void *);
ifnet_t *	rumpns_npf_test_addif(const char *, bool, bool);
ifnet_t *	rumpns_npf_test_getif(const char *);

int		rumpns_npf_test_statetrack(const void *, size_t,
		    ifnet_t *, bool, int64_t *);
void		rumpns_npf_test_conc(bool, unsigned);

bool		rumpns_npf_nbuf_test(bool);
bool		rumpns_npf_bpf_test(bool);
bool		rumpns_npf_table_test(bool, void *, size_t);
bool		rumpns_npf_state_test(bool);

bool		rumpns_npf_rule_test(bool);
bool		rumpns_npf_nat_test(bool);

int		process_stream(const char *, const char *, ifnet_t *);

#endif
