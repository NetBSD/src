/*	$NetBSD: sntptest.h,v 1.1.1.8 2020/05/25 20:40:15 christos Exp $	*/

#ifndef SNTPTEST_H
#define SNTPTEST_H

#include "ntp_stdlib.h"
#include "sntp-opts.h"

void sntptest(void);
void sntptest_destroy(void);
void ActivateOption(const char* option, const char* argument);

#endif // SNTPTEST_H
