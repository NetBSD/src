/*	$NetBSD: machine_type.h,v 1.1 1997/04/16 21:12:29 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POWERPC_MACHINE_TYPE_H_
#define _POWERPC_MACHINE_TYPE_H_

/*
 * The following types provide the kernel with information on what
 * sort of machine we're running on.  This is passed at the end of
 * the boot arguments, which is a contiguous memory region which
 * looks like this:
 *
 *	/pci/scsi@3/disk@0,0:1/netbsd -s<NUL><esym><tag>
 *
 * The post-<NUL> values are:
 *
 *	<esym>	end of the kernel symbol table (32-bits)
 *
 *	<tag>	machine type tag (32-bits)
 *
 * The pre-<NUL> portion of the boot arguments may be interpreted
 * differently depending on the machine tag.
 *
 * If the tag indicates that the system has OpenFirmware, then
 * OpenFirmware will be used to get additional system information.
 *
 * If the tag indicates that the system has Motorola BUG, then
 * BUG will be used to get additional system information.
 */

#define	POWERPC_MACHINE_OPENFIRMWARE	0  /* system has OpenFirmware */
#define	POWERPC_MACHINE_MOTOROLABUG	1  /* system has Motorola BUG */
#define	POWERPC_MACHINE_BEBOX		2  /* system is a BeBox */
#define	POWERPC_MACHINE_NUBUSPOWERMAC	3  /* system is a Nubus PowerMac */
#define	POWERPC_MACHINE_NTYPES		4  /* add them to the end, please */

#endif /* _POWERPC_MACHINE_TYPE_H_ */
