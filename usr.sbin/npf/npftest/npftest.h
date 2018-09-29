/*	$NetBSD$	*/

/*
 * Public Domain.
 */

#ifndef _NPF_TEST_H_
#define _NPF_TEST_H_

#include <inttypes.h>
#include <stdbool.h>

#if !defined(_NPF_STANDALONE)
#include <net/if.h>
#else
#define	rumpns_npf_test_addif		npf_test_addif
#define	rumpns_npf_test_load		npf_test_load
#define	rumpns_npf_test_init		npf_test_init
#define	rumpns_npf_test_fini		npf_test_fini
#define	rumpns_npf_test_getif		npf_test_getif
#define	rumpns_npf_nbuf_test		npf_nbuf_test
#define	rumpns_npf_bpf_test		npf_bpf_test
#define	rumpns_npf_table_test		npf_table_test
#define	rumpns_npf_state_test		npf_state_test
#define	rumpns_npf_rule_test		npf_rule_test
#define	rumpns_npf_nat_test		npf_nat_test
#define	rumpns_npf_test_conc		npf_test_conc
#define	rumpns_npf_test_statetrack	npf_test_statetrack
#endif

#include "npf.h"

void		rumpns_npf_test_init(int (*)(int, const char *, void *),
		    const char *(*)(int, const void *, char *, socklen_t),
		    long (*)(void));
void		rumpns_npf_test_fini(void);
int		rumpns_npf_test_load(const void *, size_t, bool);
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
