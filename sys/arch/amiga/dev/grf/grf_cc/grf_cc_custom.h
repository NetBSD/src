/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
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
 *	$Id: grf_cc_custom.h,v 1.2 1994/01/29 07:00:17 chopps Exp $
 */

#if ! defined (_GRF_CC_CUSTOM_H)
#define _GRF_CC_CUSTOM_H

/* ECS stuff */
#define VARVBLANK	0x1000	/* Variable vertical blank enable */
#define LOLDIS		0x0800	/* long line disable */
#define CSCBLANKEN	0x0400	/* redirect composite sync */
#define VARVSYNC	0x0200	/* Variable vertical sync enable */
#define VARHSYNC	0x0100	/* Variable horizontal sync enable */
#define VARBEAM	0x0080	/* variable beam counter enable */
#define DISPLAYDUAL	0x0040	/* use UHRES pointer and standard pointers */
#define DISPLAYPAL	0x0020	/* set decodes to generate PAL display */
#define VARCSYNC	0x0010	/* Variable composite sync enable */
#define CSBLANK	0x0008	/* Composite blank out to CSY* pin */
#define CSYNCTRUE	0x0004	/* composite sync true signal */
#define VSYNCTRUE	0x0002	/* vertical sync true */
#define HSYNCTRUE	0x0001	/* horizontal sync true */

/* new defines for bplcon0 */
#define USE_BPLCON3	1

/* new defines for bplcon2 */
#define BPLCON2_ZDCTEN		(1<<10) /* colormapped genlock bit */
#define BPLCON2_ZDBPEN		(1<<11) /* use bitplane as genlock bits */
#define BPLCON2_ZDBPSEL0	(1<<12) /* three bits to select one */
#define BPLCON2_ZDBPSEL1	(1<<13) /* of 8 bitplanes in */
#define BPLCON2_ZDBPSEL2	(1<<14) /* ZDBPEN genlock mode */

/* defines for bplcon3 register */
#define BPLCON3_EXTBLNKEN	(1<<0)	/* external blank enable */
#define BPLCON3_EXTBLKZD	(1<<1)	/* external blank ored into trnsprncy */
#define BPLCON3_ZDCLKEN	(1<<2)	/* zd pin outputs a 14mhz clock*/
#define BPLCON3_BRDNTRAN	(1<<4)	/* border is opaque */
#define BPLCON3_BRDNBLNK	(1<<5)	/* border is opaque */

/* mixture of stuff. */
#define	STANDARD_NTSC_ROWS	262
#define	STANDARD_PAL_ROWS	312
#define	STANDARD_COLORCLOCKS	226
#define	STANDARD_DENISE_MAX	455
#define	STANDARD_DENISE_MIN	93
#define	STANDARD_NTSC_BEAMCON	( 0x0000 )
#define	STANDARD_PAL_BEAMCON	( DISPLAYPAL )
#define	SPECIAL_BEAMCON	( VARVBLANK | LOLDIS | VARVSYNC | VARHSYNC | VARBEAM | CSBLANK | VSYNCTRUE)

#define	MIN_NTSC_ROW	21
#define	MIN_PAL_ROW	29
#define	STANDARD_VIEW_X	0x81
#define	STANDARD_VIEW_Y	0x2C
#define	STANDARD_HBSTRT	0x06
#define	STANDARD_HSSTRT	0x0B
#define	STANDARD_HSSTOP	0x1C
#define	STANDARD_HBSTOP	0x2C
#define	STANDARD_VBSTRT	0x0122
#define	STANDARD_VSSTRT	0x02A6
#define	STANDARD_VSSTOP	0x03AA
#define	STANDARD_VBSTOP	0x1066

#endif /* _GRF_CC_CUSTOM_H */
