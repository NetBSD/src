/*	$NetBSD: bootinfo.h,v 1.1.10.2 2014/08/20 00:03:10 tls Exp $	*/

/*-
 * Copyright (c) 2014 Izumi Tsutsui.  All rights reserved.
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
 */

#ifndef __LUNA68K_BOOTINFO_H_
#define __LUNA68K_BOOTINFO_H_

/*
 * see also MAKEBOOTDEV macro in <sys/reboot.h>
 *
 * ADAPTOR:	SPC or LANCE
 * CONTROLLER:	SPC and LANCE unit number
 * UNIT:	SCSI ID of SPC devices (XXX: no LUN support)
 * PARTITION:	booted partition of the boot disk
 * TYPE:	unused
 */

#define LUNA68K_BOOTADPT_SPC	0
#define LUNA68K_BOOTADPT_LANCE	1

#define LUNA68K_BOOTADPT_SPC0_PA	0xe1000000
#define LUNA68K_BOOTADPT_SPC1_PA	0xe1000040
#define LUNA68K_BOOTADPT_LANCE0_PA	0xf1000000

#ifdef _KERNEL
extern uint32_t bootdev;
#endif

#endif /* __LUNA68K_BOOTINFO_H_ */
