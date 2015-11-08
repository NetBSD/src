/*	$NetBSD: lfptest.c,v 1.1.1.1.10.2 2015/11/08 01:55:37 riz Exp $	*/

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
