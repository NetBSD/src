/*	$NetBSD: efi.h,v 1.7 2022/08/20 10:55:27 riastradh Exp $	*/

/*-
 * Copyright (c) 2004 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _ARM_EFI_H_
#define _ARM_EFI_H_

#include <sys/uuid.h>

#include <dev/efi/efi.h>

#define	EFI_PAGE_SHIFT		12
#define	EFI_PAGE_SIZE		(1 << EFI_PAGE_SHIFT)
#define	EFI_PAGE_MASK		(EFI_PAGE_SIZE - 1)

#define	EFI_TABLE_ACPI20						\
	{0x8868e871,0xe4f1,0x11d3,0xbc,0x22,{0x00,0x80,0xc7,0x3c,0x88,0x81}}
#define	EFI_TABLE_ACPI10						\
	{0xeb9d2d30,0x2d88,0x11d3,0x9a,0x16,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define	EFI_TABLE_SMBIOS						\
	{0xeb9d2d31,0x2d88,0x11d3,0x9a,0x16,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define	EFI_TABLE_SMBIOS3						\
	{0xf2fd1544,0x9794,0x4a2c,0x99,0x2e,{0xe5,0xbb,0xcf,0x20,0xe3,0x94}}

extern const struct uuid EFI_UUID_ACPI20;
extern const struct uuid EFI_UUID_ACPI10;
extern const struct uuid EFI_UUID_SMBIOS;
extern const struct uuid EFI_UUID_SMBIOS3;

extern bool bootmethod_efi;

#endif /* _ARM_EFI_H_ */
