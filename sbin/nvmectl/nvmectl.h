/*	$NetBSD: nvmectl.h,v 1.7 2018/04/17 15:31:00 nonaka Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (C) 2012-2013 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sbin/nvmecontrol/nvmecontrol.h 326276 2017-11-27 15:37:16Z pfg $
 */

#ifndef __NVMECTL_H__
#define __NVMECTL_H__

#include <sys/ioctl.h>

#include <dev/ic/nvmeio.h>
#include "nvme.h"

typedef void (*nvme_fn_t)(int argc, char *argv[]);

struct nvme_function {
	const char	*name;
	nvme_fn_t	fn;
	const char	*usage;
};

#define NVME_CTRLR_PREFIX	"nvme"
#define NVME_NS_PREFIX		"ns"

#define DEVLIST_USAGE							       \
"devlist\n"

#define IDENTIFY_USAGE							       \
"identify [-x [-v]] <controller id|namespace id>\n"

#ifdef ENABLE_PREFTEST
#define PERFTEST_USAGE							       \
"perftest <-n num_threads> <-o read|write>\n"		       \
"                        <-s size_in_bytes> <-t time_in_seconds>\n"	       \
"                        <-i intr|wait> [-f refthread] [-p]\n"		       \
"                        <namespace id>\n"
#endif

#ifdef ENABLE_RESET
#define RESET_USAGE							       \
"reset <controller id>\n"
#endif

#define LOGPAGE_USAGE							       \
"logpage <-p page_id> [-b] [-v vendor] [-x] "		       \
    "<controller id|namespace id>\n"

#ifdef ENABLE_FIRMWARE
#define FIRMWARE_USAGE							       \
"firmware [-s slot] [-f path_to_firmware] [-a] <controller id>\n"
#endif

#define POWER_USAGE							       \
"power [-l] [-p new-state [-w workload-hint]] <controller id>\n"

#define WDC_USAGE							       \
"wdc cap-diag [-o path-templete]\n"

void devlist(int, char *[]) __dead;
void identify(int, char *[]) __dead;
#ifdef PERFTEST_USAGE
void perftest(int, char *[]) __dead;
#endif
#ifdef RESET_USAGE
void reset(int, char *[]) __dead;
#endif
void logpage(int, char *[]) __dead;
#ifdef FIRMWARE_USAGE
void firmware(int, char *[]) __dead;
#endif
void power(int, char *[]) __dead;
void wdc(int, char *[]) __dead;

int open_dev(const char *, int *, int, int);
void parse_ns_str(const char *, char *, int *);
void read_controller_data(int, struct nvm_identify_controller *);
void read_namespace_data(int, int, struct nvm_identify_namespace *);
void print_hex(void *, uint32_t);
void read_logpage(int, uint8_t, int, void *, uint32_t);
__dead void dispatch(int argc, char *argv[], const struct nvme_function *f);

/* Utility Routines */
void nvme_strvis(uint8_t *, int, const uint8_t *, int);
void print_bignum(const char *, uint64_t v[2], const char *);
uint64_t le48dec(const void *);

#endif	/* __NVMECTL_H__ */
