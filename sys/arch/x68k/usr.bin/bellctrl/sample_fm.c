/*	$NetBSD: sample_fm.c,v 1.3 2003/07/15 01:44:53 lukem Exp $	*/

#include <sys/cdefs.h>
__RCSID("$NetBSD: sample_fm.c,v 1.3 2003/07/15 01:44:53 lukem Exp $");

#include <stdio.h>
#include <stdlib.h>
#include "../../dev/opmreg.h"

/* Sample Voice Parameter : piano */

struct opm_voice bell_voice = {
/*  AR  DR  SR  RR  SL  OL  KS  ML DT1 DT2 AME  */
  { 31,  0,  0,  0,  0, 32,  0, 14,  6,  0,  0, },
  { 31, 12, 12,  9,  3,  2,  0,  4,  6,  0,  0, },
  { 31,  0,  0,  0,  0, 32,  0, 14,  2,  0,  0, },
  { 31, 13, 12,  8,  2,  0,  0,  4,  2,  0,  0, },
/*  CON FL  OP  */
    4,  0, 15
};
