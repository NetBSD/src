/*	$NetBSD: npftest.h,v 1.1 2012/04/14 21:57:29 rmind Exp $	*/

/*
 * Public Domain.
 */

#ifndef _NPF_TEST_H_
#define _NPF_TEST_H_

#include <stdbool.h>

bool		rumpns_npf_nbuf_test(bool);
bool		rumpns_npf_processor_test(bool);
bool		rumpns_npf_table_test(bool);

#endif
