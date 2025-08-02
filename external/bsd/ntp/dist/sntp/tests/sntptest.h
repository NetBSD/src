/*	$NetBSD: sntptest.h,v 1.6.8.1 2025/08/02 05:23:02 perseant Exp $	*/

#ifndef SNTPTEST_H
#define SNTPTEST_H

#include "ntp_stdlib.h"
#include "ntp_types.h"
#include "sntp-opts.h"

void sntptest(void);
void sntptest_destroy(void);
void ActivateOption(const char* option, const char* argument);

#endif // SNTPTEST_H
