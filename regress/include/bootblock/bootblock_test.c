/*	$NetBSD: bootblock_test.c,v 1.2 2006/05/10 19:07:22 mrg Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/bootblock.h>

#include <assert.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{

		/* mbr_sector */

	assert(512		== sizeof(struct mbr_sector));
	assert(16		== sizeof(struct mbr_partition));

	assert(MBR_BPB_OFFSET	== offsetof(struct mbr_sector, mbr_bpb));

	assert(MBR_BS_OFFSET	== offsetof(struct mbr_sector, mbr_bootsel));

	assert(440		== offsetof(struct mbr_sector, mbr_dsn));

	assert(446		== MBR_PART_OFFSET);
	assert(MBR_PART_OFFSET	== offsetof(struct mbr_sector, mbr_parts));

	assert(510		== MBR_MAGIC_OFFSET);
	assert(MBR_MAGIC_OFFSET	== offsetof(struct mbr_sector, mbr_magic));

	exit(0);
}
