/*	$NetBSD: lfptest.c,v 1.1.1.5 2020/05/25 20:40:16 christos Exp $	*/

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
