/*	$NetBSD: ym2149reg.h,v 1.5.92.1 2009/05/13 17:16:22 jym Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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
 */

#ifndef _YM2149REG_H
#define _YM2149REG_H
/*
 * Yahama YM-2149 Programmable Sound Generator
 */

#define	YM2149	((struct ym2149 *)AD_SOUND)

struct ym2149 {
	volatile u_char	sdb[4];	/* use only the even bytes		*/
};

#define	sd_selr		sdb[0]	/* select register			*/
#define	sd_rdat		sdb[0]	/* read register data			*/
#define	sd_wdat		sdb[2]	/* write register data			*/

/*
 * Accessing the YM-2149 registers is indirect through ST-specific
 * circuitry by writing the register number into sd_selr.
 */
#define	YM_PA0		0	/* Period Channel A, bits 0-7		*/
#define	YM_PA1		1	/* Period Channel A, bits 8-11		*/
#define	YM_PB0		2	/* Period Channel B, bits 0-7		*/
#define	YM_PB1		3	/* Period Channel B, bits 8-11		*/
#define	YM_PC0		4	/* Period Channel C, bits 0-7		*/
#define	YM_PC1		5	/* Period Channel C, bits 8-11		*/
#define	YM_PNG		6	/* Period Noise Generator, bits 0-4	*/
#define	YM_MFR		7	/* Multi Function Register		*/
#define	YM_VA		8	/* Volume Channel A			*/
#define	YM_VB		9	/* Volume Channel B			*/
#define	YM_VC		10	/* Volume Channel C			*/
#define	YM_PE0		11	/* Period Envelope, bits 0-7		*/
#define	YM_PE1		12	/* Period Envelope, bits 8-15		*/
#define	YM_WFE		13	/* Wave Form Envelope			*/
#define	YM_IOA		14	/* I/O port A				*/
#define	YM_IOB		15	/* I/O port B				*/

/* bits in MFR: */
#define	SA_OFF		0x01	/* Sound Channel A off			*/
#define	SB_OFF		0x02	/* Sound Channel B off			*/
#define	SC_OFF		0x04	/* Sound Channel C off			*/
#define	NA_OFF		0x08	/* Noise Channel A off			*/
#define	NB_OFF		0x10	/* Noise Channel B off			*/
#define	NC_OFF		0x20	/* Noise Channel C off			*/
#define	PA_OUT		0x40	/* Port A for Output			*/
#define	PB_OUT		0x80	/* Port B for Output			*/

/* bits in Vx: */
#define	VOLUME		0x0F	/* 16 steps				*/
#define	ENVELOP		0x10	/* volume steered by envelope		*/

/* bits in WFE: */
#define	WF_HOLD		0x01	/* hold after one period		*/
#define	WF_ALTERNAT	0x02	/* up and down (no saw teeth)		*/
#define	WF_ATTACK	0x04	/* start up				*/
#define	WF_CONTINUE	0x08	/* multiple periods			*/

/* names for bits in Port A (ST specific): */
#define	PA_SIDEB	0x01	/* select floppy head - if double sided	*/
#define	PA_FLOP0	0x02	/* Drive Select Floppy 0		*/
#define	PA_FLOP1	0x04	/* Drive Select Floppy 1		*/
#define	PA_FDSEL	(PA_SIDEB|PA_FLOP0|PA_FLOP1)
#define	PA_SRTS		0x08	/* Serial RTS				*/
#define	PA_SDTR		0x10	/* Serial DTR				*/
#define	PA_PSTROBE	0x20	/* Parallel Strobe			*/
#define	PA_USER		0x40	/* Free Pin on Monitor Connector	*/
#define	PA_SER2		0x80	/* Choose between LAN or Ser2 port	*/

/*
 * Access macro's for Port A
 * Port A is defined as an output port. Reading the port does not work
 * reliably so keeping a 'soft-copy' seems to be the only way to make
 * things work.
 */
extern u_char	ym2149_ioa;	/* Soft-copy of port-A			*/

#define ym2149_write_ioport(port, value) {				\
	int	_os = splhigh();					\
									\
	YM2149->sd_selr = port;						\
	YM2149->sd_wdat = value;					\
	splx(_os);							\
}

#define ym2149_write_ioport2(port, value) {				\
	YM2149->sd_selr = port;						\
	YM2149->sd_wdat = value;					\
}

#define ym2149_fd_select(select) {					\
	int _s = splhigh();						\
									\
	ym2149_ioa = (ym2149_ioa & ~PA_FDSEL) | (select & PA_FDSEL);	\
	ym2149_write_ioport(YM_IOA, ym2149_ioa);			\
	splx(_s);							\
	}

#define ym2149_rts(set) {						\
	int _s = splhigh();						\
									\
	ym2149_ioa = set ? ym2149_ioa | PA_SRTS : ym2149_ioa & ~PA_SRTS;\
	ym2149_write_ioport(YM_IOA, ym2149_ioa);			\
	splx(_s);							\
	}

#define ym2149_dtr(set) {						\
	int _s = splhigh();						\
									\
	ym2149_ioa = set ? ym2149_ioa | PA_SDTR : ym2149_ioa & ~PA_SDTR;\
	ym2149_write_ioport(YM_IOA, ym2149_ioa);			\
	splx(_s);							\
	}

#define ym2149_strobe(set) {						\
	int _s = splhigh();						\
									\
	ym2149_ioa = set ? ym2149_ioa | PA_PSTROBE : ym2149_ioa & ~PA_PSTROBE;\
	ym2149_write_ioport(YM_IOA, ym2149_ioa);			\
	splx(_s);							\
	}

#define ym2149_ser2(set) {						\
	int _s = splhigh();						\
									\
	ym2149_ioa = set ? ym2149_ioa | PA_SER2 : ym2149_ioa & ~PA_SER2;\
	ym2149_write_ioport(YM_IOA, ym2149_ioa);			\
	splx(_s);							\
	}

#undef ym2149_write_ioport2

#ifdef _KERNEL
/*
 * Prototypes
 */
void ym2149_init(void);
#endif

#endif /*  _YM2149REG_H */
