/*	$NetBSD: sntptest.h,v 1.7 2024/08/18 20:47:26 christos Exp $	*/

#ifndef SNTPTEST_H
#define SNTPTEST_H

#include "ntp_stdlib.h"
#include "ntp_types.h"
#include "sntp-opts.h"

void sntptest(void);
void sntptest_destroy(void);
void ActivateOption(const char* option, const char* argument);

#endif // SNTPTEST_H
