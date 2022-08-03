/*	$NetBSD: tpmvar.h,v 1.4.2.2 2022/08/03 16:00:47 martin Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	DEV_IC_TPMVAR_H
#define	DEV_IC_TPMVAR_H

#include <sys/types.h>

#define TPM_API_VERSION		1

enum tpm_version {
	TPM_1_2,
	TPM_2_0
};

enum itf_version {
	TIS_1_2,
	CRB
};

struct tpm_ioc_getinfo {
	uint32_t api_version;
	uint32_t tpm_version;
	uint32_t itf_version;
	uint32_t device_id;
	uint32_t device_rev;
	uint32_t device_caps;
};

#define TPM_IOC_GETINFO		_IOR ('N',  0, struct tpm_ioc_getinfo)

#ifdef _KERNEL

#include <sys/bus.h>
#include <sys/device_if.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>
#include <sys/workqueue.h>

struct tpm_softc;

struct tpm_intf {
	enum itf_version version;
	int (*probe)(bus_space_tag_t, bus_space_handle_t);
	int (*init)(struct tpm_softc *);
	int (*start)(struct tpm_softc *, int);
	int (*read)(struct tpm_softc *, void *, size_t, size_t *, int);
	int (*write)(struct tpm_softc *, const void *, size_t);
	int (*end)(struct tpm_softc *, int, int);
};

extern const struct tpm_intf tpm_intf_tis12;

struct tpm_softc {
	device_t sc_dev;
	enum tpm_version sc_ver;
	kmutex_t sc_lock;
	bool sc_busy;

	const struct tpm_intf *sc_intf;
	bus_space_tag_t sc_bt;
	bus_space_handle_t sc_bh;

	uint32_t sc_devid;
	uint32_t sc_rev;
	uint32_t sc_status;
	uint32_t sc_caps;

	struct krndsource sc_rnd;
	struct workqueue *sc_rndwq;
	struct work sc_rndwk;
	volatile unsigned sc_rndpending;
	bool sc_rnddisabled;
};

bool tpm_suspend(device_t, const pmf_qual_t *);
bool tpm_resume(device_t, const pmf_qual_t *);

#endif

#endif	/* DEV_IC_TPMVAR_H */
