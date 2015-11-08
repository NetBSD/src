/*	$NetBSD: lfptest.c,v 1.1.1.1.6.2 2015/11/08 00:16:08 snj Exp $	*/

#include "config.h"
#include "ntp_fp.h"
#include "lfptest.h"

int IsEqual(const l_fp expected, const l_fp actual) {
	if (L_ISEQU(&expected, &actual)) {
		return TRUE;
	} else {
		return FALSE;
	}
}
