/* $NetBSD: apic.c,v 1.9 2019/06/13 07:28:17 msaitoh Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apic.c,v 1.9 2019/06/13 07:28:17 msaitoh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <machine/apicvar.h>

const char redirlofmt[] = "\177\20"
	"f\0\10vector\0"
	"f\10\3delmode\0"
	"b\13logical\0"
	"b\14pending\0"
	"b\15actlo\0"
	"b\16irrpending\0"
	"b\17level\0"
	"b\20masked\0"
	"f\22\1dest\0" "=\1self" "=\2all" "=\3all-others";

const char redirhifmt[] = "\177\20"
	"f\30\10target\0";

void
apic_format_redir(const char *where1, const char *where2, int idx,
		uint32_t redirhi, uint32_t redirlo)
{
	char buf[256];

	snprintb(buf, sizeof(buf), redirlofmt, redirlo);
	printf("%s: %s%d %s",
	    where1, where2, idx, buf);

	if ((redirlo & LAPIC_DEST_MASK) == 0) {
		snprintb(buf, sizeof(buf), redirhifmt, redirhi);
		printf(" %s", buf);
	}

	printf("\n");
}
