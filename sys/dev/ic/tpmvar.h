/*	$NetBSD: tpmvar.h,v 1.6 2019/10/09 07:30:58 maxv Exp $	*/

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

#define TPM_API_VERSION		1

enum tpm_version {
	TPM_1_2,
	TPM_2_0
};

struct tpm_ioc_getinfo {
	uint32_t api_version;
	uint32_t tpm_version;
	uint32_t device_id;
	uint32_t device_rev;
	uint32_t device_caps;
};

#define TPM_IOC_GETINFO		_IOR ('N',  0, struct tpm_ioc_getinfo)

#ifdef _KERNEL

struct tpm_softc {
	device_t sc_dev;
	enum tpm_version sc_ver;
	kmutex_t sc_lock;
	bool sc_busy;

	int (*sc_init)(struct tpm_softc *);
	int (*sc_start)(struct tpm_softc *, int);
	int (*sc_read)(struct tpm_softc *, void *, size_t, size_t *, int);
	int (*sc_write)(struct tpm_softc *, const void *, size_t);
	int (*sc_end)(struct tpm_softc *, int, int);

	bus_space_tag_t sc_bt;
	bus_space_handle_t sc_bh;

	uint32_t sc_devid;
	uint32_t sc_rev;
	uint32_t sc_status;
	uint32_t sc_caps;
};

bool tpm_suspend(device_t, const pmf_qual_t *);
bool tpm_resume(device_t, const pmf_qual_t *);

int tpm_tis12_probe(bus_space_tag_t, bus_space_handle_t);
int tpm_tis12_init(struct tpm_softc *);
int tpm_tis12_start(struct tpm_softc *, int);
int tpm_tis12_read(struct tpm_softc *, void *, size_t, size_t *, int);
int tpm_tis12_write(struct tpm_softc *, const void *, size_t);
int tpm_tis12_end(struct tpm_softc *, int, int);

#endif
