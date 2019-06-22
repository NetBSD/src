/*	$NetBSD: tpmvar.h,v 1.4 2019/06/22 12:57:41 maxv Exp $	*/

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

/*
 * Copyright (c) 2008, 2009 Michael Shalayeff
 * Copyright (c) 2009, 2010 Hans-Joerg Hoexer
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
	void *sc_ih;

	int (*sc_init)(struct tpm_softc *, int);
	int (*sc_start)(struct tpm_softc *, int);
	int (*sc_read)(struct tpm_softc *, void *, size_t, size_t *, int);
	int (*sc_write)(struct tpm_softc *, const void *, size_t);
	int (*sc_end)(struct tpm_softc *, int, int);

	bus_space_tag_t sc_bt, sc_batm;
	bus_space_handle_t sc_bh, sc_bahm;

	uint32_t sc_devid;
	uint32_t sc_rev;
	uint32_t sc_status;
	uint32_t sc_capabilities;

	int sc_flags;
#define	TPM_OPEN	0x0001

	int sc_vector;
};

int tpm_intr(void *);

bool tpm12_suspend(device_t, const pmf_qual_t *);
bool tpm12_resume(device_t, const pmf_qual_t *);

int tpm_tis12_probe(bus_space_tag_t, bus_space_handle_t);
int tpm_tis12_init(struct tpm_softc *, int);
int tpm_tis12_start(struct tpm_softc *, int);
int tpm_tis12_read(struct tpm_softc *, void *, size_t, size_t *, int);
int tpm_tis12_write(struct tpm_softc *, const void *, size_t);
int tpm_tis12_end(struct tpm_softc *, int, int);

#endif
