/*	$NetBSD: mkfs_msdos.h,v 1.1.2.2 2013/01/23 00:05:32 yamt Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
#include <stdbool.h>
#define ALLOPTS \
AOPT('@', off_t, offset, "Offset in device") \
AOPT('B', char *, bootstrap, "Bootstrap file") \
AOPT('C', off_t, create_size, "Create file") \
AOPT('F', uint8_t,  fat_type, "FAT type (12, 16, or 32)") \
AOPT('I', uint32_t, volume_id, "Volume ID") \
AOPT('L', char *, volume_label, "Volume Label") \
AOPT('N', bool, no_create, "Don't create filesystem, print params only") \
AOPT('O', char *, OEM_string, "OEM string") \
AOPT('S', uint16_t, bytes_per_sector, "Bytes per sector") \
AOPT('a', uint32_t, sectors_per_fat, "Sectors per FAT") \
AOPT('b', uint32_t, block_size, "Block size") \
AOPT('c', uint8_t, sectors_per_cluster, "Sectors per cluster") \
AOPT('e', uint16_t, directory_entries, "Directory entries") \
AOPT('f', char *, floppy, "Standard format floppies (160,180,320,360,640,720,1200,1232,1440,2880)") \
AOPT('h', uint16_t, drive_heads, "Drive heads") \
AOPT('i', uint16_t, info_sector, "Info sector") \
AOPT('k', uint16_t, backup_sector, "Backup sector") \
AOPT('m', uint8_t, media_descriptor, "Media descriptor") \
AOPT('n', uint8_t, num_FAT, "Number of FATs") \
AOPT('o', uint32_t, hidden_sectors, "Hidden sectors") \
AOPT('r', uint16_t, reserved_sectors, "Reserved sectors") \
AOPT('s', uint32_t, size, "File System size") \
AOPT('u', uint16_t, sectors_per_track, "Sectors per track")

struct msdos_options {
#define AOPT(a, b, c, d)	b c;
ALLOPTS
#undef AOPT	
	uint32_t volume_id_set:1;
	uint32_t media_descriptor_set:1;
	uint32_t hidden_sectors_set:1;
};

int mkfs_msdos(const char *, const char *, const struct msdos_options *);
