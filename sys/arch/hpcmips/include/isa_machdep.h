/*	$NetBSD: isa_machdep.h,v 1.1.1.1 1999/09/16 12:23:22 takemura Exp $	*/

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */
typedef void *isa_chipset_tag_t;
#define VR_ISA_PORT_BASE 0x14000000
#define VR_ISA_PORT_SIZE 0x4000000
#define VR_ISA_MEM_BASE 0x10000000
#define VR_ISA_MEM_SIZE 0x4000000
/*
 * Functions provided to machine-independent ISA code.
 */
void isa_attach_hook __P((struct device*, struct device*, struct isabus_attach_args*));
int isa_intr_alloc __P((isa_chipset_tag_t, int, int, int*));
void *isa_intr_establish __P((isa_chipset_tag_t, int, int, int, int (*)(void *), void*));
void isa_intr_disestablish __P((isa_chipset_tag_t, void*));
