/*	$NetBSD: lmc1982.h,v 1.1.4.2 2001/10/05 22:27:42 reinoud Exp $	*/

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
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
 *      This product includes software developed by Michael L. Hitch.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Sound registers and related definitions for ARM7500
 */

#ifndef _LMC1982_H_
#define _LMC1982_H_

/* IOCR IOC Control Register offset 0x000*/
#define IOCR_I2CD       0x01    /* I2C Data */
#define IOCR_I2CK       0x02    /* I2C Clock */
#define IOCR_AUDIO      0x08    /* audio controller data pin*/
#define IOCR_BUTTON     0x40    /* NOR  of 4 button pins */
#define IOCR_FLYBACK    0x80    /* crt flyback */

/*
 * LMC1982
 */
#define LID	0x20
#define LCLOCK	0x10

/*
 * stereo and volume control definition for NS LMC1982.
 */
#define VINPUTSEL	0x40
#define VLOUD		0x41
#define VBASS		0x42
#define VTREB		0x43
#define VLEFT		0x44
#define VRIGHT		0x45
#define VMODE		0x46
#define VDIN		0x47
 
#define VIN1		0x0
#define VIN2		0x1
#define VMUTE		0x3
 
#define VLOUDNESS	0x1
#define VENHANCEDSTEREO 0x2
 
#define VDBM12		0x0
#define VDBM6		0x3
#define VDBM0		0x6
#define VDBP6		0x9
#define VDBP12		0xc
 
#define VVOL0		0x0     /* zero attenuation */
#define VVOLM80		0x28    /* max attenuation */
 
#define VLMONO		0x4
#define VSTEREO		0x5
#define VRMONO		0x6

#ifndef _LOCORE
extern void volume_ctl __P((u_int, u_int));
extern void conv_jap __P((u_char *, int));
#endif

#endif /* _LMC1982_H_ */
