/*	$NetBSD: preptype.h,v 1.1 2001/06/17 15:57:11 nonaka Exp $	*/

/*-
 * Copyright (C) 2001 NONAKA Kimihiro.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MACHINE_PREPTYPE_H__
#define __MACHINE_PREPTYPE_H__

#ifndef _STANDALONE
extern int prep_model;
#endif /* _STANDALONE */

/*
 * vendor
 */
#define	PREP_VENDOR_IBM		0x0000

#define	PREP_VENDOR(type)	(((type) >> 16) & 0xffff)

/*
 * product
 */
#define	PREP_UNKNOWN	-1

/* IBM */
#define	IBM_6050	0x00000001	/* IBM Personal Power Series 830/850 */
#define	IBM_7248	0x00000002	/* IBM RS600/43P-100,120,133 */
#define	IBM_6040	0x00000003	/* IBM ThinkPad 820 */

#define	IBM_6050_STR	"IBM PPS Model 6050/6070"
#define	IBM_7248_STR	"IBM PPS Model 7248"
#define	IBM_6040_STR	"IBM PPS Model 6040"

#endif /* __MACHINE_PREPTYPE_H__ */
