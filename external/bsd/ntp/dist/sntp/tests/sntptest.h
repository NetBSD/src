/*	$NetBSD: sntptest.h,v 1.1.1.1.6.1.2.1 2015/11/08 01:55:36 riz Exp $	*/

#ifndef SNTPTEST_H
#define SNTPTEST_H

#include "ntp_stdlib.h"
#include "sntp-opts.h"

void sntptest(void);
void sntptest_destroy(void);
void ActivateOption(const char* option, const char* argument);

#endif // SNTPTEST_H
