/*	$NetBSD: kloader.h,v 1.2.2.3 2002/03/16 15:57:39 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/bootinfo.h>

struct kloader_ops;
struct kloader_page_tag;
struct kloader_bootinfo;

typedef void kloader_bootfunc_t(struct kloader_bootinfo *,
    struct kloader_page_tag *);

struct kloader_ops {
	void (*jump)(kloader_bootfunc_t *, vaddr_t, struct kloader_bootinfo *,
	    struct kloader_page_tag *);
	kloader_bootfunc_t *boot;
	void (*reset)(void);
};

struct kloader_page_tag {
	u_int32_t next;
	u_int32_t src;
	u_int32_t dst;
	u_int32_t sz;
} __attribute__((__packed__, __aligned__(4)));

#define KLOADER_KERNELARGS_MAX		256

struct kloader_bootinfo {
	vaddr_t entry;
	int argc;
	char **argv;
	struct bootinfo bootinfo;		

	char _argbuf[KLOADER_KERNELARGS_MAX];
} __attribute__((__packed__, __aligned__(4)));

void __kloader_reboot_setup(struct kloader_ops *, const char *);
void kloader_reboot(void);
void kloader_bootinfo_set(struct kloader_bootinfo *, int, char *[],
    struct bootinfo *, int);
