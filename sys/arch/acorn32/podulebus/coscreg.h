/* $NetBSD: coscreg.h,v 1.1 2001/10/05 22:27:54 reinoud Exp $ */

/*
 * Copyright (c) 1996 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *      for the NetBSD Project.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/*
 * Termination and DMA enable registers provided by
 * Andreas Gandor <andi@knipp.de>
 */

#ifndef _COSCREG_H_
#define _COSCREG_H_

#include <acorn32/podulebus/escvar.h>

typedef volatile unsigned short vu_short;

typedef struct cosc_regmap {
	esc_regmap_t	esc;
	vu_char		*chipreset;
	vu_char		*inten;
	vu_char		*status;
	vu_char		*term;
	vu_char		*led;
} cosc_regmap_t;
typedef cosc_regmap_t *cosc_regmap_p;

/*
#define COSC_CONTROL_CHIPRESET
#define	COSC_CONTROL_INTEN
#define COSC_STATUS
#define COSC_CONTROL_LED
*/

#define COSC_CONFIG_CONTROL_REG1	0x0ff0
#define COSC_CONFIG_CONTROL_REG1_MASK	0x97
#define COSC_CONFIG_CONTROL_REG2	0x0ff4
#define COSC_CONFIG_CONTROL_REG2_MASK	0x07
#define COSC_CONFIG_CONTROL_REG3	0x0ff8
#define COSC_CONFIG_CONTROL_REG3_MASK	0x83
#define COSC_CONFIG_CONTROL_REG4	0x0ffc
#define COSC_CONFIG_CONTROL_REG4_MASK	0xec
#define COSC_CONFIG_TERMINATION		0x1000
#define COSC_CONFIG_TERMINATION_ON	0x01
#define COSC_CONFIG_FIRST_SCANNED	0x1040	/* Used by RiscOS */
#define COSC_CONFIG_INC_FURTHER_PARTS	0x1048	/* Used by RiscOS */
#define COSC_CONFIG_SYNCH_MODE		0x1050
#define COSC_CONFIG_SYNCH_MODE_ON	0x01
#define COSC_CONFIG_SHUTDOWN		0x1054	/* Used by RiscOS */
#define COSC_CONFIG_SHUTDOWN_SPINDOWN	0x01	/* Used by RiscOS */
#define COSC_CONFIG_SHUTDOWN_EJECT	0x02	/* Used by RiscOS */
#define COSC_CONFIG_DELAY_REMOVABLE	0x105c	/* Used by RiscOS */
#define COSC_CONFIG_DELAY_HARDDISC	0x1060	/* Used by RiscOS */
#define COSC_CONFIG_DELAY_BOOT		0x1064	/* Used by RiscOS */
#define COSC_CONFIG_CDROM		0x1068	/* Used by RiscOS */

#define COSC_TERMINATION_CONTROL	0x2600
#define COSC_TERMINATION_ON		0x00
#define COSC_TERMINATION_OFF		0x01
#define COSC_REGISTER_00		0x2800
#define COSC_REGISTER_01		0x2a00
#define COSC_PAGE_REGISTER		0x3000

#define COSC_ESCOFFSET_BASE		0x3c00
#define COSC_ESCOFFSET_TCL		0x0000
#define COSC_ESCOFFSET_TCM		0x0004
#define COSC_ESCOFFSET_FIFO		0x0008
#define COSC_ESCOFFSET_COMMAND		0x000c
#define COSC_ESCOFFSET_DESTID		0x0010
#define COSC_ESCOFFSET_TIMEOUT		0x0014
#define COSC_ESCOFFSET_PERIOD		0x0018
#define COSC_ESCOFFSET_OFFSET		0x001c
#define COSC_ESCOFFSET_CONFIG1		0x0020
#define COSC_ESCOFFSET_CLOCKCONV	0x0024
#define COSC_ESCOFFSET_TEST		0x0028
#define COSC_ESCOFFSET_CONFIG2		0x002c
#define COSC_ESCOFFSET_CONFIG3		0x0030
#define COSC_ESCOFFSET_CONFIG4		0x0034
#define COSC_ESCOFFSET_TCH		0x0038
#define COSC_ESCOFFSET_FIFOBOTTOM	0x003c

#endif /* _COSCREG_H_ */
