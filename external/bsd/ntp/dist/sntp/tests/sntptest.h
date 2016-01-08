/*	$NetBSD: sntptest.h,v 1.5 2016/01/08 21:35:42 christos Exp $	*/

#ifndef SNTPTEST_H
#define SNTPTEST_H

#include "ntp_stdlib.h"
#include "sntp-opts.h"

void sntptest(void);
void sntptest_destroy(void);
void ActivateOption(const char* option, const char* argument);

#endif // SNTPTEST_H
