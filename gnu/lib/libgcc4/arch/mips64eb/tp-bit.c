/*	$NetBSD: tp-bit.c,v 1.1 2009/12/18 12:51:43 uebayasi Exp $	*/
#if __LDBL_MANT_DIG__ == 113
#define QUIET_NAN_NEGATED
#define TFLOAT
#include "gcc/config/fp-bit.c"
#endif
