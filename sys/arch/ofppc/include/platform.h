/*	$NetBSD: platform.h,v 1.1 2001/10/22 23:01:18 thorpej Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OFPPC_PLATFORM_H_
#define	_OFPPC_PLATFORM_H_

/*
 * The "platform" structure contains various routines that each
 * platform must supply.
 */
struct platform {
	void	(*cons_init)(void);
	void	(*device_register)(struct device *, void *);
};

/*
 * The "platmap" and "platinit" structures/arrays provide a means to
 * identify a platform, assign a symbolic ID to it, and initialize the
 * "platform" structure.
 */
struct platmap {
	const char *name;	/* name from OFW root node */
	int platid;
};

struct platinit {
	const char *option;	/* option name to enable platform support */
	void (*init)(void);	/* platform init routine */
};

#define	PLATID_UNKNOWN		0	/* unknown platform */

#define	PLATID_FIREPOWER_ES	1	/* Firepower ES */
#define	PLATID_FIREPOWER_MX	2	/* Firepower MX */
#define	PLATID_FIREPOWER_LX	3	/* Firepower LX */

#define	PLATID_TOTALIMPACT_BRIQ	4	/* Total Impact briQ */

#ifdef _KERNEL
extern struct platform platform;
extern char platform_name[];
extern int platid;

void	platform_init(void);
#endif /* _KERNEL */

#endif /* _OFPPC_PLATFORM_H_ */
