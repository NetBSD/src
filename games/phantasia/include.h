/*	$NetBSD: include.h,v 1.6 2009/05/27 17:44:38 dholland Exp $	*/

/*
 * include.h - includes all important files for Phantasia
 */

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef __dead /* Not NetBSD */
#define __dead
#endif

#include "macros.h"
#include "phantdefs.h"
#include "phantstruct.h"
#include "phantglobs.h"
#include "pathnames.h"
