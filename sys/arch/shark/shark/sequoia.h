/*	$NetBSD: sequoia.h,v 1.1.14.2 2002/06/23 17:41:34 jdolecek Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**
**  MODULE DESCRIPTION:
**
**      National Sequoia-1 and Sequoia-2 Definitions 
**
**	Initial Register Write Values are identified by _INIT
**
**  CREATION DATE:
**
**      22-Jan-1997
**
**  AUTHOR:
**
**      Madeleine D. Robert
**
**  MODIFICATION HISTORY:
**
**	27-Jan-1997	Maddy		Added additional initial values.	
**
**	24-Jan-1997	Maddy		Initial Entry.
*/


#ifndef SEQUOIAH
#define SEQUOIAH

/****************************************************
**		      	SEQUOIA                    **
*****************************************************/
#define SEQUOIA_BASE          0x24
#define SEQUOIA_NPORTS        0x4                

#define SEQUOIA_INDEX         0x24
#define SEQUOIA_INDEX_OFFSET  0x0  

#define SEQUOIA_DATA          0x26
#define SEQUOIA_DATA_OFFSET   0x2


/****************************************************
**		      	SEQUOIA-1                  **
*****************************************************/

/*
** Power Management Control Registers
**
*/

/* 
**
**  PMC Clock Control Register (CCR) - Index 0x000
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     CNCLKSEL0	Conserve Clock Select 0
**        <1>     1     CNCLKSEL1	Conserve Clock Select 1
**        <2>     1     CNCLKSEL2	Conserve Clock Select 2
**        <3>     1     CNSRVEN		Conserve Mode Enable
**        <4>     1     SLCLKSEL0	Slow Clock Select 0
**        <5>     1     SLCLKSEL1	Slow Clock Select 1
**        <6>     1     SLCLKSEL2	Slow Clock Select 2
**	  <7>	  1     SLWCLKEN	Slow Clock Enable
**	  <8>	  1     SLPSLWEN	Sleep Mode, Slow Clock Enable
**	  <9>	  1     DZSLWEN		Doze Mode, Slow Clock Enable
**	  <10>	  1     SPNDSLWEN	Suspend Mode, Slow Clock Enable
**	  <11>	  1     LBSLWEN		Low Battery Slow Enable
**	  <12>	  1     DZONHALT	Doze on HALT
**	  <14>	  1     LBST		Low Battery Status
**	  <15>	  1     VLBST		Very Low Battery Status
**
**	  <13> Reserved
*/
#define PMC_CCR_REG		0x000
#define PMC_CCR_INIT		0x000
#define CCR_V_CNCLKSEL      	0
#define CCR_M_CNCLKSEL		0x7 << CCR_V_CNCLKSEL
#define CCR_V_CNCLKSEL0      	0
#define CCR_M_CNCLKSEL0		1 << CCR_V_CNCLKSEL0
#define CCR_V_CNCLKSEL1      	1
#define CCR_M_CNCLKSEL1		1 << CCR_V_CNCLKSEL1
#define CCR_V_CNCLKSEL2     	2
#define CCR_M_CNCLKSEL2		1 << CCR_V_CNCLKSEL2
#define CCR_V_CNSRVEN     	3
#define CCR_M_CNSRVEN		1 << CCR_V_CNSRVEN
#define CCR_V_SLCLKSEL     	4
#define CCR_M_SLCLKSEL		0x7 << CCR_V_SLCLKSEL
#define CCR_V_SLCLKSEL0     	4
#define CCR_M_SLCLKSEL0		1 << CCR_V_SLCLKSEL0
#define CCR_V_SLCLKSEL1     	5
#define CCR_M_SLCLKSEL1		1 << CCR_V_SLCLKSEL1
#define CCR_V_SLCLKSEL2     	6
#define CCR_M_SLCLKSEL2		1 << CCR_V_SLCLKSEL2
#define CCR_V_SLWCLKEN     	7
#define CCR_M_SLWCLKEN		1 << CCR_V_SLWCLKEN
#define CCR_V_SLPSLWEN     	8
#define CCR_M_SLPSLWEN		1 << CCR_V_SLPSLWEN
#define CCR_V_DZSLWEN     	9
#define CCR_M_DZSLWEN		1 << CCR_V_DZSLWEN
#define CCR_V_SPNDSLWEN     	10
#define CCR_M_SPNDSLWEN		1 << CCR_V_SPNDSLWEN
#define CCR_V_LBSLWEN     	11
#define CCR_M_LBSLWEN		1 << CCR_V_LBSLWEN
#define CCR_V_DZONHALT     	12
#define CCR_M_DZONHALT		1 << CCR_V_DZONHALT
#define CCR_V_LBST	     	14
#define CCR_M_LBST		1 << CCR_V_LBST
#define CCR_V_VLBST     	15
#define CCR_M_VLBST		1 << CCR_V_VLBST

/*
** Conserve Clock Selects
*/
#define CNCLKSEL_DIV1		0x000
#define CNCLKSEL_DIV2		CCR_M_CNCLKSEL0
#define CNCLKSEL_DIV4		CCR_M_CNCLKSEL1
#define CNCLKSEL_DIV8		(CCR_M_CNCLKSEL0 | CCR_M_CNCLKSEL1)
#define CNCLKSEL_DIV16		CCR_M_CNCLKSEL2
#define CNCLKSEL_DIV32		(CCR_M_CNCLKSEL0 | CCR_M_CNCLKSEL2)
#define CNCLKSEL_DIV64		(CCR_M_CNCLKSEL1 | CCR_M_CNCLKSEL2)

/*
** Slow Clock Selects
*/
#define SLCLKSEL_DIV1		0x000
#define SLCLKSEL_DIV2		CCR_M_SLCLKSEL0
#define SLCLKSEL_DIV4		CCR_M_SLCLKSEL1
#define SLCLKSEL_DIV8		(CCR_M_SLCLKSEL0 | CCR_M_SLCLKSEL1)
#define SLCLKSEL_DIV16		CCR_M_SLCLKSEL2
#define SLCLKSEL_DIV32		(CCR_M_SLCLKSEL0 | CCR_M_SLCLKSEL2)
#define SLCLKSEL_DIV64		(CCR_M_SLCLKSEL1 | CCR_M_SLCLKSEL2)
#define SLCLKSEL_STOPPED	(CCR_M_SLCLKSEL0 | CCR_M_SLCLKSEL1 | CCR_M_SLCLKSEL2)

/* 
**
**  Power Management Status Register (PMSR) - Index 0x001
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     WAKESRC0	Wake-up Source 0
**        <1>     1     WAKESRC1	Wake-up Source 1
**        <2>     1     WAKESRC2	Wake-up Source 2
**        <3>     1     ACPWR		AC Power
**        <4>     1     PMISRC0		Power Management Interrupt 0
**        <5>     1     PMISRC1		Power Management Interrupt 1
**        <6>     1     PMISRC2		Power Management Interrupt 2
**	  <7>	  1     PMISRC3		Power Management Interrupt 3
**	  <8>	  1     PMISRC4		Power Management Interrupt 4
**	  <9>	  1     RESUME		Resume
**	  <10>	  1     WAKE0STATUS	Wake 0 Status
**	  <11>	  1     WAKE1STATUS	Wake 1 Status
**	  <13>	  1     PMMD0		Power Management Mode 0
**	  <14>	  1     PMMD1		Power Management Mode 1
**	  <15>	  1     PMMD2		Power Management Mode 2
**
**	  <12> Reserved
*/
#define PMC_PMSR_REG		0x001
#define PMC_PMSR_INIT		( PMSR_M_WAKESRC | PMSR_M_PMISRC | PMMD_ON )
#define PMSR_V_WAKESRC		0
#define PMSR_M_WAKESRC		0x7 << PMSR_V_WAKESRC
#define PMSR_V_WAKESRC0		0
#define PMSR_M_WAKESRC0		1 << PMSR_V_WAKESRC0
#define PMSR_V_WAKESRC1		1
#define PMSR_M_WAKESRC1		1 << PMSR_V_WAKESRC1
#define PMSR_V_WAKESRC2		2
#define PMSR_M_WAKESRC2		1 << PMSR_V_WAKESRC2
#define PMSR_V_ACPWR		3
#define PMSR_M_ACPWR		1 << PMSR_V_ACPWR
#define PMSR_V_PMISRC		4
#define PMSR_M_PMISRC		0x1F << PMSR_V_PMISRC
#define PMSR_V_PMISRC0		4
#define PMSR_M_PMISRC0		1 << PMSR_V_PMISRC0
#define PMSR_V_PMISRC1		5
#define PMSR_M_PMISRC1		1 << PMSR_V_PMISRC1
#define PMSR_V_PMISRC2		6
#define PMSR_M_PMISRC2		1 << PMSR_V_PMISRC2
#define PMSR_V_PMISRC3		7
#define PMSR_M_PMISRC3		1 << PMSR_V_PMISRC3
#define PMSR_V_PMISRC4		8
#define PMSR_M_PMISRC4		1 << PMSR_V_PMISRC4
#define PMSR_V_RESUME		9
#define PMSR_M_RESUME		1 << PMSR_V_RESUME
#define PMSR_V_WAKE0STATUS	10
#define PMSR_M_WAKE0STATUS	1 << PMSR_V_WAKE0STATUS
#define PMSR_V_WAKE1STATUS	11
#define PMSR_M_WAKE1STATUS	1 << PMSR_V_WAKE1STATUS
#define PMSR_V_PMMD		13
#define PMSR_M_PMMD		0x7 << PMSR_V_PMMD
#define PMSR_V_PMMD0		13
#define PMSR_M_PMMD0		1 << PMSR_V_PMMD0
#define PMSR_V_PMMD1		14
#define PMSR_M_PMMD1		1 << PMSR_V_PMMD1
#define PMSR_V_PMMD2		15
#define PMSR_M_PMMD2		1 << PMSR_V_PMMD2

/*
** Wake-up Source
*/
#define WAKESRC_NONE		0x000
#define WAKESRC_RING		PMSR_M_WAKESRC0
#define WAKESRC_RTCI		PMSR_M_WAKESRC1
#define WAKESRC_SWTCH		(PMSR_M_WAKESRC0 | PMSR_M_WAKESRC1)
#define WAKESRC_GPTC		PMSR_M_WAKESRC2
#define WAKESRC_WAKE0		(PMSR_M_WAKESRC0 | PMSR_M_WAKESRC2)
#define WAKESRC_WAKE1		(PMSR_M_WAKESRC1 | PMSR_M_WAKESRC2)
#define WAKESRC_WAKECLR		(PMSR_M_WAKESRC0 | PMSR_M_WAKESRC1 | PMSR_M_WAKESRC2)

/*
** Power Management Interrupt Source
*/
#define PMISRC_NONE		0x000
#define PMISRC_IRQI		PMSR_M_PMISRC0
#define PMISRC_LBC		PMSR_M_PMISRC1
#define PMISRC_SUSTO		(PMSR_M_PMISRC0 | PMSR_M_PMISRC1)
#define PMISRC_SLPTO		PMSR_M_PMISRC2
#define PMISRC_DZTO		(PMSR_M_PMISRC0 | PMSR_M_PMISRC2)
#define PMISRC_GENTO		(PMSR_M_PMISRC1 | PMSR_M_PMISRC2)
#define PMISRC_GENACT		(PMSR_M_PMISRC0 | PMSR_M_PMISRC1 | PMSR_M_PMISRC2)
#define PMISRC_PRIACT		PMSR_M_PMISRC3
#define PMISRC_SCDACT		(PMSR_M_PMISRC0 | PMSR_M_PMISRC3)
#define PMISRC_SCDACTTO		(PMSR_M_PMISRC1 | PMSR_M_PMISRC3)
#define PMISRC_SWTCH		(PMSR_M_PMISRC0 | PMSR_M_PMISRC1 | PMSR_M_PMISRC3)
#define PMISRC_ACPWR		(PMSR_M_PMISRC2 | PMSR_M_PMISRC3)
#define PMISRC_PRGTTO		(PMSR_M_PMISRC0 | PMSR_M_PMISRC2 | PMSR_M_PMISRC3)
#define PMISRC_GPTCOMP		(PMSR_M_PMISRC1 | PMSR_M_PMISRC2 | PMSR_M_PMISRC3)
#define PMISRC_RTC		(PMSR_M_PMISRC0 | PMSR_M_PMISRC4)
#define PMISRC_RESCHPMI		(PMSR_M_PMISRC1 | PMSR_M_PMISRC4)
#define PMISRC_SWSMI		(PMSR_M_PMISRC0 | PMSR_M_PMISRC1 | PMSR_M_PMISRC4)

/*
** Power Management Mode
*/
#define PMMD_STDBY		0x000
#define PMMD_ON			PMSR_M_PMMD0
#define PMMD_CONSERVE		PMSR_M_PMMD1
#define PMMD_DOZE		(PMSR_M_PMMD0 | PMSR_M_PMMD1)
#define PMMD_SLEEP		PMSR_M_PMMD2
#define PMMD_SUSPEND		(PMSR_M_PMMD0 | PMSR_M_PMMD2)


/* 
**
**  Activity Source Register (ASR) - Index 0x002
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     VIDACTV		Video Active
**	  <1>	  1     HDACTV		Hard Disk Active
**	  <2>	  1     FLPACTV		Floppy Disk Active
**	  <3>	  1     KBDACTV		Keyboard Active
**	  <4>	  1     SIOACTV		Serial I/O Active
**	  <5>	  1     PIOACTV		Parallel I/O Active
**	  <6>	  1     PROG0ACTV	PRM 0 Active
**	  <7>	  1     PROG1ACTV	PRM 1 Active
**	  <8>	  1     PROG2ACTV	PRM 2 Active
**	  <9>	  1     PROG3ACTV	PRM 3 Active
**	  <12>	  1     EXT0ACTV	Primary Activity Mask EXTACT0
**	  <13>	  1     EXT1ACTV	Primary Activity Mask EXTACT1
**	  <14>	  1     EXT2ACTV	Primary Activity Mask EXTACT2
**	  <15>	  1     EXT3ACTV	Primary Activity Mask EXTACT3
**
**	  <10:11> Reserved
*/
#define PMC_ASR_REG		0x002
#define PMC_ASR_INIT		0x0000
#define ASR_V_VIDACTV		0
#define ASR_M_VIDACTV		1 << ASR_V_VIDACTV
#define ASR_V_HDACTV		1
#define ASR_M_HDACTV		1 << ASR_V_HDACTV
#define ASR_V_FLPACTV		2
#define ASR_M_FLPACTV		1 << ASR_V_FLPACTV
#define ASR_V_KBDACTV		3
#define ASR_M_KBDACTV		1 << ASR_V_KBDACTV
#define ASR_V_SIOACTV		4
#define ASR_M_SIOACTV		1 << ASR_V_SIOACTV
#define ASR_V_PIOACTV		5
#define ASR_M_PIOACTV		1 << ASR_V_PIOACTV
#define ASR_V_PROG0ACTV		6
#define ASR_M_PROG0ACTV		1 << ASR_V_PROG0ACTV
#define ASR_V_PROG1ACTV		7
#define ASR_M_PROG1ACTV		1 << ASR_V_PROG1ACTV
#define ASR_V_PROG2ACTV		8
#define ASR_M_PROG2ACTV		1 << ASR_V_PROG2ACTV
#define ASR_V_PROG3ACTV		9
#define ASR_M_PROG3ACTV		1 << ASR_V_PROG3ACTV
#define ASR_V_EXT0ACTV		12
#define ASR_M_EXT0ACTV		1 << ASR_V_EXT0ACTV
#define ASR_V_EXT1ACTV		13
#define ASR_M_EXT1ACTV		1 << ASR_V_EXT1ACTV
#define ASR_V_EXT2ACTV		14
#define ASR_M_EXT2ACTV		1 << ASR_V_EXT2ACTV
#define ASR_V_EXT3ACTV		15
#define ASR_M_EXT3ACTV		1 << ASR_V_EXT3ACTV

/* 
**
**  Primary Activity Mask Register (PAMR) - Index 0x003
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PAMSKVID	PAM Video Accesses
**        <1>     1     PAMSKHD		PAM Hard Disk Accesses
**        <2>     1     PAMSKFLP	PAM Floppy Accesses
**        <3>     1     PAMSKKDB	PAM Keyboard Accesses
**        <4>     1     PAMSKSIO	PAM Serial I/O Accesses
**        <5>     1     PAMSKPIO	PAM Parallel I/O Accesses
**        <6>     1     PAMSKPROG0	PAM Programmable Range 0
**        <7>     1     PAMSKPROG1	PAM Programmable Range 1
**        <8>     1     PAMSKPROG2	PAM Programmable Range 2
**        <9>     1     PAMSKPROG3	PAM Programmable Range 3
**        <12>    1     PAMSKEACT0	PAM EXTACT0 
**        <13>    1     PAMSKEACT1	PAM EXTACT1 
**        <14>    1     PAMSKEACT2	PAM EXTACT2 
**        <15>    1     PAMSKEACT3	PAM EXTACT3
**
**	  <10:11> Reserved
*/
#define PMC_PAMR_REG		0x003
#define PMC_PAMR_INIT		0x0000
#define PAMR_V_PAMSKVID		0
#define PAMR_M_PAMSKVID		1 << PAMR_V_PAMSKVID
#define PAMR_V_PAMSKHD		1
#define PAMR_M_PAMSKHD		1 << PAMR_V_PAMSKHD
#define PAMR_V_PAMSKFLP		2
#define PAMR_M_PAMSKFLP		1 << PAMR_V_PAMSKFLP
#define PAMR_V_PAMSKKDB		3
#define PAMR_M_PAMSKKDB		1 << PAMR_V_PAMSKKDB
#define PAMR_V_PAMSKSIO		4
#define PAMR_M_PAMSKSIO		1 << PAMR_V_PAMSKSIO
#define PAMR_V_PAMSKPIO		5
#define PAMR_M_PAMSKPIO		1 << PAMR_V_PAMSKPIO
#define PAMR_V_PAMSKPROG0	6
#define PAMR_M_PAMSKPROG0	1 << PAMR_V_PAMSKPROG0
#define PAMR_V_PAMSKPROG1	7
#define PAMR_M_PAMSKPROG1	1 << PAMR_V_PAMSKPROG1
#define PAMR_V_PAMSKPROG2	8
#define PAMR_M_PAMSKPROG2	1 << PAMR_V_PAMSKPROG2
#define PAMR_V_PAMSKPROG3	9
#define PAMR_M_PAMSKPROG3	1 << PAMR_V_PAMSKPROG3
#define PAMR_V_PAMSKEACT0	12
#define PAMR_M_PAMSKEACT0	1 << PAMR_V_PAMSKEACT0
#define PAMR_V_PAMSKEACT1	13
#define PAMR_M_PAMSKEACT1	1 << PAMR_V_PAMSKEACT1
#define PAMR_V_PAMSKEACT2	14
#define PAMR_M_PAMSKEACT2	1 << PAMR_V_PAMSKEACT2
#define PAMR_V_PAMSKEACT3	15                  
#define PAMR_M_PAMSKEACT3	1 << PAMR_V_PAMSKEACT3

/* 
**
**  PMI Mask Register (PMIMR) - Index 0x004
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     IMSKVID		PMI Mask Video Accesses
**        <1>     1     IMSKHD		PMI Mask Hard Disk Accesses
**        <2>     1     IMSKFLP		PMI Mask Floppy Accesses
**        <3>     1     IMSKKDB		PMI Mask Keyboard Accesses
**        <4>     1     IMSKSIO		PMI Mask Serial I/O Accesses
**        <5>     1     IMSKPIO		PMI Mask Parallel I/O Accesses
**        <6>     1     IMSKPROG0	PMI Mask Programmable Range 0
**        <7>     1     IMSKPROG1	PMI Mask Programmable Range 1
**        <8>     1     IMSKPROG2	PMI Mask Programmable Range 2
**        <9>     1     IMSKPROG3	PMI Mask Programmable Range 3
**        <12>    1     IMSKEACT0	PMI Mask EXTACT0 
**        <13>    1     IMSKEACT1	PMI Mask EXTACT1 
**        <14>    1     IMSKEACT2	PMI Mask EXTACT2 
**        <15>    1     IMSKEACT3	PMI Mask EXTACT3
**
**	  <10:11> Reserved
*/
#define PMC_PMIMR_REG		0x004
#define PMC_PMIMR_INIT		( PMIMR_M_IMSKVID   | PMIMR_M_IMSKHD    | \
				  PMIMR_M_IMSKFLP   | PMIMR_M_IMSKKDB   | \
				  PMIMR_M_IMSKSIO   | PMIMR_M_IMSKPIO   | \
				  PMIMR_M_IMSKPROG0 | PMIMR_M_IMSKPROG1 | \
				  PMIMR_M_IMSKPROG2 | PMIMR_M_IMSKPROG3 | \
				  PMIMR_M_IMSKEACT0 | PMIMR_M_IMSKEACT1 | \
				  PMIMR_M_IMSKEACT2 | PMIMR_M_IMSKEACT3 )
#define PMIMR_V_IMSKVID		0
#define PMIMR_M_IMSKVID		1 << PMIMR_V_IMSKVID
#define PMIMR_V_IMSKHD		1
#define PMIMR_M_IMSKHD		1 << PMIMR_V_IMSKHD
#define PMIMR_V_IMSKFLP		2
#define PMIMR_M_IMSKFLP		1 << PMIMR_V_IMSKFLP
#define PMIMR_V_IMSKKDB		3
#define PMIMR_M_IMSKKDB		1 << PMIMR_V_IMSKKDB
#define PMIMR_V_IMSKSIO		4
#define PMIMR_M_IMSKSIO		1 << PMIMR_V_IMSKSIO
#define PMIMR_V_IMSKPIO		5
#define PMIMR_M_IMSKPIO		1 << PMIMR_V_IMSKPIO
#define PMIMR_V_IMSKPROG0	6
#define PMIMR_M_IMSKPROG0	1 << PMIMR_V_IMSKPROG0
#define PMIMR_V_IMSKPROG1	7
#define PMIMR_M_IMSKPROG1	1 << PMIMR_V_IMSKPROG1
#define PMIMR_V_IMSKPROG2	8
#define PMIMR_M_IMSKPROG2	1 << PMIMR_V_IMSKPROG2
#define PMIMR_V_IMSKPROG3	9
#define PMIMR_M_IMSKPROG3	1 << PMIMR_V_IMSKPROG3
#define PMIMR_V_IMSKEACT0	12
#define PMIMR_M_IMSKEACT0	1 << PMIMR_V_IMSKEACT0
#define PMIMR_V_IMSKEACT1	13
#define PMIMR_M_IMSKEACT1	1 << PMIMR_V_IMSKEACT1
#define PMIMR_V_IMSKEACT2	14
#define PMIMR_M_IMSKEACT2	1 << PMIMR_V_IMSKEACT2
#define PMIMR_V_IMSKEACT3	15                  
#define PMIMR_M_IMSKEACT3	1 << PMIMR_V_IMSKEACT3

/* 
**
**  Heat Regulator Control Register (HRCR) - Index 0x005
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     HTRGDLY0	Heat Regulator Delay 0 
**        <1>     1     HTRGDLY1	Heat Regulator Delay 1 
**        <2>     1     HTRGDLY2	Heat Regulator Delay 2 
**        <3>     1     HTRGRAT0	Heat Regulator Ratio 0 
**        <4>     1     HTRGRAT1	Heat Regulator Ratio 1
**        <5>     1     HTRGRAT2	Heat Regulator Ratio 2 
**        <6>     1     HTRGEN		Heat Regulator Enable
**        <7>     1     HTRGLOCK	Heat Regulator Configuration Lock
**        <14>    1     FRCSLWEN	Force SLOW Enable
**        <15>    1     FRCSLWLOCK	Force Slow Lock
**
**	  <8:13> Reserved
*/
#define PMC_HRCR_REG		0x005
#define PMC_HRCR_INIT		0x0000
#define HRCR_V_HTRGDLY		0
#define HRCR_M_HTRGDLY		0x7 << HRCR_V_HTRGDLY
#define HRCR_V_HTRGDLY0		0
#define HRCR_M_HTRGDLY0		1 << HRCR_V_HTRGDLY0
#define HRCR_V_HTRGDLY1		1
#define HRCR_M_HTRGDLY1		1 << HRCR_V_HTRGDLY1
#define HRCR_V_HTRGDLY2		2
#define HRCR_M_HTRGDLY2		1 << HRCR_V_HTRGDLY2
#define HRCR_V_HTRGRAT		3
#define HRCR_M_HTRGRAT		0x7 << HRCR_V_HTRGRAT
#define HRCR_V_HTRGRAT0		3
#define HRCR_M_HTRGRAT0		1 << HRCR_V_HTRGRAT0
#define HRCR_V_HTRGRAT1		4
#define HRCR_M_HTRGRAT1		1 << HRCR_V_HTRGRAT1
#define HRCR_V_HTRGRAT2		5
#define HRCR_M_HTRGRAT2		1 << HRCR_V_HTRGRAT2
#define HRCR_V_HTRGEN		6
#define HRCR_M_HTRGEN		1 << HRCR_V_HTRGEN
#define HRCR_V_HTRGLOCK		7
#define HRCR_M_HTRGLOCK		1 << HRCR_V_HTRGLOCK
#define HRCR_V_FRCSLWEN		14
#define HRCR_M_FRCSLWEN		1 << HRCR_V_FRCSLWEN
#define HRCR_V_FRCSLWLOCK	15
#define HRCR_M_FRCSLWLOCK	1 << HRCR_V_FRCSLWLOCK

/*
** Heater Regulator Delay
*/
#define HTRGDLY_16SEC		0x000
#define HTRGDLY_32SEC		HRCR_M_HTRGDLY0
#define HTRGDLY_60SEC		HRCR_M_HTRGDLY1
#define HTRGDLY_120SEC		HRCR_M_HTRGDLY2
#define HTRGDLY_240SEC		(HRCR_M_HTRGDLY1 | HRCR_M_HTRGDLY2)
#define HTRGDLY_480SEC		(HRCR_M_HTRGDLY0 | HRCR_M_HTRGDLY1 | HRCR_M_HTRGDLY2)

/*
** Heater Regulator Ratio  (Percent)
*/
#define HTRGRAT_50		0x000  		  
#define HTRGRAT_66		HRCR_M_HTRGRAT0	  
#define HTRGRAT_80		HRCR_M_HTRGRAT1		
#define HTRGRAT_90		(HRCR_M_HTRGRAT0 | HRCR_M_HTRGRAT1) 

/* 
**
**  PMI Mask and Control Register (PMIMCR) - Index 0x006
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     IMSKSQ2PMI	Mask SEQUOIA-2 Source from PMI
**        <1>     1     IMSKLB		Mask Low Battery from PMI
**        <2>     1     IMSKSPNDTO	Mask Suspend Time-out from PMI
**        <3>     1     IMSKSLPTO	Mask Sleep Time-out from PMI
**        <4>     1     IMSKDZTO	Mask Doze Time-out from PMI
**        <5>     1     IMSKGENTO	Mask Generic Time-out from PMI
**        <6>     1     IMSKACTV	Mask Generic Activity from PMI
**        <7>     1     IMSKPACTV	Mask Primary Activity from PMI
**        <8>     1     IMSKSACTV	Mask Secondary Activity from PMI
**        <10>    1     IMSKSWSTBY	Mask SWTCH from PMI
**        <11>    1     IMSKACPWR	Mask ACPWR from PMI
**        <12>    1     IMSKPROGTO	Mask Programmable Time-out from PMI
**        <13>    1     IMSKGPTMR	Mask GP Timer from PMI
**
**	  <9><14:15> Reserved
*/
#define PMC_PMIMCR_REG		0x006
#define PMC_PMIMCR_INIT		( PMIMCR_M_IMSKSQ2PMI | PMIMCR_M_IMSKLB     | \
				  PMIMCR_M_IMSKSPNDTO | PMIMCR_M_IMSKSLPTO  | \
				  PMIMCR_M_IMSKDZTO   | PMIMCR_M_IMSKGENTO  | \
				  PMIMCR_M_IMSKACTV   | PMIMCR_M_IMSKPACTV  | \
				  PMIMCR_M_IMSKSACTV  | PMIMCR_M_IMSKSWSTBY | \
				  PMIMCR_M_IMSKACPWR  | PMIMCR_M_IMSKPROGTO | \
				  PMIMCR_M_IMSKGPTMR  )
#define PMIMCR_V_IMSKSQ2PMI	0
#define PMIMCR_M_IMSKSQ2PMI	1 << PMIMCR_V_IMSKSQ2PMI
#define PMIMCR_V_IMSKLB		1
#define PMIMCR_M_IMSKLB		1 << PMIMCR_V_IMSKLB
#define PMIMCR_V_IMSKSPNDTO	2
#define PMIMCR_M_IMSKSPNDTO	1 << PMIMCR_V_IMSKSPNDTO
#define PMIMCR_V_IMSKSLPTO	3
#define PMIMCR_M_IMSKSLPTO	1 << PMIMCR_V_IMSKSLPTO
#define PMIMCR_V_IMSKDZTO	4
#define PMIMCR_M_IMSKDZTO	1 << PMIMCR_V_IMSKDZTO
#define PMIMCR_V_IMSKGENTO	5
#define PMIMCR_M_IMSKGENTO	1 << PMIMCR_V_IMSKGENTO
#define PMIMCR_V_IMSKACTV	6
#define PMIMCR_M_IMSKACTV	1 << PMIMCR_V_IMSKACTV
#define PMIMCR_V_IMSKPACTV	7
#define PMIMCR_M_IMSKPACTV	1 << PMIMCR_V_IMSKPACTV
#define PMIMCR_V_IMSKSACTV	8
#define PMIMCR_M_IMSKSACTV	1 << PMIMCR_V_IMSKSACTV
#define PMIMCR_V_IMSKSWSTBY	10
#define PMIMCR_M_IMSKSWSTBY	1 << PMIMCR_V_IMSKSWSTBY
#define PMIMCR_V_IMSKACPWR	11
#define PMIMCR_M_IMSKACPWR	1 << PMIMCR_V_IMSKACPWR
#define PMIMCR_V_IMSKPROGTO	12
#define PMIMCR_M_IMSKPROGTO	1 << PMIMCR_V_IMSKPROGTO
#define PMIMCR_V_IMSKGPTMR	13
#define PMIMCR_M_IMSKGPTMR	1 << PMIMCR_V_IMSKGPTMR

/* 
**
**  General Purpose Control Register (GPCR) - Index 0x007
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     ACDISCNSRV	ACPWR Disable Conserve Mode
**        <1>     1     ACON		ACPWR On
**        <2>     1     REFRPRD0	Refresh Period 0
**        <3>     1     REFRPRD1	Refresh Period 1
**        <4>     1     REFRPRD2	Refresh Period 2
**        <5>     1     SLFREFEN	Self Refresh Enable 
**        <6>     1     RINGS0		Modem RING Select 0
**        <7>     1     RINGS1		Modem RING Select 1
**        <8>     1     GPIODATA0	General Purpose I/O Data 0
**        <9>     1     GPIODATA1	General Purpose I/O Data 1
**        <10>    1     GPIODATA2	General Purpose I/O Data 2
**        <11>    1     GPIODATA3	General Purpose I/O Data 3
**        <12>    1     GPIODIR0	General Purpose I/O Direction 0
**        <13>    1     GPIODIR1	General Purpose I/O Direction 1
**        <14>    1     GPIODIR2	General Purpose I/O Direction 2
**        <15>    1     GPIODIR3	General Purpose I/O Direction 3
**
*/
#define PMC_GPCR_REG		0x007
#define PMC_GPCR_INIT		( REFRPRD_STOPPED | GPCR_M_GPIODIR0 | \
				  GPCR_M_GPIODIR1 )
#define GPCR_V_ACDISCNSRV	0
#define GPCR_M_ACDISCNSRV	1 << GPCR_V_ACDISCNSRV
#define GPCR_V_ACON		1
#define GPCR_M_ACON		1 << GPCR_V_ACON
#define GPCR_V_REFRPRD		2
#define GPCR_M_REFRPRD		0x7 << GPCR_V_REFRPRD
#define GPCR_V_REFRPRD0		2
#define GPCR_M_REFRPRD0		1 << GPCR_V_REFRPRD0
#define GPCR_V_REFRPRD1		3
#define GPCR_M_REFRPRD1		1 << GPCR_V_REFRPRD1
#define GPCR_V_REFRPRD2		4
#define GPCR_M_REFRPRD2		1 << GPCR_V_REFRPRD2
#define GPCR_V_SLFREFEN		5
#define GPCR_M_SLFREFEN		1 << GPCR_V_SLFREFEN
#define GPCR_V_RINGS		6
#define GPCR_M_RINGS		0x3 << GPCR_V_RINGS
#define GPCR_V_GPIODATA		8
#define GPCR_M_GPIODATA		0xF << GPCR_V_GPIODATA
#define GPCR_V_GPIODATA0	8
#define GPCR_M_GPIODATA0	1 << GPCR_V_GPIODATA0
#define GPCR_V_GPIODATA1	9
#define GPCR_M_GPIODATA1	1 << GPCR_V_GPIODATA1
#define GPCR_V_GPIODATA2	10
#define GPCR_M_GPIODATA2	1 << GPCR_V_GPIODATA2
#define GPCR_V_GPIODATA3	11
#define GPCR_M_GPIODATA3	1 << GPCR_V_GPIODATA3
#define GPCR_V_GPIODIR		12
#define GPCR_M_GPIODIR		0xF << GPCR_V_GPIODIR
#define GPCR_V_GPIODIR0		12
#define GPCR_M_GPIODIR0		1 << GPCR_V_GPIODIR0
#define GPCR_V_GPIODIR1		13
#define GPCR_M_GPIODIR1		1 << GPCR_V_GPIODIR1
#define GPCR_V_GPIODIR2		14
#define GPCR_M_GPIODIR2		1 << GPCR_V_GPIODIR2
#define GPCR_V_GPIODIR3		15
#define GPCR_M_GPIODIR3		1 << GPCR_V_GPIODIR3

/*
** Refresh Period
*/
#define REFRPRD_15US		0x000  		  
#define REFRPRD_30US		GPCR_M_REFRPRD0
#define REFRPRD_120US		GPCR_M_REFRPRD1
#define REFRPRD_STOPPED		GPCR_M_REFRPRD2
#define REFRPRD_60US		(GPCR_M_REFRPRD1 | GPCR_M_REFRPRD2)

/* 
**
**  Stop Clock Control Register (SCCR) - Index 0x008
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     STPGLBEN	Stop Clock Global Enable
**        <1>     1     MORESTOP	More Stop Enable
**        <2>     1     STPRELDLY0	STPCLK Release Delay 0
**        <3>     1     STPRELDLY1	STPCLK Release Delay 1
**        <4>     1     GRNTDLY0	STPCLK Latency Delay Select 0
**        <5>     1     GRNTDLY1	STPCLK Latency Delay Select 1
**        <6>     1     WMSKINTR	Wake Mask INTR
**	  <7>	  1     WAIT4GRNT	Wait for Stop Grant
**	  <9>	  1     IOTRAPDIS	I/O Trap Disable
**	  <10>	  1     SVNCLKPLS	SMI PULSE Width Select
**	  <11>	  1     PCSTGSEL0	PC Staggering Select 0
**	  <12>	  1     PCSTGSEL1	PC Staggering Select 1
**	  <13>	  1     PCSTGDIS	PC Stagger Disable
**	  <14>	  1     PCSLWEN		PC Slow Enable
**
**	  <8><15> Reserved
*/
#define PMC_SCCR_REG		0x008
#define PMC_SCCR_INIT		SCCR_M_WMSKINTR
#define SCCR_V_STPGLBEN		0
#define SCCR_M_STPGLBEN		1 << SCCR_V_STPGLBEN
#define SCCR_V_MORESTOP		1
#define SCCR_M_MORESTOP		1 << SCCR_V_MORESTOP
#define SCCR_V_STPRELDLY	2
#define SCCR_M_STPRELDLY	0x3 << SCCR_V_STPRELDLY
#define SCCR_V_STPRELDLY0	2
#define SCCR_M_STPRELDLY0	1 << SCCR_V_STPRELDLY0
#define SCCR_V_STPRELDLY1	3
#define SCCR_M_STPRELDLY1	1 << SCCR_V_STPRELDLY1
#define SCCR_V_GRNTDLY		4
#define SCCR_M_GRNTDLY		0x3 << SCCR_V_GRNTDLY
#define SCCR_V_GRNTDLY0		4
#define SCCR_M_GRNTDLY0		1 << SCCR_V_GRNTDLY0
#define SCCR_V_GRNTDLY1		5
#define SCCR_M_GRNTDLY1		1 << SCCR_V_GRNTDLY1
#define SCCR_V_WMSKINTR		6
#define SCCR_M_WMSKINTR		1 << SCCR_V_WMSKINTR
#define SCCR_V_WAIT4GRNT	7
#define SCCR_M_WAIT4GRNT	1 << SCCR_V_WAIT4GRNT
#define SCCR_V_IOTRAPDIS	9
#define SCCR_M_IOTRAPDIS	1 << SCCR_V_IOTRAPDIS
#define SCCR_V_SVNCLKPLS	10
#define SCCR_M_SVNCLKPLS	1 << SCCR_V_SVNCLKPLS
#define SCCR_V_PCSTGSEL		11
#define SCCR_M_PCSTGSEL		0x3 << SCCR_V_PCSTGSEL
#define SCCR_V_PCSTGSEL0	11
#define SCCR_M_PCSTGSEL0	1 << SCCR_V_PCSTGSEL0
#define SCCR_V_PCSTGSEL1	12
#define SCCR_M_PCSTGSEL1	1 << SCCR_V_PCSTGSEL1
#define SCCR_V_PCSTGDIS         13
#define SCCR_M_PCSTGDIS		1 << SCCR_V_PCSTGDIS
#define SCCR_V_PCSLWEN		14
#define SCCR_M_PCSLWEN		1 << SCCR_V_PCSLWEN

/*
** STPCLK Release Delay
*/
#define STPRELDLY_0S		0x000  		  
#define STPRELDLY_1US		SCCR_M_STPRELDLY0
#define STPRELDLY_45US		SCCR_M_STPRELDLY1
#define STPRELDLY_1MS		(SCCR_M_STPRELDLY0 | SCCR_M_STPRELDLY1)

/*
** STPCLK Release Delay Select
*/
#define GRNTDLY_GRANTBUS        0x000
#define GRNTDLY_30US		SCCR_M_GRNTDLY0
#define GRNTDLY_125US		SCCR_M_GRNTDLY1
#define GRNTDLY_1MS		(SCCR_M_GRNTDLY0 | SCCR_M_GRNTDLY1)

/*
** PC Staggering Select
*/
#define PCSTGSEL_240US		0x000
#define PCSTGSEL_4US		SCCR_M_PCSTGSEL0
#define PCSTGSEL_16US		SCCR_M_PCSTGSEL1
#define PCSTGSEL_64MS		(SCCR_M_PCSTGSEL0 | SCCR_M_PCSTGSEL1)

/* 
**
**  Fully-On Mode Power Control Register (FOMPCR) - Index 0x009
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PCON0		Power Control Fully-On Mode 0
**        <1>     1     PCON1		Power Control Fully-On Mode 1
**        <2>     1     PCON2		Power Control Fully-On Mode 2
**        <3>     1     PCON3		Power Control Fully-On Mode 3
**        <4>     1     PCON4		Power Control Fully-On Mode 4
**        <5>     1     PCON5		Power Control Fully-On Mode 5
**        <6>     1     PCON6		Power Control Fully-On Mode 6
**        <7>     1     PCON7		Power Control Fully-On Mode 7
**        <8>     1     PCON8		Power Control Fully-On Mode 8
**        <9>     1     PCON9		Power Control Fully-On Mode 9
**	  <10>	  1     GLBLPCEN	Global Power Control Enable
**
**	  <11:15> Reserved
*/
#define PMC_FOMPCR_REG		0x009
#define PMC_FOMPCR_INIT		( FOMPCR_M_PCON0 | FOMPCR_M_PCON1 )
#define FOMPCR_V_PCON		0
#define FOMPCR_M_PCON		0x3FF << FOMPCR_V_PCON
#define FOMPCR_V_PCON0		0
#define FOMPCR_M_PCON0		1 << FOMPCR_V_PCON0
#define FOMPCR_V_PCON1		1
#define FOMPCR_M_PCON1		1 << FOMPCR_V_PCON1
#define FOMPCR_V_PCON2		2                 
#define FOMPCR_M_PCON2		1 << FOMPCR_V_PCON2
#define FOMPCR_V_PCON3		3
#define FOMPCR_M_PCON3		1 << FOMPCR_V_PCON3
#define FOMPCR_V_PCON4		4
#define FOMPCR_M_PCON4		1 << FOMPCR_V_PCON4
#define FOMPCR_V_PCON5		5
#define FOMPCR_M_PCON5		1 << FOMPCR_V_PCON5
#define FOMPCR_V_PCON6		6
#define FOMPCR_M_PCON6		1 << FOMPCR_V_PCON6
#define FOMPCR_V_PCON7		7
#define FOMPCR_M_PCON7		1 << FOMPCR_V_PCON7
#define FOMPCR_V_PCON8		8
#define FOMPCR_M_PCON8		1 << FOMPCR_V_PCON8
#define FOMPCR_V_PCON9		9
#define FOMPCR_M_PCON9		1 << FOMPCR_V_PCON9
#define FOMPCR_V_GLBLPCEN	10
#define FOMPCR_M_GLBLPCEN	1 << FOMPCR_V_GLBLPCEN

/* 
**
**  Doze Mode Power Control Register (DZMPCR) - Index 0x00A
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PCDZ0		Power Control Doze Mode 0
**        <1>     1     PCDZ1		Power Control Doze Mode 1
**        <2>     1     PCDZ2		Power Control Doze Mode 2
**        <3>     1     PCDZ3		Power Control Doze Mode 3
**        <4>     1     PCDZ4		Power Control Doze Mode 4
**        <5>     1     PCDZ5		Power Control Doze Mode 5
**        <6>     1     PCDZ6		Power Control Doze Mode 6
**        <7>     1     PCDZ7		Power Control Doze Mode 7
**        <8>     1     PCDZ8		Power Control Doze Mode 8
**        <9>     1     PCDZ9		Power Control Doze Mode 9
**	  <10>	  1     SPNDDLY		Suspend Delay
**	  <11>	  1     STRTDLY		Start Delay
**
**	  <12:15> Reserved
*/
#define PMC_DZMPCR_REG		0x00A
#define PMC_DZMPCR_INIT		( DZMPCR_M_PCDZ0 | DZMPCR_M_PCDZ1 | \
				  DZMPCR_M_PCDZ2 )
#define DZMPCR_V_PCDZ		0
#define DZMPCR_M_PCDZ		0x3FF << DZMPCR_V_PCDZ
#define DZMPCR_V_PCDZ0		0
#define DZMPCR_M_PCDZ0		1 << DZMPCR_V_PCDZ0
#define DZMPCR_V_PCDZ1		1
#define DZMPCR_M_PCDZ1		1 << DZMPCR_V_PCDZ1
#define DZMPCR_V_PCDZ2		2                 
#define DZMPCR_M_PCDZ2		1 << DZMPCR_V_PCDZ2
#define DZMPCR_V_PCDZ3		3
#define DZMPCR_M_PCDZ3		1 << DZMPCR_V_PCDZ3
#define DZMPCR_V_PCDZ4		4
#define DZMPCR_M_PCDZ4		1 << DZMPCR_V_PCDZ4
#define DZMPCR_V_PCDZ5		5
#define DZMPCR_M_PCDZ5		1 << DZMPCR_V_PCDZ5
#define DZMPCR_V_PCDZ6		6
#define DZMPCR_M_PCDZ6		1 << DZMPCR_V_PCDZ6
#define DZMPCR_V_PCDZ7		7
#define DZMPCR_M_PCDZ7		1 << DZMPCR_V_PCDZ7
#define DZMPCR_V_PCDZ8		8
#define DZMPCR_M_PCDZ8		1 << DZMPCR_V_PCDZ8
#define DZMPCR_V_PCDZ9		9
#define DZMPCR_M_PCDZ9		1 << DZMPCR_V_PCDZ9
#define DZMPCR_V_SPNDDLY	10
#define DZMPCR_M_SPNDDLY	1 << DZMPCR_V_SPNDDLY
#define DZMPCR_V_STRTDLY	11
#define DZMPCR_M_STRTDLY	1 << DZMPCR_V_STRTDLY

/* 
**
**  Sleep Mode Power Control Register (SLPMPCR) - Index 0x00B
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PCSLP0		Power Control Sleep Mode 0
**        <1>     1     PCSLP1		Power Control Sleep Mode 1
**        <2>     1     PCSLP2		Power Control Sleep Mode 2
**        <3>     1     PCSLP3		Power Control Sleep Mode 3
**        <4>     1     PCSLP4		Power Control Sleep Mode 4
**        <5>     1     PCSLP5		Power Control Sleep Mode 5
**        <6>     1     PCSLP6		Power Control Sleep Mode 6
**        <7>     1     PCSLP7		Power Control Sleep Mode 7
**        <8>     1     PCSLP8		Power Control Sleep Mode 8
**        <9>     1     PCSLP9		Power Control Sleep Mode 9
**	  <10>	  1     LBLEDFLSH	Low Battery LED Flasher Enable
**	  <11>	  1     LBFLSHRAT0	Low Battery LED Flash Rate Selects 0
**	  <12>	  1     LBFLSHRAT1	Low Battery LED Flash Rate Selects 1
**	  <13>	  1     LBFLSHDUR0	Low Battery LED Flash Rate Duration 0
**	  <14>	  1     LBFLSHDUR1	Low Battery LED Flash Rate Duration 1
**	  <15>	  1     VLBFLSHEN	Very Low Battery Flash Enable
**
*/
#define PMC_SLPMPCR_REG		0x00B
#define PMC_SLPMPCR_INIT	( SLPMPCR_M_PCSLP0  | SLPMPCR_M_PCSLP1 | \
				  SLPMPCR_M_PCSLP2  | LBFLSHRAT_2HZ    | \
				  LBFLSHDUR_62PT5MS )
#define SLPMPCR_V_PCSLP		0
#define SLPMPCR_M_PCSLP		0x3FF << SLPMPCR_V_PCSLP
#define SLPMPCR_V_PCSLP0	0
#define SLPMPCR_M_PCSLP0	1 << SLPMPCR_V_PCSLP0
#define SLPMPCR_V_PCSLP1	1
#define SLPMPCR_M_PCSLP1	1 << SLPMPCR_V_PCSLP1
#define SLPMPCR_V_PCSLP2	2                 
#define SLPMPCR_M_PCSLP2	1 << SLPMPCR_V_PCSLP2
#define SLPMPCR_V_PCSLP3	3
#define SLPMPCR_M_PCSLP3	1 << SLPMPCR_V_PCSLP3
#define SLPMPCR_V_PCSLP4	4
#define SLPMPCR_M_PCSLP4	1 << SLPMPCR_V_PCSLP4
#define SLPMPCR_V_PCSLP5	5
#define SLPMPCR_M_PCSLP5	1 << SLPMPCR_V_PCSLP5
#define SLPMPCR_V_PCSLP6	6
#define SLPMPCR_M_PCSLP6	1 << SLPMPCR_V_PCSLP6
#define SLPMPCR_V_PCSLP7	7
#define SLPMPCR_M_PCSLP7	1 << SLPMPCR_V_PCSLP7
#define SLPMPCR_V_PCSLP8	8
#define SLPMPCR_M_PCSLP8	1 << SLPMPCR_V_PCSLP8
#define SLPMPCR_V_PCSLP9	9
#define SLPMPCR_M_PCSLP9	1 << SLPMPCR_V_PCSLP9
#define SLPMPCR_V_LBLEDFLSH	10
#define SLPMPCR_M_LBLEDFLSH	1 << SLPMPCR_V_LBLEDFLSH
#define SLPMPCR_V_LBFLSHRAT	11
#define SLPMPCR_M_LBFLSHRAT	0x3 << SLPMPCR_V_LBFLSHRAT
#define SLPMPCR_V_LBFLSHRAT0	11
#define SLPMPCR_M_LBFLSHRAT0	1 << SLPMPCR_V_LBFLSHRAT0
#define SLPMPCR_V_LBFLSHRAT1	12
#define SLPMPCR_M_LBFLSHRAT1	1 << SLPMPCR_V_LBFLSHRAT1
#define SLPMPCR_V_LBFLSHDUR	13
#define SLPMPCR_M_LBFLSHDUR	0x3 << SLPMPCR_V_LBFLSHDUR
#define SLPMPCR_V_LBFLSHDUR0	13
#define SLPMPCR_M_LBFLSHDUR0	1 << SLPMPCR_V_LBFLSHDUR0
#define SLPMPCR_V_LBFLSHDUR1	14
#define SLPMPCR_M_LBFLSHDUR1	1 << SLPMPCR_V_LBFLSHDUR1
#define SLPMPCR_V_VLBFLSHEN	15
#define SLPMPCR_M_VLBFLSHEN	1 << SLPMPCR_V_VLBFLSHEN

/*
** Low Battery LED Flash Rate Selects
*/
#define LBFLSHRAT_HLFHZ		0x000
#define LBFLSHRAT_1HZ		SLPMPCR_M_LBFLSHRAT0
#define LBFLSHRAT_2HZ		SLPMPCR_M_LBFLSHRAT1
#define LBFLSHRAT_4HZ		(SLPMPCR_M_LBFLSHRAT0 | SLPMPCR_M_LBFLSHRAT1)

/*
** Low Battery LED Flash Rate Duration	(milliseconds)
*/
#define LBFLSHDUR_256MS		0x000
#define LBFLSHDUR_128MS		SLPMPCR_M_LBFLSHDUR0
#define LBFLSHDUR_62PT5MS	SLPMPCR_M_LBFLSHDUR1
#define LBFLSHDUR_31PT25MS	(SLPMPCR_M_LBFLSHDUR0 | SLPMPCR_M_LBFLSHDUR1)


/* 
**
**  Suspend Mode Power Control Register (SPNDMPCR) - Index 0x00C
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PCSPND0		Power Control Suspend Mode 0
**        <1>     1     PCSPND1		Power Control Suspend Mode 1
**        <2>     1     PCSPND2		Power Control Suspend Mode 2
**        <3>     1     PCSPND3		Power Control Suspend Mode 3
**        <4>     1     PCSPND4		Power Control Suspend Mode 4
**        <5>     1     PCSPND5		Power Control Suspend Mode 5
**        <6>     1     PCSPND6		Power Control Suspend Mode 6
**        <7>     1     PCSPND7		Power Control Suspend Mode 7
**        <8>     1     PCSPND8		Power Control Suspend Mode 8
**        <9>     1     PCSPND9		Power Control Suspend Mode 9
**	  <10>	  1     SPLEDFLSH	Suspend LED Flasher Enable
**	  <11>	  1     SPFLSHRAT0	Suspend LED Flash Rate Selects 0
**	  <12>	  1     SPFLSHRAT1	Suspend LED Flash Rate Selects 1
**	  <13>	  1     SPFLSHDUR0	Suspend LED Flash Rate Duration 0
**	  <14>	  1     SPFLSHDUR1	Suspend LED Flash Rate Duration 1
**
**	  <15> Reserved
**	
*/
#define PMC_SPNDMPCR_REG	0x00c
#define PMC_SPNDMPCR_INIT	( SPNDMPCR_M_PCSPND0 | SPNDMPCR_M_PCSPND1 | \
				  SPNDMPCR_M_PCSPND2 | SPFLSHRAT_2HZ      | \
				  SPFLSHDUR_62PT5MS  )
#define SPNDMPCR_V_PCSPND	0
#define SPNDMPCR_M_PCSPND	0x3FF << SPNDMPCR_V_PCSPND
#define SPNDMPCR_V_PCSPND0	0
#define SPNDMPCR_M_PCSPND0	1 << SPNDMPCR_V_PCSPND0
#define SPNDMPCR_V_PCSPND1	1
#define SPNDMPCR_M_PCSPND1	1 << SPNDMPCR_V_PCSPND1
#define SPNDMPCR_V_PCSPND2	2                 
#define SPNDMPCR_M_PCSPND2	1 << SPNDMPCR_V_PCSPND2
#define SPNDMPCR_V_PCSPND3	3
#define SPNDMPCR_M_PCSPND3	1 << SPNDMPCR_V_PCSPND3
#define SPNDMPCR_V_PCSPND4	4
#define SPNDMPCR_M_PCSPND4	1 << SPNDMPCR_V_PCSPND4
#define SPNDMPCR_V_PCSPND5	5
#define SPNDMPCR_M_PCSPND5	1 << SPNDMPCR_V_PCSPND5
#define SPNDMPCR_V_PCSPND6	6
#define SPNDMPCR_M_PCSPND6	1 << SPNDMPCR_V_PCSPND6
#define SPNDMPCR_V_PCSPND7	7
#define SPNDMPCR_M_PCSPND7	1 << SPNDMPCR_V_PCSPND7
#define SPNDMPCR_V_PCSPND8	8
#define SPNDMPCR_M_PCSPND8	1 << SPNDMPCR_V_PCSPND8
#define SPNDMPCR_V_PCSPND9	9
#define SPNDMPCR_M_PCSPND9	1 << SPNDMPCR_V_PCSPND9
#define SPNDMPCR_V_SPLEDFLSH	10
#define SPNDMPCR_M_SPLEDFLSH	1 << SPNDMPCR_V_SPLEDFLSH
#define SPNDMPCR_V_SPFLSHRAT	11
#define SPNDMPCR_M_SPFLSHRAT	0x3 << SPNDMPCR_V_SPFLSHRAT
#define SPNDMPCR_V_SPFLSHRAT0	11
#define SPNDMPCR_M_SPFLSHRAT0	1 << SPNDMPCR_V_SPFLSHRAT0
#define SPNDMPCR_V_SPFLSHRAT1	12
#define SPNDMPCR_M_SPFLSHRAT1	1 << SPNDMPCR_V_SPFLSHRAT1
#define SPNDMPCR_V_SPFLSHDUR	13
#define SPNDMPCR_M_SPFLSHDUR	0x3 << SPNDMPCR_V_SPFLSHDUR
#define SPNDMPCR_V_SPFLSHDUR0	13
#define SPNDMPCR_M_SPFLSHDUR0	1 << SPNDMPCR_V_SPFLSHDUR0
#define SPNDMPCR_V_SPFLSHDUR1	14
#define SPNDMPCR_M_SPFLSHDUR1	1 << SPNDMPCR_V_SPFLSHDUR1

/*
** Suspend LED Flash Rate Selects
*/
#define SPFLSHRAT_HLFHZ		0x000
#define SPFLSHRAT_1HZ		SPNDMPCR_M_SPFLSHRAT0
#define SPFLSHRAT_2HZ		SPNDMPCR_M_SPFLSHRAT1
#define SPFLSHRAT_4HZ		(SPNDMPCR_M_SPFLSHRAT0 | SPNDMPCR_M_SPFLSHRAT1)

/*
** Suspend LED Flash Rate Duration	(milliseconds)
*/
#define SPFLSHDUR_256MS		0x000
#define SPFLSHDUR_128MS		SPNDMPCR_M_SPFLSHDUR0
#define SPFLSHDUR_62PT5MS	SPNDMPCR_M_SPFLSHDUR1
#define SPFLSHDUR_31PT25MS	(SPNDMPCR_M_SPFLSHDUR0 | SPNDMPCR_M_SPFLSHDUR1)

/* 
**
**  Timer Register (TIMERR) - Index 0x00D
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     GENTMR0		Generic Timer 0
**        <1>     1     GENTMR1		Generic Timer 1
**        <2>     1     GENTMR2		Generic Timer 2
**        <3>     1     GENTMR3		Generic Timer 3
**        <4>     1     SPMDTMR0	Suspend Mode Timer 0
**        <5>     1     SPMDTMR1	Suspend Mode Timer 1
**        <6>     1     SPMDTMR2	Suspend Mode Timer 2
**        <7>     1     SLPTMR0		Sleep Mode Timer 0
**        <8>     1     SLPTMR1		Sleep Mode Timer 1
**        <9>     1     SLPTMR2		Sleep Mode Timer 2
**	  <10>	  1     DZTMR0		Doze Mode Timer 0
**	  <11>	  1     DZTMR1		Doze Mode Timer 1
**	  <12>	  1     DZTMR2		Doze Mode Timer 2
**	  <15>	  1     GTRSTPADIS	Generic Timer Reset by PA Disable
**
**	  <13:14> Reserved
**	
*/
#define PMC_TIMERR_REG		0x00D
#define PMC_TIMERR_INIT		0x0000
#define TIMERR_V_GENTMR		0
#define TIMERR_M_GENTMR		0xF << TIMERR_V_GENTMR
#define TIMERR_V_GENTMR0	0
#define TIMERR_M_GENTMR0	1 << TIMERR_V_GENTMR0
#define TIMERR_V_GENTMR1	1
#define TIMERR_M_GENTMR1	1 << TIMERR_V_GENTMR1
#define TIMERR_V_GENTMR2	2
#define TIMERR_M_GENTMR2	1 << TIMERR_V_GENTMR2
#define TIMERR_V_GENTMR3	3
#define TIMERR_M_GENTMR3	1 << TIMERR_V_GENTMR3
#define TIMERR_V_SPMDTMR	4
#define TIMERR_M_SPMDTMR	0x7 << TIMERR_V_SPMDTMR
#define TIMERR_V_SPMDTMR0	4
#define TIMERR_M_SPMDTMR0	1 << TIMERR_V_SPMDTMR0
#define TIMERR_V_SPMDTMR1	5
#define TIMERR_M_SPMDTMR1	1 << TIMERR_V_SPMDTMR1
#define TIMERR_V_SPMDTMR2	6
#define TIMERR_M_SPMDTMR2	1 << TIMERR_V_SPMDTMR2
#define TIMERR_V_SLPTMR		7
#define TIMERR_M_SLPTMR		0x7 << TIMERR_V_SLPTMR
#define TIMERR_V_SLPTMR0	7
#define TIMERR_M_SLPTMR0	1 << TIMERR_V_SLPTMR0
#define TIMERR_V_SLPTMR1	8
#define TIMERR_M_SLPTMR1	1 << TIMERR_V_SLPTMR1
#define TIMERR_V_SLPTMR2	9
#define TIMERR_M_SLPTMR2	1 << TIMERR_V_SLPTMR2
#define TIMERR_V_DZTMR		10
#define TIMERR_M_DZTMR		0x7 << TIMERR_V_DZTMR
#define TIMERR_V_DZTMR0		10
#define TIMERR_M_DZTMR0		1 << TIMERR_V_DZTMR0
#define TIMERR_V_DZTMR1		11
#define TIMERR_M_DZTMR1		1 << TIMERR_V_DZTMR1
#define TIMERR_V_DZTMR2		12
#define TIMERR_M_DZTMR2		1 << TIMERR_V_DZTMR2
#define TIMERR_V_GTRSTPADIS	15
#define TIMERR_M_GTRSTPADIS	1 << TIMERR_V_GTRSTPADIS

/*
** Generic Timer (Seconds)
*/
#define GENTMR_DISABLE		0x000
#define GENTMR_2S		TIMERR_M_GENTMR0
#define GENTMR_4S		TIMERR_M_GENTMR1
#define GENTMR_6S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR1)
#define GENTMR_8S		TIMERR_M_GENTMR2
#define GENTMR_10S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR2)
#define GENTMR_15S		(TIMERR_M_GENTMR1 | TIMERR_M_GENTMR2)
#define GENTMR_20S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR1 | TIMERR_M_GENTMR2)
#define GENTMR_25S		TIMERR_M_GENTMR3
#define GENTMR_30S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR3)
#define GENTMR_40S		(TIMERR_M_GENTMR1 | TIMERR_M_GENTMR3)
#define GENTMR_50S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR1 | TIMERR_M_GENTMR3)
#define GENTMR_60S		(TIMERR_M_GENTMR2 | TIMERR_M_GENTMR3)
#define GENTMR_75S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR2 | TIMERR_M_GENTMR3)
#define GENTMR_90S		(TIMERR_M_GENTMR1 | TIMERR_M_GENTMR2 | TIMERR_M_GENTMR3)
#define GENTMR_120S		(TIMERR_M_GENTMR0 | TIMERR_M_GENTMR1 | TIMERR_M_GENTMR2 | TIMERR_M_GENTMR3)

/*
** Suspend Mode Timer (Minutes)
*/
#define SPMDTMR_DISABLE		0x000
#define SPMDTMR_5MIN		TIMERR_M_SPMDTMR0
#define SPMDTMR_10MIN		TIMERR_M_SPMDTMR1
#define SPMDTMR_15MIN		(TIMERR_M_SPMDTMR0 | TIMERR_M_SPMDTMR1)
#define SPMDTMR_20MIN		TIMERR_M_SPMDTMR2
#define SPMDTMR_30MIN		(TIMERR_M_SPMDTMR0 | TIMERR_M_SPMDTMR2)
#define SPMDTMR_40MIN		(TIMERR_M_SPMDTMR1 | TIMERR_M_SPMDTMR2)
#define SPMDTMR_60MIN		(TIMERR_M_SPMDTMR0 | TIMERR_M_SPMDTMR1 | TIMERR_M_SPMDTMR2)

/*
** Sleep Mode Timer (Minutes)
*/
#define SLPTMR_DISABLE		0x000
#define SLPTMR_1MIN		TIMERR_M_SLPTMR0
#define SLPTMR_2MIN		TIMERR_M_SLPTMR1
#define SLPTMR_4MIN		(TIMERR_M_SLPTMR0 | TIMERR_M_SLPTMR1)
#define SLPTMR_6MIN		TIMERR_M_SLPTMR2
#define SLPTMR_8MIN		(TIMERR_M_SLPTMR0 | TIMERR_M_SLPTMR2)
#define SLPTMR_12MIN		(TIMERR_M_SLPTMR1 | TIMERR_M_SLPTMR2)
#define SLPTMR_16MIN		(TIMERR_M_SLPTMR0 | TIMERR_M_SLPTMR1 | TIMERR_M_SLPTMR2)

/*
** Doze Mode Timer (Seconds)
*/
#define DZTMR_DISABLE		0x000
#define DZTMR_PT125S		TIMERR_M_DZTMR0
#define DZTMR_PT250S		TIMERR_M_DZTMR1
#define DZTMR_PT500S		(TIMERR_M_DZTMR0 | TIMERR_M_DZTMR1)
#define DZTMR_1S		TIMERR_M_DZTMR2
#define DZTMR_4S		(TIMERR_M_DZTMR0 | TIMERR_M_DZTMR2)
#define DZTMR_8S		(TIMERR_M_DZTMR1 | TIMERR_M_DZTMR2)
#define DZTMR_16S		(TIMERR_M_DZTMR0 | TIMERR_M_DZTMR1 | TIMERR_M_DZTMR2)

/* 
**
**  PMC Miscellaneous Control 1 Register (PMCMCR1) - Index 0x00E
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <2>     1     GPCTSEL0	General Purpose Counter/Timer Select 0
**        <3>     1     GPCTSEL1	General Purpose Counter/Timer Select 1
**        <4>     1     GPCTEN		Generic Purpose Counter/Timer Enable
**        <5>     1     WMSKVLBNAC	Wake Mask VLB if Not ACPWR
**        <6>     1     WAKE0MSK	WAKE0 Mask
**        <7>     1     WAKE1MSK	WAKE1 Mask
**        <8>     1     WMSKRING	Wake Mask RING
**        <9>     1     WMSKRTC		Wake Mask RTC
**        <11>    1     WMSKTMR		Wake Mask GP Timer
**	  <12>	  1     PRDCTID0	Product Identification 0
**	  <13>	  1     PRDCTID1	Product Identification 1
**	  <14>	  1     PRDCTID2	Product Identification 2
**	  <15>	  1     PRDCTID3	Product Identification 3
**
**	  <0:1><10> Reserved
**	
*/
#define PMC_PMCMCR1_REG		0x00E
#define PMC_PMCMCR1_INIT	( PMCMCR1_M_WMSKVLBNAC | PMCMCR1_M_WAKE0MSK | \
				  PMCMCR1_M_WAKE1MSK   | PMCMCR1_M_WMSKRING | \
				  PMCMCR1_M_WMSKRTC    | PMCMCR1_M_WMSKTMR  )
#define PMCMCR1_V_GPCTSEL0	2
#define PMCMCR1_M_GPCTSEL0	1 << PMCMCR1_V_GPCTSEL0
#define PMCMCR1_V_GPCTSEL1	3
#define PMCMCR1_M_GPCTSEL1	1 << PMCMCR1_V_GPCTSEL1
#define PMCMCR1_V_GPCTEN	4
#define PMCMCR1_M_GPCTEN	1 << PMCMCR1_V_GPCTEN
#define PMCMCR1_V_WMSKVLBNAC	5
#define PMCMCR1_M_WMSKVLBNAC	1 << PMCMCR1_V_WMSKVLBNAC
#define PMCMCR1_V_WAKE0MSK	6
#define PMCMCR1_M_WAKE0MSK	1 << PMCMCR1_V_WAKE0MSK
#define PMCMCR1_V_WAKE1MSK	7
#define PMCMCR1_M_WAKE1MSK	1 << PMCMCR1_V_WAKE1MSK
#define PMCMCR1_V_WMSKRING	8
#define PMCMCR1_M_WMSKRING	1 << PMCMCR1_V_WMSKRING
#define PMCMCR1_V_WMSKRTC	9
#define PMCMCR1_M_WMSKRTC	1 << PMCMCR1_V_WMSKRTC
#define PMCMCR1_V_WMSKTMR	11
#define PMCMCR1_M_WMSKTMR	1 << PMCMCR1_V_WMSKTMR
#define PMCMCR1_V_PRDCTID	12
#define PMCMCR1_M_PRDCTID	0xF << PMCMCR1_V_PRDCTID

/* 
**
**  GP Counter/Timer Register (GPCTR) - Index 0x010
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     GPCT0		General Purpose Counter/Timer 0
**        <1>     1     GPCT1		General Purpose Counter/Timer 1
**        <2>     1     GPCT2		General Purpose Counter/Timer 2
**        <3>     1     GPCT3		General Purpose Counter/Timer 3
**        <4>     1     GPCT4		General Purpose Counter/Timer 4
**        <5>     1     GPCT5		General Purpose Counter/Timer 5
**        <6>     1     GPCT6		General Purpose Counter/Timer 6
**        <7>     1     GPCT7		General Purpose Counter/Timer 7
**        <8>     1     GPCT8		General Purpose Counter/Timer 8
**        <9>     1     GPCT9		General Purpose Counter/Timer 9
**        <10>    1     GPCT10		General Purpose Counter/Timer 10
**        <11>    1     GPCT11		General Purpose Counter/Timer 11
**        <12>    1     GPCT12		General Purpose Counter/Timer 12
**        <13>    1     GPCT13		General Purpose Counter/Timer 13
**        <14>    1     GPCT14		General Purpose Counter/Timer 14
**        <15>    1     GPCT15		General Purpose Counter/Timer 15
**	
*/
#define PMC_GPCTR_REG		0x010
#define GPCTR_M_GPCTR		0xFFFFFFFF

/* 
**
**  GP Timer Compare Register (GPTMRCMPR) - Index 0x011
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     GPTMRCMP0       GP Counter/Timer Compare 0
**        <1>     1     GPTMRCMP1	GP Counter/Timer Compare 1
**        <2>     1     GPTMRCMP2	GP Counter/Timer Compare 2
**        <3>     1     GPTMRCMP3	GP Counter/Timer Compare 3
**        <4>     1     GPTMRCMP4	GP Counter/Timer Compare 4
**        <5>     1     GPTMRCMP5	GP Counter/Timer Compare 5
**        <6>     1     GPTMRCMP6	GP Counter/Timer Compare 6
**        <7>     1     GPTMRCMP7	GP Counter/Timer Compare 7
**        <8>     1     GPTMRCMP8	GP Counter/Timer Compare 8
**        <9>     1     GPTMRCMP9	GP Counter/Timer Compare 9
**        <10>    1     GPTMRCMP10	GP Counter/Timer Compare 10
**        <11>    1     GPTMRCMP11	GP Counter/Timer Compare 11
**        <12>    1     GPTMRCMP12	GP Counter/Timer Compare 12
**        <13>    1     GPTMRCMP13	GP Counter/Timer Compare 13
**        <14>    1     GPTMRCMP14	GP Counter/Timer Compare 14
**        <15>    1     GPTMRCMP15	GP Counter/Timer Compare 15
**	
*/
#define PMC_GPTMRCMPR_REG		0x010
#define GPTMRCMPR_M_GPTMRCMPR		0xFFFFFFFF

/* 
**
**  Debounce Control Register (DBCR) - Index 0x012
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     DBDIS0		Debounce Disable 0
**        <2>     1     DBDIS1		Debounce Disable 1
**        <3>     1     DBTMDR1		Debounce Time Duration 1
**        <4>     1     DBDIS2		Debounce Disable 2
**        <5>     1     DBTMDR2		Debounce Time Duration 2
**        <6>     1     DBDIS3		Debounce Disable 3
**        <7>     1     DBTMDR3		Debounce Time Duration 3
**	  <12>	  1     WAKE0DBDIS	WAKE0 Debounce Disable
**	  <13>	  1     WAKE0DBTMDR	WAKE0 Debounce Time Duration
**	  <14>	  1     WAKE1DBDIS	WAKE1 Debounce Disable
**	  <15>	  1     WAKE1DBTMDR	WAKE1 Debounce Time Duration
**
**	  <1><8:11> Reserved
**	
*/
#define PMC_DBCR_REG		0x012
#define PMC_DBCR_INIT		0x0000
#define DBCR_V_DBDIS0		0
#define DBCR_M_DBDIS0		1 << DBCR_V_DBDIS0
#define DBCR_V_DBDIS1		2
#define DBCR_M_DBDIS1		1 << DBCR_V_DBDIS1
#define DBCR_V_DBTMDR1		3
#define DBCR_M_DBTMDR1		1 << DBCR_V_DBTMDR1
#define DBCR_V_DBDIS2		4
#define DBCR_M_DBDIS2		1 << DBCR_V_DBDIS2
#define DBCR_V_DBTMDR2		5
#define DBCR_M_DBTMDR2		1 << DBCR_V_DBTMDR2
#define DBCR_V_DBDIS3		6
#define DBCR_M_DBDIS3		1 << DBCR_V_DBDIS3
#define DBCR_V_DBTMDR3		7
#define DBCR_M_DBTMDR3		1 << DBCR_V_DBTMDR3
#define DBCR_V_WAKE0DBDIS	12
#define DBCR_M_WAKE0DBDIS	1 << DBCR_V_WAKE0DBDIS
#define DBCR_V_WAKE0DBTMDR	13
#define DBCR_M_WAKE0DBTMDR	1 << DBCR_V_WAKE0DBTMDR
#define DBCR_V_WAKE1DBDIS	14
#define DBCR_M_WAKE1DBDIS	1 << DBCR_V_WAKE1DBDIS
#define DBCR_V_WAKE1DBTMDR	15
#define DBCR_M_WAKE1DBTMDR	1 << DBCR_V_WAKE1DBTMDR

/* 
**
**  PMC Miscellaneous Control 2 Register (PMCMCR2) - Index 0x013
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <1>     1     SMIRDYEN	SMIRDY# Enable
**        <2>     1     PWRGDSTBYDIS	PWRGOOD Standby Disable
**        <3>     1     GLBLRSTEN	Global Reset Enable
**        <4>     1     PMCRSTEN	PMC Reset Enable
**        <7>     1     SPSRESETDIS	Disable SRESET Upon Resume
**        <8>     1     SPRSTDRVDIS	Disable RSTDRV Upon Resume
**        <9>     1     WAKE0TOGL	WAKE0 Toggle Select
**        <10>    1     WAKE1TOGL	WAKE1 Toggle Select
**	  <14>	  1     SWTCHEN		SWTCH Enable
**	  <15>	  1     RSTCPUFLG	RSTCPU Flag
**
**	  <0><5:6><11:13> Reserved
**	
*/
#define PMC_PMCMCR2_REG		0x013
#define PMC_PMCMCR2_INIT	( PMCMCR2_M_PWRGDSTBYDIS | PMCMCR2_M_GLBLRSTEN | \
				  PMCMCR2_M_PMCRSTEN     | \
			          PMCMCR2_M_SPSRESETDIS  )
#define PMCMCR2_V_GPCTSEL0	1
#define PMCMCR2_M_GPCTSEL0	1 << PMCMCR2_V_SMIRDYEN
#define PMCMCR2_V_PWRGDSTBYDIS	2
#define PMCMCR2_M_PWRGDSTBYDIS	1 << PMCMCR2_V_PWRGDSTBYDIS
#define PMCMCR2_V_GLBLRSTEN	3
#define PMCMCR2_M_GLBLRSTEN	1 << PMCMCR2_V_GLBLRSTEN
#define PMCMCR2_V_PMCRSTEN	4
#define PMCMCR2_M_PMCRSTEN	1 << PMCMCR2_V_PMCRSTEN
#define PMCMCR2_V_SPSRESETDIS	7
#define PMCMCR2_M_SPSRESETDIS	1 << PMCMCR2_V_SPSRESETDIS
#define PMCMCR2_V_SPRSTDRVDIS	8
#define PMCMCR2_M_SPRSTDRVDIS	1 << PMCMCR2_V_SPRSTDRVDIS
#define PMCMCR2_V_WAKE0TOGL	9
#define PMCMCR2_M_WAKE0TOGL	1 << PMCMCR2_V_WAKE0TOGL
#define PMCMCR2_V_WAKE1TOGL	10
#define PMCMCR2_M_WAKE1TOGL	1 << PMCMCR2_V_WAKE1TOGL
#define PMCMCR2_V_SWTCHEN	14
#define PMCMCR2_M_SWTCHEN	1 << PMCMCR2_V_SWTCHEN
#define PMCMCR2_V_RSTCPUFLG	15
#define PMCMCR2_M_RSTCPUFLG	1 << PMCMCR2_V_RSTCPUFLG

/* 
**
**  Optional GPIO Control Register 2 (GPIOCR2) - Index 0x014
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     GPIOADATA0	General Purpose I/O A Data 0
**        <1>     1     GPIOADATA1	General Purpose I/O A Data 1
**        <2>     1     GPIOADIR0	General Purpose I/O A Direction 0
**        <3>     1     GPIOADIR1	General Purpose I/O A Direction 1
**        <4>     1     GPIOBDATA0	General Purpose I/O B Data 0
**        <5>     1     GPIOBDATA1	General Purpose I/O B Data 1
**        <6>     1     GPIOBDIR0	General Purpose I/O B Direction 0
**        <7>     1     GPIOBDIR1	General Purpose I/O B Direction 1
**        <8>     1     GPIOCDATA0	General Purpose I/O C Data 0
**        <9>     1     GPIOCDATA1	General Purpose I/O C Data 1
**        <10>    1     GPIOCDIR0	General Purpose I/O C Direction 0
**        <11>    1     GPIOCDIR1	General Purpose I/O C Direction 1
**
**	  <12:15> Reserved
**	
*/
#define PMC_GPIOCR2_REG		0x014
#define PMC_GPIOCR2_INIT	( GPIOCR2_M_GPIOADIR0 | GPIOCR2_M_GPIOADIR1  | \
				  GPIOCR2_M_GPIOBDIR1 | GPIOCR2_M_GPIOCDATA0 | \
				  GPIOCR2_M_GPIOCDIR0 | GPIOCR2_M_GPIOCDIR1  )
#define GPIOCR2_V_GPIOADATA	0
#define GPIOCR2_M_GPIOADATA	0x3 << GPIOCR2_V_GPIOADATA
#define GPIOCR2_V_GPIOADATA0	0
#define GPIOCR2_M_GPIOADATA0	1 << GPIOCR2_V_GPIOADATA0
#define GPIOCR2_V_GPIOADATA1	1
#define GPIOCR2_M_GPIOADATA1	1 << GPIOCR2_V_GPIOADATA1
#define GPIOCR2_V_GPIOADIR	2
#define GPIOCR2_M_GPIOADIR	0x3 << GPIOCR2_V_GPIOADIR
#define GPIOCR2_V_GPIOADIR0	2
#define GPIOCR2_M_GPIOADIR0	1 << GPIOCR2_V_GPIOADIR0
#define GPIOCR2_V_GPIOADIR1	3
#define GPIOCR2_M_GPIOADIR1	1 << GPIOCR2_V_GPIOADIR1
#define GPIOCR2_V_GPIOBDATA	4
#define GPIOCR2_M_GPIOBDATA	0x3 << GPIOCR2_V_GPIOBDATA
#define GPIOCR2_V_GPIOBDATA0	4
#define GPIOCR2_M_GPIOBDATA0	1 << GPIOCR2_V_GPIOBDATA0
#define GPIOCR2_V_GPIOBDATA1	5
#define GPIOCR2_M_GPIOBDATA1	1 << GPIOCR2_V_GPIOBDATA1
#define GPIOCR2_V_GPIOBDIR	6
#define GPIOCR2_M_GPIOBDIR	0x3 << GPIOCR2_V_GPIOBDIR
#define GPIOCR2_V_GPIOBDIR0	6
#define GPIOCR2_M_GPIOBDIR0	1 << GPIOCR2_V_GPIOBDIR0
#define GPIOCR2_V_GPIOBDIR1	7
#define GPIOCR2_M_GPIOBDIR1	1 << GPIOCR2_V_GPIOBDIR1
#define GPIOCR2_V_GPIOCDATA	8
#define GPIOCR2_M_GPIOCDATA	0x3 << GPIOCR2_V_GPIOCDATA
#define GPIOCR2_V_GPIOCDATA0	8
#define GPIOCR2_M_GPIOCDATA0	1 << GPIOCR2_V_GPIOCDATA0
#define GPIOCR2_V_GPIOCDATA1	9
#define GPIOCR2_M_GPIOCDATA1	1 << GPIOCR2_V_GPIOCDATA1
#define GPIOCR2_V_GPIOCDIR	10
#define GPIOCR2_M_GPIOCDIR	0x3 << GPIOCR2_V_GPIOCDIR
#define GPIOCR2_V_GPIOCDIR0	10
#define GPIOCR2_M_GPIOCDIR0	1 << GPIOCR2_V_GPIOCDIR0
#define GPIOCR2_V_GPIOCDIR1	11
#define GPIOCR2_M_GPIOCDIR1	1 << GPIOCR2_V_GPIOCDIR1

/* 
**
**  Leakage Control Register (LKCR) - Index 0x015
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     CPULCDIS	CPU Interface Leakage Control Disable
**        <1>     1     DRAMLCDIS	DRAM Interface Leakage Control Disable
**        <2>     1     L2CACHELCDIS	L2 Cache Leakage Control Disable
**        <3>     1     ATLCDIS		AT Bus Interface Leakage Control Disable
**        <4>     1     BBUSLCDIS	Burst Bus Interface Leakage Control Disable
**        <5>     1     GPIOLCDIS	GPIO Interface Leakage Control Disable
**        <6>     1     PC49LCDIS	PC4 to PC9 Leakage Control Disable
**        <7>     1     LEDLCDIS	LED Interface Leakage Control Disable
**        <8>     1     BMLCDIS		Battery Management Leakage Control Disable
**        <9>     1     PC03LCDIS	PC0 to PC3 Leakage Control Disable
**        <14>    1     PWRGDLCDIS	Enable Leakage Control
**        <15>    1     LCDIS		Global Leakage Control
**
**	  <10:13> Reserved
**	
*/
#define PMC_LKCR_REG		0x015
#define PMC_LKCR_INIT		( LKCR_M_CPULCDIS     | LKCR_M_DRAMLCDIS | \
				  LKCR_M_L2CACHELCDIS | LKCR_M_ATLCDIS   | \
				  LKCR_M_BBUSLCDIS    | LKCR_M_GPIOLCDIS | \
				  LKCR_M_PC49LCDIS    | LKCR_M_LEDLCDIS  | \
				  LKCR_M_BMLCDIS      | LKCR_M_PC03LCDIS | \
				  LKCR_M_PWRGDLCDIS   | LKCR_M_LCDIS     )
#define LKCR_V_CPULCDIS		0
#define LKCR_M_CPULCDIS		1 << LKCR_V_CPULCDIS
#define LKCR_V_DRAMLCDIS	1
#define LKCR_M_DRAMLCDIS	1 << LKCR_V_DRAMLCDIS
#define LKCR_V_L2CACHELCDIS	2
#define LKCR_M_L2CACHELCDIS	1 << LKCR_V_L2CACHELCDIS
#define LKCR_V_ATLCDIS		3
#define LKCR_M_ATLCDIS		1 << LKCR_V_ATLCDIS
#define LKCR_V_BBUSLCDIS	4
#define LKCR_M_BBUSLCDIS	1 << LKCR_V_BBUSLCDIS
#define LKCR_V_GPIOLCDIS	5
#define LKCR_M_GPIOLCDIS	1 << LKCR_V_GPIOLCDIS
#define LKCR_V_PC49LCDIS	6
#define LKCR_M_PC49LCDIS	1 << LKCR_V_PC49LCDIS
#define LKCR_V_LEDLCDIS		7
#define LKCR_M_LEDLCDIS		1 << LKCR_V_LEDLCDIS
#define LKCR_V_BMLCDIS		8
#define LKCR_M_BMLCDIS		1 << LKCR_V_BMLCDIS
#define LKCR_V_PC03LCDIS	9
#define LKCR_M_PC03LCDIS	1 << LKCR_V_PC03LCDIS
#define LKCR_V_PWRGDLCDIS	14
#define LKCR_M_PWRGDLCDIS	1 << LKCR_V_PWRGDLCDIS
#define LKCR_V_LCDIS		15
#define LKCR_M_LCDIS		1 << LKCR_V_LCDIS

/* 
**
**  SEQUOIA-1 Miscellaneous Register 1 (SEQMR1) - Index 0x016
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <11>    1     DISCREADS	Driving EADS# on Non-Cacheable Region
**        <12>    1     FRCBLASTDIS	Forcing BLAST low During Reset
**        <14>    1     P92B3EN		Enable Control Bit for Port 92 Bit 3 Function
**        <15>    1     SWTSTBYDIS	Disable Standby on SWTCH
**
**	  <0:10><13> Reserved
**	
*/
#define PMC_SEQMR1_REG		0x016
#define PMC_SEQMR1_INIT		0x0000
#define SEQMR1_V_DISCREADS	11
#define SEQMR1_M_DISCREADS	1 << SEQMR1_V_DISCREADS
#define SEQMR1_V_FRCBLASTDIS	12
#define SEQMR1_M_FRCBLASTDIS	1 << SEQMR1_V_FRCBLASTDIS
#define SEQMR1_V_P92B3EN	14
#define SEQMR1_M_P92B3EN	1 << SEQMR1_V_P92B3EN
#define SEQMR1_V_SWTSTBYDIS	15
#define SEQMR1_M_SWTSTBYDIS	1 << SEQMR1_V_SWTSTBYDIS

/* 
**
**  SEQUOIA-1 Miscellaneous Register 2 (SEQMR2) - Index 0x017
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     DLYWCASEN	Delay Write CAS Enable
**        <1>     1     SMMWTEN		SMM Write-Through Enable 
**        <2>     1     STATICSPNDEN	Static Suspend Enable
**
**	  <3:15> Reserved
**	
*/
#define PMC_SEQMR2_REG		0x017
#define PMC_SEQMR2_INIT		0x0000
#define SEQMR2_V_DLYWCASEN	0
#define SEQMR2_M_DLYWCASEN	1 << SEQMR2_V_DLYWCASEN
#define SEQMR2_V_SMMWTEN	1
#define SEQMR2_M_SMMWTEN	1 << SEQMR2_V_SMMWTEN
#define SEQMR2_V_STATICSPNDEN	2
#define SEQMR2_M_STATICSPNDEN	1 << SEQMR2_V_STATICSPNDEN

/* 
**
**  Secondary Activity Mask Register (SAMR) - Index 0x019
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SAMSKVID	Secondary Activity Mask Video Accesses
**        <3>     1     SAMSKKBD	Secondary Activity Mask Keyboard Accesses
**        <6>     1     SAMSKPROG0	Secondary Activity Mask PR 0
**        <7>     1     SAMSKPROG1	Secondary Activity Mask PR 1
**        <8>     1     SAMSKPROG2	Secondary Activity Mask PR 2
**        <9>     1     SAMSKPROG3	Secondary Activity Mask PR 3
**        <12>    1     SAMSKEACT0	Secondary Activity Mask EXTACT0
**        <13>    1     SAMSKEACT1	Secondary Activity Mask EXTACT1
**        <14>    1     SAMSKEACT2	Secondary Activity Mask EXTACT2
**        <15>    1     SAMSKEACT3	Secondary Activity Mask EXTACT3
**
**	  <1:2><4:5><10:11> Reserved
**	
*/
#define PMC_SAMR_REG		0x019
#define PMC_SAMR_INIT		( SAMR_M_SAMSKVID   | SAMR_M_SAMSKKBD   | \
				  SAMR_M_SAMSKPROG0 | SAMR_M_SAMSKPROG1 | \
				  SAMR_M_SAMSKPROG2 | SAMR_M_SAMSKPROG3 | \
				  SAMR_M_SAMSKEACT0 | SAMR_M_SAMSKEACT1 | \
				  SAMR_M_SAMSKEACT2 | SAMR_M_SAMSKEACT3 )
#define SAMR_V_SAMSKVID		0
#define SAMR_M_SAMSKVID		1 << SAMR_V_SAMSKVID
#define SAMR_V_SAMSKKBD		3
#define SAMR_M_SAMSKKBD		1 << SAMR_V_SAMSKKBD
#define SAMR_V_SAMSKPROG0	6
#define SAMR_M_SAMSKPROG0	1 << SAMR_V_SAMSKPROG0
#define SAMR_V_SAMSKPROG1	7
#define SAMR_M_SAMSKPROG1	1 << SAMR_V_SAMSKPROG1
#define SAMR_V_SAMSKPROG2	8
#define SAMR_M_SAMSKPROG2	1 << SAMR_V_SAMSKPROG2
#define SAMR_V_SAMSKPROG3	9
#define SAMR_M_SAMSKPROG3	1 << SAMR_V_SAMSKPROG3
#define SAMR_V_SAMSKEACT0	12
#define SAMR_M_SAMSKEACT0	1 << SAMR_V_SAMSKEACT0
#define SAMR_V_SAMSKEACT1	13
#define SAMR_M_SAMSKEACT1	1 << SAMR_V_SAMSKEACT1
#define SAMR_V_SAMSKEACT2	14
#define SAMR_M_SAMSKEACT2	1 << SAMR_V_SAMSKEACT2
#define SAMR_V_SAMSKEACT3	15
#define SAMR_M_SAMSKEACT3	1 << SAMR_V_SAMSKEACT3

/* 
**
**  Additional Activity Source Register (AASR) - Index 0x01A
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     WAKE0ACTV	WAKE0 Active
**        <1>     1     WAKE1ACTV	WAKE1 Active
**        <2>     1     INTRACTV	Interrupt Active
**        <4>     1     DZTO		Doze Time-out Status
**        <5>     1     SLPTO		Sleep Time-out Status
**        <6>     1     SPNDTO		Suspend Time-out Status
**        <7>     1     GENTO		Generic Timer Time-out Status
**        <8>     1     PRTMR0TO	Programmable Timer 0 Time-out Status
**        <9>     1     PRTMR1TO	Programmable Timer 1 Time-out Status
**        <10>    1     PRTMR2TO	Programmable Timer 2 Time-out Status
**        <11>    1     PRTMR3TO	Programmable Timer 3 Time-out Status
**        <12>    1     PRTOPMI0	Programmable Timer 0 PMI Flag
**        <13>    1     PRTOPMI1	Programmable Timer 1 PMI Flag
**        <14>    1     PRTOPMI2	Programmable Timer 2 PMI Flag
**        <15>    1     PRTOPMI3	Programmable Timer 3 PMI Flag
**
**	  <3> Reserved
**	
*/
#define PMC_AASR_REG		0x01A
#define PMC_AASR_INIT		0x0000
#define AASR_V_WAKE0ACTV	0
#define AASR_M_WAKE0ACTV	1 << AASR_V_WAKE0ACTV
#define AASR_V_WAKE1ACTV	1
#define AASR_M_WAKE1ACTV	1 << AASR_V_WAKE1ACTV
#define AASR_V_INTRACTV		2
#define AASR_M_INTRACTV		1 << AASR_V_INTRACTV
#define AASR_V_DZTO		4
#define AASR_M_DZTO		1 << AASR_V_DZTO
#define AASR_V_SLPTO		5
#define AASR_M_SLPTO		1 << AASR_V_SLPTO
#define AASR_V_SPNDTO		6
#define AASR_M_SPNDTO		1 << AASR_V_SPNDTO
#define AASR_V_GENTO		7
#define AASR_M_GENTO		1 << AASR_V_GENTO
#define AASR_V_PRTMR0TO		8
#define AASR_M_PRTMR0TO		1 << AASR_V_PRTMR0TO
#define AASR_V_PRTMR1TO		9
#define AASR_M_PRTMR1TO		1 << AASR_V_PRTMR0T1
#define AASR_V_PRTMR2TO		10
#define AASR_M_PRTMR2TO		1 << AASR_V_PRTMR0T2
#define AASR_V_PRTMR3TO		11
#define AASR_M_PRTMR3TO		1 << AASR_V_PRTMR0T3
#define AASR_V_PRTOPMI0		12
#define AASR_M_PRTOPMI0		1 << AASR_V_PRTOPMI0
#define AASR_V_PRTOPMI1		13
#define AASR_M_PRTOPMI1		1 << AASR_V_PRTOPMI1
#define AASR_V_PRTOPMI2		14
#define AASR_M_PRTOPMI2		1 << AASR_V_PRTOPMI2
#define AASR_V_PRTOPMI3		15
#define AASR_M_PRTOPMI3		1 << AASR_V_PRTOPMI3

/* 
**
**  Additional Primary Activity Mask Register (APAMR) - Index 0x01B
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PAMSKWAKE0	Primary Activity Mask WAKE0 
**        <1>     1     PAMSKWAKE1	Primary Activity Mask WAKE1
**        <2>     1     PAMSKINTR	Primary Activity Mask INTR
**        <3>     1     PAMSKPMI	Primary Activity Mask PMI
**        <4>     1     PAMSKHOLD	Primary Activity Mask HOLD Request
**        <5>     1     PAMSKNMI	Primary Activity Mask NMI
**        <6>     1     PAMSKDMA	Primary Activity Mask DMA
**        <7>     1     PAMSKMSTR	Primary Activity Mask MASTER
**        <11>    1     DISPACTVON	Disable Primary Activity On
**        <14>    1     HD2EN		Hard Disk Secondar Enable
**        <15>    1     LMTPACTV	Limited Primary Activity PMI
**
**	  <8:10><12:13> Reserved
**	
*/
#define PMC_APAMR_REG		0x01B
#define PMC_APAMR_INIT		( APAMR_M_PAMSKWAKE0 | APAMR_M_PAMSKWAKE1 | \
				  APAMR_M_PAMSKINTR  | APAMR_M_PAMSKPMI   | \
				  APAMR_M_PAMSKHOLD  | APAMR_M_PAMSKNMI   | \
				  APAMR_M_PAMSKDMA   | APAMR_M_PAMSKMSTR  | \
				  APAMR_M_LMTPACTV   )
#define APAMR_V_PAMSKWAKE0	0
#define APAMR_M_PAMSKWAKE0	1 << APAMR_V_PAMSKWAKE0
#define APAMR_V_PAMSKWAKE1	1
#define APAMR_M_PAMSKWAKE1	1 << APAMR_V_PAMSKWAKE1
#define APAMR_V_PAMSKINTR	2
#define APAMR_M_PAMSKINTR	1 << APAMR_V_PAMSKINTR
#define APAMR_V_PAMSKPMI	3
#define APAMR_M_PAMSKPMI	1 << APAMR_V_PAMSKPMI
#define APAMR_V_PAMSKHOLD	4
#define APAMR_M_PAMSKHOLD	1 << APAMR_V_PAMSKHOLD
#define APAMR_V_PAMSKNMI	5
#define APAMR_M_PAMSKNMI	1 << APAMR_V_PAMSKNMI
#define APAMR_V_PAMSKDMA	6
#define APAMR_M_PAMSKDMA	1 << APAMR_V_PAMSKDMA
#define APAMR_V_PAMSKMSTR	7
#define APAMR_M_PAMSKMSTR	1 << APAMR_V_PAMSKMSTR
#define APAMR_V_DISPACTVON	11
#define APAMR_M_DISPACTVON	1 << APAMR_V_DISPACTVON
#define APAMR_V_HD2EN		14
#define APAMR_M_HD2EN		1 << APAMR_V_HD2EN
#define APAMR_V_LMTPACTV	15
#define APAMR_M_LMTPACTV	1 << APAMR_V_LMTPACTV

/* 
**
**  Additional Secondary Control Register (ASCR) - Index 0x01C
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SAMSKWAKE0	Secondary Activity Mask WAKE0 
**        <1>     1     SAMSKWAKE1	Secondary Activity Mask WAKE1
**        <2>     1     SAMSKINTR	Secondary Activity Mask INTR
**        <3>     1     SAMSKPMI	Secondary Activity Mask PMI
**        <4>     1     SAMSKIRQ0	Secondary Activity Mask IRQ0
**        <6>     1     SAMSKDMA	Secondary Activity Mask DMA
**        <7>     1     SAMSKMSTR	Secondary Activity Mask MASTER
**        <9>     1     SACTVTMR0	Secondary Activity Timer 0
**        <10>    1     SACTVTMR1	Secondary Activity Timer 1
**        <11>    1     SACTVTMR2	Secondary Activity Timer 2
**        <15>    1     LMTSACTV	Limited Secondary Activity PMI
**
**	  <5><8><12:14> Reserved
**	
*/
#define PMC_ASCR_REG		0x01C
#define PMC_ASCR_INIT		( ASCR_M_SAMSKWAKE0 | ASCR_M_SAMSKWAKE1 | \
				  ASCR_M_SAMSKINTR  | ASCR_M_SAMSKPMI   | \
				  ASCR_M_SAMSKIRQ0  | ASCR_M_SAMSKDMA   | \
				  ASCR_M_SAMSKMSTR  | ASCR_M_SACTVTMR2  | \
				  ASCR_M_LMTSACTV   )
#define ASCR_V_SAMSKWAKE0	0
#define ASCR_M_SAMSKWAKE0	1 << ASCR_V_SAMSKWAKE0
#define ASCR_V_SAMSKWAKE1	1
#define ASCR_M_SAMSKWAKE1	1 << ASCR_V_SAMSKWAKE1
#define ASCR_V_SAMSKINTR	2
#define ASCR_M_SAMSKINTR	1 << ASCR_V_SAMSKINTR
#define ASCR_V_SAMSKPMI		3
#define ASCR_M_SAMSKPMI		1 << ASCR_V_SAMSKPMI
#define ASCR_V_SAMSKIRQ0	4
#define ASCR_M_SAMSKIRQ0	1 << ASCR_V_SAMSKIRQ0
#define ASCR_V_SAMSKDMA		6
#define ASCR_M_SAMSKDMA		1 << ASCR_V_SAMSKDMA
#define ASCR_V_SAMSKMSTR	7
#define ASCR_M_SAMSKMSTR	1 << ASCR_V_SAMSKMSTR
#define ASCR_V_SACTVTMR		9
#define ASCR_M_SACTVTMR		0x7 << ASCR_V_SACTVTMR
#define ASCR_V_SACTVTMR0	9
#define ASCR_M_SACTVTMR0	1 << ASCR_V_SACTVTMR0
#define ASCR_V_SACTVTMR1	10
#define ASCR_M_SACTVTMR1	1 << ASCR_V_SACTVTMR1
#define ASCR_V_SACTVTMR2	11
#define ASCR_M_SACTVTMR2	1 << ASCR_V_SACTVTMR2
#define ASCR_V_LMTSACTV		15
#define ASCR_M_LMTSACTV		1 << ASCR_V_LMTSACTV

/*
** Secondary Activity Timer 
*/
#define SACTVTMR_125US		0x000
#define SACTVTMR_1MS		ASCR_M_SACTVTMR0
#define SACTVTMR_2MS		ASCR_M_SACTVTMR1
#define SACTVTMR_4MS		(ASCR_M_SACTVTMR0 | ASCR_M_SACTVTMR1)
#define SACTVTMR_8MS		ASCR_M_SACTVTMR2
#define SACTVTMR_16MS		(ASCR_M_SACTVTMR0 | ASCR_M_SACTVTMR2)
#define SACTVTMR_32MS		(ASCR_M_SACTVTMR1 | ASCR_M_SACTVTMR2)
#define SACTVTMR_64MS		(ASCR_M_SACTVTMR0 | ASCR_M_SACTVTMR1 | ASCR_M_SACTVTMR2)

/* 
**
**  Additional PMI Mask Register (APMIMR) - Index 0x01D
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     IMSKWAKE0	Mask WAKE0 from PMI
**        <1>     1     IMSKWAKE1	Mask WAKE1 from PMI
**        <3>     1     IMSKRTC		Mask RTC from PMI
**        <4>     1     RESCHED0	Reschedule PMI 0
**        <5>     1     RESCHED1	Reschedule PMI 1
**        <6>     1     SFTSMI		Soft SMI
**        <8>     1     IMSKPRTMR0TO	Mask Programmable Time 0 Time-out from PMI
**        <9>     1     IMSKPRTMR1TO	Mask Programmable Time 1 Time-out from PMI
**        <10>    1     IMSKPRTMR2TO	Mask Programmable Time 2 Time-out from PMI
**        <11>    1     IMSKPRTMR3TO	Mask Programmable Time 3 Time-out from PMI
**
**	  <2><7><12:15> Reserved
**	
*/
#define PMC_APMIMR_REG		0x01D
#define PMC_APMIMR_INIT		( APMIMR_M_IMSKWAKE0   | APMIMR_M_IMSKWAKE1   | \
				  APMIMR_M_IMSKRTC     | APMIMR_M_IMSKPRTMR0TO| \
				  APMIMR_M_IMSKPRTMR1TO | \
				  APMIMR_M_IMSKPRTMR2TO | \
				  APMIMR_M_IMSKPRTMR3TO )
#define APMIMR_V_IMSKWAKE0	0
#define APMIMR_M_IMSKWAKE0	1 << APMIMR_V_IMSKWAKE0
#define APMIMR_V_IMSKWAKE1	1
#define APMIMR_M_IMSKWAKE1	1 << APMIMR_V_IMSKWAKE1
#define APMIMR_V_IMSKRTC	3
#define APMIMR_M_IMSKRTC	1 << APMIMR_V_IMSKRTC
#define APMIMR_V_RESCHED	4
#define APMIMR_M_RESCHED	0x3 << APMIMR_V_RESCHED
#define APMIMR_V_RESCHED0	4
#define APMIMR_M_RESCHED0	1 << APMIMR_V_RESCHED0
#define APMIMR_V_RESCHED1	5
#define APMIMR_M_RESCHED1	1 << APMIMR_V_RESCHED1
#define APMIMR_V_SFTSMI		6
#define APMIMR_M_SFTSMI		1 << APMIMR_V_SFTSMI
#define APMIMR_V_IMSKPRTMR0TO	8
#define APMIMR_M_IMSKPRTMR0TO	1 << APMIMR_V_IMSKPRTMR0TO
#define APMIMR_V_IMSKPRTMR1TO	9
#define APMIMR_M_IMSKPRTMR1TO	1 << APMIMR_V_IMSKPRTMR1TO
#define APMIMR_V_IMSKPRTMR2TO	10
#define APMIMR_M_IMSKPRTMR2TO	1 << APMIMR_V_IMSKPRTMR2TO
#define APMIMR_V_IMSKPRTMR3TO	11
#define APMIMR_M_IMSKPRTMR3TO	1 << APMIMR_V_IMSKPRTMR3TO

/*
** Reschedule PMI
*/
#define RESCHED_DISABLE		0x000
#define RESCHED_64MS		APMIMR_M_RESCHED0
#define RESCHED_1MS		APMIMR_M_RESCHED1
#define RESCHED_1S		(APMIMR_M_RESCHED0 | APMIMR_M_RESCHED1)

/* 
**
**  Miscellaneous Control Register (MCR) - Index 0x01E
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PMIFLGEN	PMI Flag Enable
**        <1>     1     PAFLGEN		Primary Activity Flag Enable
**        <2>     1     SAFLGEN		Secondary Activity Flag Enable
**
**	  <3:15> Reserved
**	
*/
#define PMC_MCR_REG		0x01E
#define PMC_MCR_INIT		0x0000
#define MCR_V_PMIFLGEN		0
#define MCR_M_PMIFLGEN		1 << MCR_V_PMIFLGEN
#define MCR_V_PAFLGEN		1
#define MCR_M_PAFLGEN		1 << MCR_V_PAFLGEN
#define MCR_V_SAFLGEN		2
#define MCR_M_SAFLGEN		1 << MCR_V_SAFLGEN

/* 
**
**  SEQUOIA-1 Identification Register (SEQIDR) - Index 0x01F
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SQ1ID0		SEQUOIA-1 Revision Number ID 0
**        <1>     1     SQ1ID1		SEQUOIA-1 Revision Number ID 1
**        <2>     1     SQ1ID2		SEQUOIA-1 Revision Number ID 2
**        <3>     1     SQ1ID3		SEQUOIA-1 Revision Number ID 3
**
**	  <4:15> Reserved
**	
*/
#define PMC_SEQIDR_REG		0x01F
#define SEQIDR_V_SQ1ID		0
#define SEQIDR_M_SQ1ID		0xF << SEQIDR_V_SQ1ID

/* 
**
**  Programmable Range Address Register 0 (PRAR0) - Index 0x020
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     9     PRMA		PRM 0 Address
**        <14>    1     PROSHDW0EN	Programmable Shadow Register 0 Enable
**        <15>    1     PROCS0EN	Programmable Chip Select 0 Enable
**
**	  <10:13> Reserved
**	
*/
#define PMC_PRAR0_REG		0x020
#define PMC_PRAR0_INIT		0x0000
#define PRAR0_V_PRMA		0
#define PRAR0_M_PRMA		0x3FF << PRAR0_V_PRMA
#define PRAR0_V_PROSHDW0EN	14
#define PRAR0_M_PROSHDW0EN	1 << PRAR0_V_PROSHDW0EN
#define PRAR0_V_PROCS0EN	15
#define PRAR0_M_PROCS0EN	1 << PRAR0_V_PROCS0EN

/* 
**
**  Programmable Range Compare Register 0 (PRCR0) - Index 0x021
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PRMCMPEN00	PRM 0 Compare Enable 0
**        <1>     1     PRMCMPEN01	PRM 0 Compare Enable 1
**        <2>     1     PRMCMPEN02	PRM 0 Compare Enable 2
**        <3>     1     PRMCMPEN03	PRM 0 Compare Enable 3
**        <4>     1     PRMCMPEN04	PRM 0 Compare Enable 4
**        <5>     1     PRMCMPEN05	PRM 0 Compare Enable 5
**        <6>     1     PRMCMPEN06	PRM 0 Compare Enable 6
**        <7>     1     PRMCMPEN07	PRM 0 Compare Enable 7
**        <8>     1     PRMCMPEN08	PRM 0 Compare Enable 8
**        <9>     1     PRMCMPEN09	PRM 0 Compare Enable 9
**        <10>    1     PRMWR0EN	PRM 0 Compare Write Enable 
**        <11>    1     PRMRD0EN	PRM 0 Compare Read Enable 
**        <12>    1     PRM0MIO		PRM 0 Memory or I/O Compare Enable 
**        <13>    1     PRM0EN		PRM 0 Enable 
**
**	  <14:15> Reserved
**	
*/
#define PMC_PRCR0_REG		0x021
#define PMC_PRCR0_INIT		0x0000
#define PRCR0_V_PRMCMPEN	0
#define PRCR0_M_PRMCMPEN	0x3FF << PRCR0_V_PRMCMPEN
#define PRCR0_V_PRMCMPEN00	0
#define PRCR0_M_PRMCMPEN00	1 << PRCR0_V_PRMCMPEN00
#define PRCR0_V_PRMCMPEN01	1
#define PRCR0_M_PRMCMPEN01	1 << PRCR0_V_PRMCMPEN01
#define PRCR0_V_PRMCMPEN02	2
#define PRCR0_M_PRMCMPEN02	1 << PRCR0_V_PRMCMPEN02
#define PRCR0_V_PRMCMPEN03	3
#define PRCR0_M_PRMCMPEN03	1 << PRCR0_V_PRMCMPEN03
#define PRCR0_V_PRMCMPEN04	4
#define PRCR0_M_PRMCMPEN04	1 << PRCR0_V_PRMCMPEN04
#define PRCR0_V_PRMCMPEN05	5
#define PRCR0_M_PRMCMPEN05	1 << PRCR0_V_PRMCMPEN05
#define PRCR0_V_PRMCMPEN06	6
#define PRCR0_M_PRMCMPEN06	1 << PRCR0_V_PRMCMPEN06
#define PRCR0_V_PRMCMPEN07	7
#define PRCR0_M_PRMCMPEN07	1 << PRCR0_V_PRMCMPEN07
#define PRCR0_V_PRMCMPEN08	8
#define PRCR0_M_PRMCMPEN08	1 << PRCR0_V_PRMCMPEN08
#define PRCR0_V_PRMCMPEN09	9
#define PRCR0_M_PRMCMPEN09	1 << PRCR0_V_PRMCMPEN09
#define PRCR0_V_PRMWR0EN	10
#define PRCR0_M_PRMWR0EN	1 << PRCR0_V_PRMWR0EN
#define PRCR0_V_PRMRD0EN	11
#define PRCR0_M_PRMRD0EN	1 << PRCR0_V_PRMRD0EN
#define PRCR0_V_PRM0MIO		12
#define PRCR0_M_PRM0MIO		1 << PRCR0_V_PRM0MIO
#define PRCR0_V_PRM0EN		13
#define PRCR0_M_PRM0EN		1 << PRCR0_V_PRM0EN

/* 
**
**  Programmable Range Address Register 1 (PRAR1) - Index 0x022
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     9     PRMA		PRM 1 Address
**        <14>    1     PROSHDW1EN	Programmable Shadow Register 1 Enable
**        <15>    1     PROCS1EN	Programmable Chip Select 1 Enable
**
**	  <10:13> Reserved
**	
*/
#define PMC_PRAR1_REG		0x022
#define PRAR1_V_PRMA		0
#define PRAR1_M_PRMA		0x3FF << PRAR1_V_PRMA
#define PRAR1_V_PROSHDW1EN	14
#define PRAR1_M_PROSHDW1EN	1 << PRAR1_V_PROSHDW1EN
#define PRAR1_V_PROCS1EN	15
#define PRAR1_M_PROCS1EN	1 << PRAR1_V_PROCS1EN

/* 
**
**  Programmable Range Compare Register 1 (PRCR1) - Index 0x023
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PRMCMPEN10	PRM 1 Compare Enable 0
**        <1>     1     PRMCMPEN11	PRM 1 Compare Enable 1
**        <2>     1     PRMCMPEN12	PRM 1 Compare Enable 2
**        <3>     1     PRMCMPEN13	PRM 1 Compare Enable 3
**        <4>     1     PRMCMPEN14	PRM 1 Compare Enable 4
**        <5>     1     PRMCMPEN15	PRM 1 Compare Enable 5
**        <6>     1     PRMCMPEN16	PRM 1 Compare Enable 6
**        <7>     1     PRMCMPEN17	PRM 1 Compare Enable 7
**        <8>     1     PRMCMPEN18	PRM 1 Compare Enable 8
**        <9>     1     PRMCMPEN19	PRM 1 Compare Enable 9
**        <10>    1     PRMWR1EN	PRM 1 Compare Write Enable 
**        <11>    1     PRMRD1EN	PRM 1 Compare Read Enable 
**        <12>    1     PRM1MIO		PRM 1 Memory or I/O Compare Enable 
**        <13>    1     PRM1EN		PRM 1 Enable 
**
**	  <14:15> Reserved
**	
*/
#define PMC_PRCR1_REG		0x023
#define PMC_PRCR1_INIT		0x0000
#define PRCR1_V_PRMCMPEN	0
#define PRCR1_M_PRMCMPEN	0x3FF << PRCR1_V_PRMCMPEN
#define PRCR1_V_PRMCMPEN10	0
#define PRCR1_M_PRMCMPEN10	1 << PRCR1_V_PRMCMPEN10
#define PRCR1_V_PRMCMPEN11	1
#define PRCR1_M_PRMCMPEN11	1 << PRCR1_V_PRMCMPEN11
#define PRCR1_V_PRMCMPEN12	2
#define PRCR1_M_PRMCMPEN12	1 << PRCR1_V_PRMCMPEN12
#define PRCR1_V_PRMCMPEN13	3
#define PRCR1_M_PRMCMPEN13	1 << PRCR1_V_PRMCMPEN13
#define PRCR1_V_PRMCMPEN14	4
#define PRCR1_M_PRMCMPEN14	1 << PRCR1_V_PRMCMPEN14
#define PRCR1_V_PRMCMPEN15	5
#define PRCR1_M_PRMCMPEN15	1 << PRCR1_V_PRMCMPEN15
#define PRCR1_V_PRMCMPEN16	6
#define PRCR1_M_PRMCMPEN16	1 << PRCR1_V_PRMCMPEN16
#define PRCR1_V_PRMCMPEN17	7
#define PRCR1_M_PRMCMPEN17	1 << PRCR1_V_PRMCMPEN17
#define PRCR1_V_PRMCMPEN18	8
#define PRCR1_M_PRMCMPEN18	1 << PRCR1_V_PRMCMPEN18
#define PRCR1_V_PRMCMPEN19	9
#define PRCR1_M_PRMCMPEN19	1 << PRCR1_V_PRMCMPEN19
#define PRCR1_V_PRMWR1EN	10
#define PRCR1_M_PRMWR1EN	1 << PRCR1_V_PRMWR1EN
#define PRCR1_V_PRMRD1EN	11
#define PRCR1_M_PRMRD1EN	1 << PRCR1_V_PRMRD1EN
#define PRCR1_V_PRM1MIO		12
#define PRCR1_M_PRM1MIO		1 << PRCR1_V_PRM1MIO
#define PRCR1_V_PRM1EN		13
#define PRCR1_M_PRM1EN		1 << PRCR1_V_PRM1EN

/* 
**
**  Programmable Range Address Register 2 (PRAR2) - Index 0x024
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     9     PRMA		PRM 2 Address
**        <14>    1     PROSHDW2EN	Programmable Shadow Register 2 Enable
**        <15>    1     PROCS2EN	Programmable Chip Select 2 Enable
**
**	  <10:13> Reserved
**	
*/
#define PMC_PRAR2_REG		0x024
#define PRAR2_V_PRMA		0
#define PRAR2_M_PRMA		0x3FF << PRAR2_V_PRMA
#define PRAR2_V_PROSHDW2EN	14
#define PRAR2_M_PROSHDW2EN	1 << PRAR2_V_PROSHDW2EN
#define PRAR2_V_PROCS2EN	15
#define PRAR2_M_PROCS2EN	1 << PRAR2_V_PROCS2EN

/* 
**
**  Programmable Range Compare Register 2 (PRCR2) - Index 0x025
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PRMCMPEN10	PRM 2 Compare Enable 0
**        <1>     1     PRMCMPEN11	PRM 2 Compare Enable 1
**        <2>     1     PRMCMPEN12	PRM 2 Compare Enable 2
**        <3>     1     PRMCMPEN13	PRM 2 Compare Enable 3
**        <4>     1     PRMCMPEN14	PRM 2 Compare Enable 4
**        <5>     1     PRMCMPEN15	PRM 2 Compare Enable 5
**        <6>     1     PRMCMPEN16	PRM 2 Compare Enable 6
**        <7>     1     PRMCMPEN17	PRM 2 Compare Enable 7
**        <8>     1     PRMCMPEN18	PRM 2 Compare Enable 8
**        <9>     1     PRMCMPEN19	PRM 2 Compare Enable 9
**        <10>    1     PRMWR2EN	PRM 2 Compare Write Enable 
**        <11>    1     PRMRD2EN	PRM 2 Compare Read Enable 
**        <12>    1     PRM2MIO		PRM 2 Memory or I/O Compare Enable 
**        <13>    1     PRM2EN		PRM 2 Enable 
**
**	  <14:15> Reserved
**	
*/
#define PMC_PRCR2_REG		0x025
#define PMC_PRCR2_INIT		0x0000
#define PRCR2_V_PRMCMPEN	0
#define PRCR2_M_PRMCMPEN	0x3FF << PRCR2_V_PRMCMPEN
#define PRCR2_V_PRMCMPEN10	0
#define PRCR2_M_PRMCMPEN10	1 << PRCR2_V_PRMCMPEN10
#define PRCR2_V_PRMCMPEN11	1
#define PRCR2_M_PRMCMPEN11	1 << PRCR2_V_PRMCMPEN11
#define PRCR2_V_PRMCMPEN12	2
#define PRCR2_M_PRMCMPEN12	1 << PRCR2_V_PRMCMPEN12
#define PRCR2_V_PRMCMPEN13	3
#define PRCR2_M_PRMCMPEN13	1 << PRCR2_V_PRMCMPEN13
#define PRCR2_V_PRMCMPEN14	4
#define PRCR2_M_PRMCMPEN14	1 << PRCR2_V_PRMCMPEN14
#define PRCR2_V_PRMCMPEN15	5
#define PRCR2_M_PRMCMPEN15	1 << PRCR2_V_PRMCMPEN15
#define PRCR2_V_PRMCMPEN16	6
#define PRCR2_M_PRMCMPEN16	1 << PRCR2_V_PRMCMPEN16
#define PRCR2_V_PRMCMPEN17	7
#define PRCR2_M_PRMCMPEN17	1 << PRCR2_V_PRMCMPEN17
#define PRCR2_V_PRMCMPEN18	8
#define PRCR2_M_PRMCMPEN18	1 << PRCR2_V_PRMCMPEN18
#define PRCR2_V_PRMCMPEN19	9
#define PRCR2_M_PRMCMPEN19	1 << PRCR2_V_PRMCMPEN19
#define PRCR2_V_PRMWR2EN	10
#define PRCR2_M_PRMWR2EN	1 << PRCR2_V_PRMWR2EN
#define PRCR2_V_PRMRD2EN	11
#define PRCR2_M_PRMRD2EN	1 << PRCR2_V_PRMRD2EN
#define PRCR2_V_PRM2MIO		12
#define PRCR2_M_PRM2MIO		1 << PRCR2_V_PRM2MIO
#define PRCR2_V_PRM2EN		13
#define PRCR2_M_PRM2EN		1 << PRCR2_V_PRM2EN

/* 
**
**  Programmable Range Address Register 3 (PRAR3) - Index 0x026
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     9     PRMA		PRM 3 Address
**        <14>    1     PROSHDW3EN	Programmable Shadow Register 3 Enable
**        <15>    1     PROCS3EN	Programmable Chip Select 3 Enable
**
**	  <10:13> Reserved
**	
*/
#define PMC_PRAR3_REG		0x026
#define PMC_PRAR3_INIT		0x0000
#define PRAR3_V_PRMA		0
#define PRAR3_M_PRMA		0x3FF << PRAR3_V_PRMA
#define PRAR3_V_PROSHDW3EN	14
#define PRAR3_M_PROSHDW3EN	1 << PRAR3_V_PROSHDW3EN
#define PRAR3_V_PROCS3EN	15
#define PRAR3_M_PROCS3EN	1 << PRAR3_V_PROCS3EN

/* 
**
**  Programmable Range Compare Register 3 (PRCR3) - Index 0x027
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PRMCMPEN10	PRM 3 Compare Enable 0
**        <1>     1     PRMCMPEN11	PRM 3 Compare Enable 1
**        <2>     1     PRMCMPEN12	PRM 3 Compare Enable 2
**        <3>     1     PRMCMPEN13	PRM 3 Compare Enable 3
**        <4>     1     PRMCMPEN14	PRM 3 Compare Enable 4
**        <5>     1     PRMCMPEN15	PRM 3 Compare Enable 5
**        <6>     1     PRMCMPEN16	PRM 3 Compare Enable 6
**        <7>     1     PRMCMPEN17	PRM 3 Compare Enable 7
**        <8>     1     PRMCMPEN18	PRM 3 Compare Enable 8
**        <9>     1     PRMCMPEN19	PRM 3 Compare Enable 9
**        <10>    1     PRMWR3EN	PRM 3 Compare Write Enable 
**        <11>    1     PRMRD3EN	PRM 3 Compare Read Enable 
**        <12>    1     PRM3MIO		PRM 3 Memory or I/O Compare Enable 
**        <13>    1     PRM3EN		PRM 3 Enable 
**
**	  <14:15> Reserved
**	
*/
#define PMC_PRCR3_REG		0x027
#define PMC_PRCR3_INIT		0x0000
#define PRCR3_V_PRMCMPEN	0
#define PRCR3_M_PRMCMPEN	0x3FF << PRCR3_V_PRMCMPEN
#define PRCR3_V_PRMCMPEN10	0
#define PRCR3_M_PRMCMPEN10	1 << PRCR3_V_PRMCMPEN10
#define PRCR3_V_PRMCMPEN11	1
#define PRCR3_M_PRMCMPEN11	1 << PRCR3_V_PRMCMPEN11
#define PRCR3_V_PRMCMPEN12	2
#define PRCR3_M_PRMCMPEN12	1 << PRCR3_V_PRMCMPEN12
#define PRCR3_V_PRMCMPEN13	3
#define PRCR3_M_PRMCMPEN13	1 << PRCR3_V_PRMCMPEN13
#define PRCR3_V_PRMCMPEN14	4
#define PRCR3_M_PRMCMPEN14	1 << PRCR3_V_PRMCMPEN14
#define PRCR3_V_PRMCMPEN15	5
#define PRCR3_M_PRMCMPEN15	1 << PRCR3_V_PRMCMPEN15
#define PRCR3_V_PRMCMPEN16	6
#define PRCR3_M_PRMCMPEN16	1 << PRCR3_V_PRMCMPEN16
#define PRCR3_V_PRMCMPEN17	7
#define PRCR3_M_PRMCMPEN17	1 << PRCR3_V_PRMCMPEN17
#define PRCR3_V_PRMCMPEN18	8
#define PRCR3_M_PRMCMPEN18	1 << PRCR3_V_PRMCMPEN18
#define PRCR3_V_PRMCMPEN19	9
#define PRCR3_M_PRMCMPEN19	1 << PRCR3_V_PRMCMPEN19
#define PRCR3_V_PRMWR3EN	10
#define PRCR3_M_PRMWR3EN	1 << PRCR3_V_PRMWR3EN
#define PRCR3_V_PRMRD3EN	11
#define PRCR3_M_PRMRD3EN	1 << PRCR3_V_PRMRD3EN
#define PRCR3_V_PRM3MIO		12
#define PRCR3_M_PRM3MIO		1 << PRCR3_V_PRM3MIO
#define PRCR3_V_PRM3EN		13
#define PRCR3_M_PRM3EN		1 << PRCR3_V_PRM3EN

/* 
**
**  Programmable Time-out Timer Register 0 (PTOTR0) - Index 0x028
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PROGTMR00	Programmable Time-out Timer 0 0
**        <1>     1     PROGTMR01	Programmable Time-out Timer 0 1
**        <2>     1     PROGTMR02	Programmable Time-out Timer 0 2
**        <3>     1     PROGTMR03	Programmable Time-out Timer 0 3
**        <7>     1     TMR0SOURCE	Clock Source for Timer 0
**        <14>    1     TMR0PC2EN	Time-out Timer 0 to PC2 Enable
**        <15>    1     TMR0EN		Programmable Timer 0 Enable
**
**	  <4:6><8:13> Reserved
**	
*/
#define PMC_PTOTR0_REG		0x028
#define PMC_PTOTR0_INIT		0x0000
#define PTOTR0_V_PROGTMR0	0
#define PTOTR0_M_PROGTMR0	0xF << PTOTR0_V_PROGTMR0
#define PTOTR0_V_PROGTMR00	0
#define PTOTR0_M_PROGTMR00	1 << PTOTR0_V_PROGTMR00
#define PTOTR0_V_PROGTMR01	1
#define PTOTR0_M_PROGTMR01	1 << PTOTR0_V_PROGTMR01
#define PTOTR0_V_PROGTMR02	2
#define PTOTR0_M_PROGTMR02	1 << PTOTR0_V_PROGTMR02
#define PTOTR0_V_PROGTMR03	3
#define PTOTR0_M_PROGTMR03	1 << PTOTR0_V_PROGTMR03
#define PTOTR0_V_TMR0SOURCE	7
#define PTOTR0_M_TMR0SOURCE	1 << PTOTR0_V_TMR0SOURCE
#define PTOTR0_V_TMR0PC2EN	14
#define PTOTR0_M_TMR0PC2EN	1 << PTOTR0_V_TMR0PC2EN
#define PTOTR0_V_TMR0EN		15
#define PTOTR0_M_TMR0EN		1 << PTOTR0_V_TMR0EN

/*
** Progammable Time-out Timer 0 
*/
#define PROGTMR0_2S		0x000
#define PROGTMR0_5S		PTOTR0_M_PROGTMR00
#define PROGTMR0_10S		PTOTR0_M_PROGTMR01
#define PROGTMR0_15S		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR01)
#define PROGTMR0_30S		PTOTR0_M_PROGTMR02
#define PROGTMR0_45S		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR02)
#define PROGTMR0_60S		(PTOTR0_M_PROGTMR01 | PTOTR0_M_PROGTMR02)
#define PROGTMR0_90S		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR01 | PTOTR0_M_PROGTMR02)
#define PROGTMR0_2MIN		PTOTR0_M_PROGTMR03
#define PROGTMR0_3MIN		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR03)
#define PROGTMR0_4MIN		(PTOTR0_M_PROGTMR01 | PTOTR0_M_PROGTMR03)
#define PROGTMR0_6MIN		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR01 | PTOTR0_M_PROGTMR03
#define PROGTMR0_8MIN		(PTOTR0_M_PROGTMR02 | PTOTR0_M_PROGTMR03)
#define PROGTMR0_10MIN		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR02 | PTOTR0_M_PROGTMR03)
#define PROGTMR0_15MIN		(PTOTR0_M_PROGTMR01 | PTOTR0_M_PROGTMR02 | PTOTR0_M_PROGTMR03)
#define PROGTMR0_20MIN		(PTOTR0_M_PROGTMR00 | PTOTR0_M_PROGTMR01 | PTOTR0_M_PROGTMR02 | PTOTR0_M_PROGTMR03)

/* 
**
**  Programmable Time-out Timer Register 1 (PTOTR1) - Index 0x029
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PROGTMR10	Programmable Time-out Timer 1 0
**        <1>     1     PROGTMR11	Programmable Time-out Timer 1 1
**        <2>     1     PROGTMR12	Programmable Time-out Timer 1 2
**        <3>     1     PROGTMR13	Programmable Time-out Timer 1 3
**        <7>     1     TMR1SOURCE	Clock Source for Timer 1
**        <14>    1     TMR1PC3EN	Time-out Timer 1 to PC3 Enable
**        <15>    1     TMR1EN		Programmable Timer 1 Enable
**
**	  <4:6><8:13> Reserved
**	
*/
#define PMC_PTOTR1_REG		0x029
#define PMC_PTOTR1_INIT		0x0000
#define PTOTR1_V_PROGTMR1	0
#define PTOTR1_M_PROGTMR1	0xF << PTOTR1_V_PROGTMR1
#define PTOTR1_V_PROGTMR10	0
#define PTOTR1_M_PROGTMR10	1 << PTOTR1_V_PROGTMR10
#define PTOTR1_V_PROGTMR11	1
#define PTOTR1_M_PROGTMR11	1 << PTOTR1_V_PROGTMR11
#define PTOTR1_V_PROGTMR12	2
#define PTOTR1_M_PROGTMR12	1 << PTOTR1_V_PROGTMR12
#define PTOTR1_V_PROGTMR13	3
#define PTOTR1_M_PROGTMR13	1 << PTOTR1_V_PROGTMR13
#define PTOTR1_V_TMR1SOURCE	7
#define PTOTR1_M_TMR1SOURCE	1 << PTOTR1_V_TMR1SOURCE
#define PTOTR1_V_TMR1PC3EN	14
#define PTOTR1_M_TMR1PC3EN	1 << PTOTR1_V_TMR1PC3EN
#define PTOTR1_V_TMR1EN		15
#define PTOTR1_M_TMR1EN		1 << PTOTR1_V_TMR1EN

/*
** Progammable Time-out Timer 1 
*/
#define PROGTMR1_2S		0x000
#define PROGTMR1_5S		PTOTR1_M_PROGTMR10
#define PROGTMR1_10S		PTOTR1_M_PROGTMR11
#define PROGTMR1_15S		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR11)
#define PROGTMR1_30S		PTOTR1_M_PROGTMR12
#define PROGTMR1_45S		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR12)
#define PROGTMR1_60S		(PTOTR1_M_PROGTMR11 | PTOTR1_M_PROGTMR12)
#define PROGTMR1_90S		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR11 | PTOTR1_M_PROGTMR12)
#define PROGTMR1_2MIN		PTOTR1_M_PROGTMR13
#define PROGTMR1_3MIN		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR13)
#define PROGTMR1_4MIN		(PTOTR1_M_PROGTMR11 | PTOTR1_M_PROGTMR13)
#define PROGTMR1_6MIN		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR11 | PTOTR1_M_PROGTMR13
#define PROGTMR1_8MIN		(PTOTR1_M_PROGTMR12 | PTOTR1_M_PROGTMR13)
#define PROGTMR1_10MIN		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR12 | PTOTR1_M_PROGTMR13)
#define PROGTMR1_15MIN		(PTOTR1_M_PROGTMR11 | PTOTR1_M_PROGTMR12 | PTOTR1_M_PROGTMR13)
#define PROGTMR1_20MIN		(PTOTR1_M_PROGTMR10 | PTOTR1_M_PROGTMR11 | PTOTR1_M_PROGTMR12 | PTOTR0_M_PROGTMR13)

/* 
**
**  Programmable Time-out Timer Register 2 (PTOTR2) - Index 0x02A
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PROGTMR20	Programmable Time-out Timer 2 0
**        <1>     1     PROGTMR21	Programmable Time-out Timer 2 1
**        <2>     1     PROGTMR22	Programmable Time-out Timer 2 2
**        <3>     1     PROGTMR23	Programmable Time-out Timer 2 3
**        <7>     1     TMR2SOURCE	Clock Source for Timer 2
**        <14>    1     TMR2PC8EN	Time-out Timer 2 to PC8 Enable
**        <15>    1     TMR2EN		Programmable Timer 2 Enable
**
**	  <4:6><8:13> Reserved
**	
*/
#define PMC_PTOTR2_REG		0x02A
#define PMC_PTOTR2_INIT		0x0000
#define PTOTR2_V_PROGTMR2	0
#define PTOTR2_M_PROGTMR2	0xF << PTOTR2_V_PROGTMR2
#define PTOTR2_V_PROGTMR20	0
#define PTOTR2_M_PROGTMR20	1 << PTOTR2_V_PROGTMR20
#define PTOTR2_V_PROGTMR21	1
#define PTOTR2_M_PROGTMR21	1 << PTOTR2_V_PROGTMR21
#define PTOTR2_V_PROGTMR22	2
#define PTOTR2_M_PROGTMR22	1 << PTOTR2_V_PROGTMR22
#define PTOTR2_V_PROGTMR23	3
#define PTOTR2_M_PROGTMR23	1 << PTOTR2_V_PROGTMR23
#define PTOTR2_V_TMR2SOURCE	7
#define PTOTR2_M_TMR2SOURCE	1 << PTOTR2_V_TMR2SOURCE
#define PTOTR2_V_TMR2PC8EN	14
#define PTOTR2_M_TMR2PC8EN	1 << PTOTR2_V_TMR2PC8EN
#define PTOTR2_V_TMR2EN		15
#define PTOTR2_M_TMR2EN		1 << PTOTR2_V_TMR2EN

/*
** Progammable Time-out Timer 2
*/
#define PROGTMR2_2S		0x000
#define PROGTMR2_5S		PTOTR2_M_PROGTMR20
#define PROGTMR2_10S		PTOTR2_M_PROGTMR21
#define PROGTMR2_15S		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR21)
#define PROGTMR2_30S		PTOTR2_M_PROGTMR22
#define PROGTMR2_45S		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR22)
#define PROGTMR2_60S		(PTOTR2_M_PROGTMR21 | PTOTR2_M_PROGTMR22)
#define PROGTMR2_90S		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR21 | PTOTR2_M_PROGTMR22)
#define PROGTMR2_2MIN		PTOTR2_M_PROGTMR23
#define PROGTMR2_3MIN		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR23)
#define PROGTMR2_4MIN		(PTOTR2_M_PROGTMR21 | PTOTR2_M_PROGTMR23)
#define PROGTMR2_6MIN		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR21 | PTOTR2_M_PROGTMR23
#define PROGTMR2_8MIN		(PTOTR2_M_PROGTMR22 | PTOTR2_M_PROGTMR23)
#define PROGTMR2_10MIN		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR22 | PTOTR2_M_PROGTMR23)
#define PROGTMR2_15MIN		(PTOTR2_M_PROGTMR21 | PTOTR2_M_PROGTMR22 | PTOTR2_M_PROGTMR23)
#define PROGTMR2_20MIN		(PTOTR2_M_PROGTMR20 | PTOTR2_M_PROGTMR21 | PTOTR2_M_PROGTMR22 | PTOTR0_M_PROGTMR23)

/* 
**
**  Programmable Time-out Timer Register 3 (PTOTR3) - Index 0x02B
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PROGTMR30	Programmable Time-out Timer 3 0
**        <1>     1     PROGTMR31	Programmable Time-out Timer 3 1
**        <2>     1     PROGTMR32	Programmable Time-out Timer 3 2
**        <3>     1     PROGTMR33	Programmable Time-out Timer 3 3
**        <7>     1     TMR3SOURCE	Clock Source for Timer 3
**        <14>    1     TMR3PC9EN	Time-out Timer 3 to PC9 Enable
**        <15>    1     TMR3EN		Programmable Timer 3 Enable
**
**	  <4:6><8:13> Reserved
**	
*/
#define PMC_PTOTR3_REG		0x02B
#define PMC_PTOTR3_INIT		0x0000
#define PTOTR3_V_PROGTMR3	0
#define PTOTR3_M_PROGTMR3	0xF << PTOTR3_V_PROGTMR3
#define PTOTR3_V_PROGTMR30	0
#define PTOTR3_M_PROGTMR30	1 << PTOTR3_V_PROGTMR30
#define PTOTR3_V_PROGTMR31	1
#define PTOTR3_M_PROGTMR31	1 << PTOTR3_V_PROGTMR31
#define PTOTR3_V_PROGTMR32	2
#define PTOTR3_M_PROGTMR32	1 << PTOTR3_V_PROGTMR32
#define PTOTR3_V_PROGTMR33	3
#define PTOTR3_M_PROGTMR33	1 << PTOTR3_V_PROGTMR33
#define PTOTR3_V_TMR3SOURCE	7
#define PTOTR3_M_TMR3SOURCE	1 << PTOTR3_V_TMR3SOURCE
#define PTOTR3_V_TMR3PC9EN	14
#define PTOTR3_M_TMR3PC9EN	1 << PTOTR3_V_TMR3PC9EN
#define PTOTR3_V_TMR3EN		15
#define PTOTR3_M_TMR3EN		1 << PTOTR3_V_TMR3EN

/*
** Progammable Time-out Timer 3
*/
#define PROGTMR3_2S		0x000
#define PROGTMR3_5S		PTOTR3_M_PROGTMR30
#define PROGTMR3_10S		PTOTR3_M_PROGTMR31
#define PROGTMR3_15S		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR31)
#define PROGTMR3_30S		PTOTR3_M_PROGTMR32
#define PROGTMR3_45S		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR32)
#define PROGTMR3_60S		(PTOTR3_M_PROGTMR31 | PTOTR3_M_PROGTMR32)
#define PROGTMR3_90S		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR31 | PTOTR3_M_PROGTMR32)
#define PROGTMR3_2MIN		PTOTR3_M_PROGTMR33
#define PROGTMR3_3MIN		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR33)
#define PROGTMR3_4MIN		(PTOTR3_M_PROGTMR31 | PTOTR3_M_PROGTMR33)
#define PROGTMR3_6MIN		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR31 | PTOTR3_M_PROGTMR33
#define PROGTMR3_8MIN		(PTOTR3_M_PROGTMR32 | PTOTR3_M_PROGTMR33)
#define PROGTMR3_10MIN		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR32 | PTOTR3_M_PROGTMR33)
#define PROGTMR3_15MIN		(PTOTR3_M_PROGTMR31 | PTOTR3_M_PROGTMR32 | PTOTR3_M_PROGTMR33)
#define PROGTMR3_20MIN		(PTOTR3_M_PROGTMR30 | PTOTR3_M_PROGTMR31 | PTOTR3_M_PROGTMR32 | PTOTR0_M_PROGTMR33)

/* 
**
**  Programmable Time-out Timer Source Register 1 (PTOTSR1) - Index 0x02C
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     VDTMRSEL0	Video Activity Time-out Timer Select 0
**        <1>     1     VDTMRSEL1	Video Activity Time-out Timer Select 1
**        <3>     1     VDTMRSELEN	Video Activity Time-out Timer Select Enable
**        <4>     1     HDTMRSEL0	Hard Drive Activity Time-out Timer Select 0
**        <5>     1     HDTMRSEL1	Hard Drive Activity Time-out Timer Select 1
**        <7>     1     HDTMRSELEN	Hard Drive Activity Time-out Timer Select Enable 
**        <8>     1     FDTMRSEL0	Floppy Drive Activity Time-out Timer Select 0
**        <9>     1     FDTMRSEL1	Floppy Drive Activity Time-out Timer Select 1
**        <11>    1     FDTMRSELEN	Floppy Drive Activity Time-out Timer Select Enable 
**        <12>    1     KBTMRSEL0	Keyboard Activity Time-out Timer Select 0
**        <13>    1     KBTMRSEL1	Keyboard Activity Time-out Timer Select 1
**        <15>    1     KBTMRSELEN	Keyboard Activity Time-out Timer Select Enable 
**
**	  <2><6><10><14> Reserved
**	
*/
#define PMC_PTOTSR1_REG		0x02C
#define PMC_PTOTSR1_INIT	0x0000
#define PTOTSR1_V_VDTMRSEL	0
#define PTOTSR1_M_VDTMRSEL	0x3 << PTOTSR1_V_VDTMRSEL
#define PTOTSR1_V_VDTMRSEL0	0
#define PTOTSR1_M_VDTMRSEL0	1 << PTOTSR1_V_VDTMRSEL0
#define PTOTSR1_V_VDTMRSEL1	1
#define PTOTSR1_M_VDTMRSEL1	1 << PTOTSR1_V_VDTMRSEL1
#define PTOTSR1_V_VDTMRSELEN	3
#define PTOTSR1_M_VDTMRSELEN	1 << PTOTSR1_V_VDTMRSELEN
#define PTOTSR1_V_HDTMRSEL	4
#define PTOTSR1_M_HDTMRSEL	0x3 << PTOTSR1_V_HDTMRSEL
#define PTOTSR1_V_HDTMRSEL0	4
#define PTOTSR1_M_HDTMRSEL0	1 << PTOTSR1_V_HDTMRSEL0
#define PTOTSR1_V_HDTMRSEL1	5
#define PTOTSR1_M_HDTMRSEL1	1 << PTOTSR1_V_HDTMRSEL1
#define PTOTSR1_V_HDTMRSELEN	7
#define PTOTSR1_M_HDTMRSELEN	1 << PTOTSR1_V_HDTMRSELEN
#define PTOTSR1_V_FDTMRSEL	8
#define PTOTSR1_M_FDTMRSEL	0x3 << PTOTSR1_V_FDTMRSEL
#define PTOTSR1_V_FDTMRSEL0	8
#define PTOTSR1_M_FDTMRSEL0	1 << PTOTSR1_V_FDTMRSEL0
#define PTOTSR1_V_FDTMRSEL1	9
#define PTOTSR1_M_FDTMRSEL1	1 << PTOTSR1_V_FDTMRSEL1
#define PTOTSR1_V_FDTMRSELEN	11
#define PTOTSR1_M_FDTMRSELEN	1 << PTOTSR1_V_FDTMRSELEN
#define PTOTSR1_V_KBTMRSEL	12
#define PTOTSR1_M_KBTMRSEL	0x3 << PTOTSR1_V_KBTMRSEL
#define PTOTSR1_V_KBTMRSEL0	12
#define PTOTSR1_M_KBTMRSEL0	1 << PTOTSR1_V_KBTMRSEL0
#define PTOTSR1_V_KBTMRSEL1	13
#define PTOTSR1_M_KBTMRSEL1	1 << PTOTSR1_V_KBTMRSEL1
#define PTOTSR1_V_KBTMRSELEN	15
#define PTOTSR1_M_KBTMRSELEN	1 << PTOTSR1_V_KBTMRSELEN

/*
** Video Activity Progammable Time-out Timer
*/
#define VDTMRSEL_0		0x000
#define VDTMRSEL_1		PTOTSR1_M_VDTMRSEL0
#define VDTMRSEL_2		PTOTSR1_M_VDTMRSEL1
#define VDTMRSEL_3		(PTOTSR1_M_VDTMRSEL0 | PTOTSR1_M_VDTMRSEL1)

/*
** Hard Drive Activity Progammable Time-out Timer
*/
#define HDTMRSEL_0		0x000
#define HDTMRSEL_1		PTOTSR1_M_HDTMRSEL0
#define HDTMRSEL_2		PTOTSR1_M_HDTMRSEL1
#define HDTMRSEL_3		(PTOTSR1_M_HDTMRSEL0 | PTOTSR1_M_HDTMRSEL1)

/*
** Floppy Drive Activity Progammable Time-out Timer
*/
#define FDTMRSEL_0		0x000
#define FDTMRSEL_1		PTOTSR1_M_FDTMRSEL0
#define FDTMRSEL_2		PTOTSR1_M_FDTMRSEL1
#define FDTMRSEL_3		(PTOTSR1_M_FDTMRSEL0 | PTOTSR1_M_FDTMRSEL1)

/*
** Keyboard Activity Progammable Time-out Timer
*/
#define KBTMRSEL_0		0x000
#define KBTMRSEL_1		PTOTSR1_M_KBTMRSEL0
#define KBTMRSEL_2		PTOTSR1_M_KBTMRSEL1
#define KBTMRSEL_3		(PTOTSR1_M_KBTMRSEL0 | PTOTSR1_M_KBTMRSEL1)

/* 
**
**  Programmable Time-out Timer Source Register 2 (PTOTSR2) - Index 0x02D
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SIOTMRSEL0	Serial Port Activity Time-out Timer Select 0
**        <1>     1     SIOTMRSEL1	Serial Port Activity Time-out Timer Select 1
**        <3>     1     SIOTMRSELEN	Serial Port Activity Time-out Timer Select Enable
**        <4>     1     PIOTMRSEL0	Parallel Port Activity Time-out Timer Select 0
**        <5>     1     PIOTMRSEL1	Parallel Port Activity Time-out Timer Select 1
**        <7>     1     PIOTMRSELEN	Parallel Port Activity Time-out Timer Select Enable 
**        <8>     1     PR0TMRSEL0	PRM 0 Activity Time-out Timer Select 0
**        <9>     1     PR0TMRSEL1	PRM 0 Activity Time-out Timer Select 1
**        <11>    1     PR0TMRSELEN	PRM 0 Activity Time-out Timer Select Enable 
**        <12>    1     PR1TMRSEL0	PRM 1 Activity Time-out Timer Select 0
**        <13>    1     PR1TMRSEL1	PRM 1 Activity Time-out Timer Select 1
**        <15>    1     PR1TMRSELEN	PRM 1 Activity Time-out Timer Select Enable 
**
**	  <2><6><10><14> Reserved
**	
*/
#define PMC_PTOTSR2_REG		0x02D
#define PMC_PTOTSR2_INIT	0x0000
#define PTOTSR2_V_SIOTMRSEL	0
#define PTOTSR2_M_SIOTMRSEL	0x3 << PTOTSR2_V_SIOTMRSEL
#define PTOTSR2_V_SIOTMRSEL0	0
#define PTOTSR2_M_SIOTMRSEL0	1 << PTOTSR2_V_SIOTMRSEL0
#define PTOTSR2_V_SIOTMRSEL1	1
#define PTOTSR2_M_SIOTMRSEL1	1 << PTOTSR2_V_SIOTMRSEL1
#define PTOTSR2_V_SIOTMRSELEN	3
#define PTOTSR2_M_SIOTMRSELEN	1 << PTOTSR2_V_SIOTMRSELEN
#define PTOTSR2_V_PIOTMRSEL	4
#define PTOTSR2_M_PIOTMRSEL	0x3 << PTOTSR2_V_PIOTMRSEL
#define PTOTSR2_V_PIOTMRSEL0	4
#define PTOTSR2_M_PIOTMRSEL0	1 << PTOTSR2_V_PIOTMRSEL0
#define PTOTSR2_V_PIOTMRSEL1	5
#define PTOTSR2_M_PIOTMRSEL1	1 << PTOTSR2_V_PIOTMRSEL1
#define PTOTSR2_V_PIOTMRSELEN	7
#define PTOTSR2_M_PIOTMRSELEN	1 << PTOTSR2_V_PIOTMRSELEN
#define PTOTSR2_V_PR0TMRSEL	8
#define PTOTSR2_M_PR0TMRSEL	0x3 << PTOTSR2_V_PR0TMRSEL
#define PTOTSR2_V_PR0TMRSEL0	8
#define PTOTSR2_M_PR0TMRSEL0	1 << PTOTSR2_V_PR0TMRSEL0
#define PTOTSR2_V_PR0TMRSEL1	9
#define PTOTSR2_M_PR0TMRSEL1	1 << PTOTSR2_V_PR0TMRSEL1
#define PTOTSR2_V_PR0TMRSELEN	11
#define PTOTSR2_M_PR0TMRSELEN	1 << PTOTSR2_V_PR0TMRSELEN
#define PTOTSR2_V_PR1TMRSEL	12
#define PTOTSR2_M_PR1TMRSEL	0x3 << PTOTSR2_V_PR1TMRSEL
#define PTOTSR2_V_PR1TMRSEL0	12
#define PTOTSR2_M_PR1TMRSEL0	1 << PTOTSR2_V_PR1TMRSEL0
#define PTOTSR2_V_PR1TMRSEL1	13
#define PTOTSR2_M_PR1TMRSEL1	1 << PTOTSR2_V_PR1TMRSEL1
#define PTOTSR2_V_PR1TMRSELEN	15
#define PTOTSR2_M_PR1TMRSELEN	1 << PTOTSR2_V_PR1TMRSELEN

/*
** Serial Port Activity Progammable Time-out Timer
*/
#define SIOTMRSEL_0		0x000
#define SIOTMRSEL_1		PTOTSR2_M_SIOTMRSEL0
#define SIOTMRSEL_2		PTOTSR2_M_SIOTMRSEL1
#define SIOTMRSEL_3		(PTOTSR2_M_SIOTMRSEL0 | PTOTSR2_M_SIOTMRSEL1)

/*
** Parallel Port Activity Progammable Time-out Timer
*/
#define PIOTMRSEL_0		0x000
#define PIOTMRSEL_1		PTOTSR2_M_PIOTMRSEL0
#define PIOTMRSEL_2		PTOTSR2_M_PIOTMRSEL1
#define PIOTMRSEL_3		(PTOTSR2_M_PIOTMRSEL0 | PTOTSR2_M_PIOTMRSEL1)

/*
** PRM 0 Activity Progammable Time-out Timer
*/
#define PR0TMRSEL_0		0x000
#define PR0TMRSEL_1		PTOTSR2_M_PR0TMRSEL0
#define PR0TMRSEL_2		PTOTSR2_M_PR0TMRSEL1
#define PR0TMRSEL_3		(PTOTSR2_M_PR0TMRSEL0 | PTOTSR2_M_PR0TMRSEL1)

/*
** PRM 1 Activity Progammable Time-out Timer
*/
#define PR1TMRSEL_0		0x000
#define PR1TMRSEL_1		PTOTSR2_M_PR1TMRSEL0
#define PR1TMRSEL_2		PTOTSR2_M_PR1TMRSEL1
#define PR1TMRSEL_3		(PTOTSR2_M_PR1TMRSEL0 | PTOTSR2_M_PR1TMRSEL1)

/* 
**
**  Programmable Time-out Timer Source Register 3 (PTOTSR3) - Index 0x02E
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PR2TMRSEL0	PRM 2 Activity Time-out Timer Select 0
**        <1>     1     PR2TMRSEL1	PRM 2 Activity Time-out Timer Select 1
**        <3>     1     PR2TMRSELEN	PRM 2 Activity Time-out Timer Select Enable
**        <4>     1     PR3TMRSEL0	PRM 3 Activity Time-out Timer Select 0
**        <5>     1     PR3TMRSEL1	PRM 3 Activity Time-out Timer Select 1
**        <7>     1     PR3TMRSELEN	PRM 3 Activity Time-out Timer Select Enable 
**
**	  <2><6><8:15> Reserved
**	
*/
#define PMC_PTOTSR3_REG		0x02E
#define PMC_PTOTSR3_INIT	0x0000
#define PTOTSR3_V_PR2TMRSEL	0
#define PTOTSR3_M_PR2TMRSEL	0x3 << PTOTSR3_V_PR2TMRSEL
#define PTOTSR3_V_PR2TMRSEL0	0
#define PTOTSR3_M_PR2TMRSEL0	1 << PTOTSR3_V_PR2TMRSEL0
#define PTOTSR3_V_PR2TMRSEL1	1
#define PTOTSR3_M_PR2TMRSEL1	1 << PTOTSR3_V_PR2TMRSEL1
#define PTOTSR3_V_PR2TMRSELEN	3
#define PTOTSR3_M_PR2TMRSELEN	1 << PTOTSR3_V_PR2TMRSELEN
#define PTOTSR3_V_PR3TMRSEL	4
#define PTOTSR3_M_PR3TMRSEL	0x3 << PTOTSR3_V_PR3TMRSEL
#define PTOTSR3_V_PR3TMRSEL0	4
#define PTOTSR3_M_PR3TMRSEL0	1 << PTOTSR3_V_PR3TMRSEL0
#define PTOTSR3_V_PR3TMRSEL1	5
#define PTOTSR3_M_PR3TMRSEL1	1 << PTOTSR3_V_PR3TMRSEL1
#define PTOTSR3_V_PR3TMRSELEN	7
#define PTOTSR3_M_PR3TMRSELEN	1 << PTOTSR3_V_PR3TMRSELEN

/*
** PRM 2 Activity Progammable Time-out Timer
*/
#define PR2TMRSEL_0		0x000
#define PR2TMRSEL_1		PTOTSR3_M_PR2TMRSEL0
#define PR2TMRSEL_2		PTOTSR3_M_PR2TMRSEL1
#define PR2TMRSEL_3		(PTOTSR3_M_PR2TMRSEL0 | PTOTSR3_M_PR2TMRSEL1)

/*
** PRM 3 Activity Progammable Time-out Timer
*/
#define PR3TMRSEL_0		0x000
#define PR3TMRSEL_1		PTOTSR3_M_PR3TMRSEL0
#define PR3TMRSEL_2		PTOTSR3_M_PR3TMRSEL1
#define PR3TMRSEL_3		(PTOTSR3_M_PR3TMRSEL0 | PTOTSR3_M_PR3TMRSEL1)

/* 
**
**  Programmable Time-out Timer Source Register 4 (PTOTSR4) - Index 0x02F
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     EXT0TMRSEL0	EXTACT0 Activity Time-out Timer Select 0
**        <1>     1     EXT0TMRSEL1	EXTACT0 Activity Time-out Timer Select 1
**        <3>     1     EXT0TMRSELEN	EXTACT0 Activity Time-out Timer Select Enable
**        <4>     1     EXT1TMRSEL0	EXTACT1 Activity Time-out Timer Select 0
**        <5>     1     EXT1TMRSEL1	EXTACT1 Activity Time-out Timer Select 1
**        <7>     1     EXT1TMRSELEN	EXTACT1 Activity Time-out Timer Select Enable 
**        <8>     1     EXT2TMRSEL0	EXTACT2 Activity Time-out Timer Select 0
**        <9>     1     EXT2TMRSEL1	EXTACT2 Activity Time-out Timer Select 1
**        <11>    1     EXT2TMRSELEN	EXTACT2 Activity Time-out Timer Select Enable 
**        <12>    1     EXT3TMRSEL0	EXTACT3 Activity Time-out Timer Select 0
**        <13>    1     EXT3TMRSEL1	EXTACT3 Activity Time-out Timer Select 1
**        <15>    1     EXT3TMRSELEN	EXTACT3 Activity Time-out Timer Select Enable 
**
**	  <2><6><10><14> Reserved
**	
*/
#define PMC_PTOTSR4_REG		0x02F
#define PMC_PTOTSR4_INIT	0x0000
#define PTOTSR4_V_EXT0TMRSEL	0
#define PTOTSR4_M_EXT0TMRSEL	0x3 << PTOTSR4_V_EXT0TMRSEL
#define PTOTSR4_V_EXT0TMRSEL0	0
#define PTOTSR4_M_EXT0TMRSEL0	1 << PTOTSR4_V_EXT0TMRSEL0
#define PTOTSR4_V_EXT0TMRSEL1	1
#define PTOTSR4_M_EXT0TMRSEL1	1 << PTOTSR4_V_EXT0TMRSEL1
#define PTOTSR4_V_EXT0TMRSELEN	3
#define PTOTSR4_M_EXT0TMRSELEN	1 << PTOTSR4_V_EXT0TMRSELEN
#define PTOTSR4_V_EXT1TMRSEL	4
#define PTOTSR4_M_EXT1TMRSEL	0x3 << PTOTSR4_V_EXT1TMRSEL
#define PTOTSR4_V_EXT1TMRSEL0	4
#define PTOTSR4_M_EXT1TMRSEL0	1 << PTOTSR4_V_EXT1TMRSEL0
#define PTOTSR4_V_EXT1TMRSEL1	5
#define PTOTSR4_M_EXT1TMRSEL1	1 << PTOTSR4_V_EXT1TMRSEL1
#define PTOTSR4_V_EXT1TMRSELEN	7
#define PTOTSR4_M_EXT1TMRSELEN	1 << PTOTSR4_V_EXT1TMRSELEN
#define PTOTSR4_V_EXT2TMRSEL	8
#define PTOTSR4_M_EXT2TMRSEL	0x3 << PTOTSR4_V_EXT2TMRSEL
#define PTOTSR4_V_EXT2TMRSEL0	8
#define PTOTSR4_M_EXT2TMRSEL0	1 << PTOTSR4_V_EXT2TMRSEL0
#define PTOTSR4_V_EXT2TMRSEL1	9
#define PTOTSR4_M_EXT2TMRSEL1	1 << PTOTSR4_V_EXT2TMRSEL1
#define PTOTSR4_V_EXT2TMRSELEN	11
#define PTOTSR4_M_EXT2TMRSELEN	1 << PTOTSR4_V_EXT2TMRSELEN
#define PTOTSR4_V_EXT3TMRSEL	12
#define PTOTSR4_M_EXT3TMRSEL	0x3 << PTOTSR4_V_EXT3TMRSEL
#define PTOTSR4_V_EXT3TMRSEL0	12
#define PTOTSR4_M_EXT3TMRSEL0	1 << PTOTSR4_V_EXT3TMRSEL0
#define PTOTSR4_V_EXT3TMRSEL1	13
#define PTOTSR4_M_EXT3TMRSEL1	1 << PTOTSR4_V_EXT3TMRSEL1
#define PTOTSR4_V_EXT3TMRSELEN	15
#define PTOTSR4_M_EXT3TMRSELEN	1 << PTOTSR4_V_EXT3TMRSELEN

/*
** EXTACT0 Activity Progammable Time-out Timer
*/
#define EXT0TMRSEL_0		0x000
#define EXT0TMRSEL_1		PTOTSR4_M_EXT0TMRSEL0
#define EXT0TMRSEL_2		PTOTSR4_M_EXT0TMRSEL1
#define EXT0TMRSEL_3		(PTOTSR4_M_EXT0TMRSEL0 | PTOTSR4_M_EXT0TMRSEL1)

/*
** EXTACT1 Activity Progammable Time-out Timer
*/
#define EXT1TMRSEL_0		0x000
#define EXT1TMRSEL_1		PTOTSR4_M_EXT1TMRSEL0
#define EXT1TMRSEL_2		PTOTSR4_M_EXT1TMRSEL1
#define EXT1TMRSEL_3		(PTOTSR4_M_EXT1TMRSEL0 | PTOTSR4_M_EXT1TMRSEL1)

/*
** EXTACT2 Activity Progammable Time-out Timer
*/
#define EXT2TMRSEL_0		0x000
#define EXT2TMRSEL_1		PTOTSR4_M_EXT2TMRSEL0
#define EXT2TMRSEL_2		PTOTSR4_M_EXT2TMRSEL1
#define EXT2TMRSEL_3		(PTOTSR4_M_EXT2TMRSEL0 | PTOTSR4_M_EXT2TMRSEL1)

/*
** EXTACT3 Activity Progammable Time-out Timer
*/
#define EXT3TMRSEL_0		0x000
#define EXT3TMRSEL_1		PTOTSR4_M_EXT3TMRSEL0
#define EXT3TMRSEL_2		PTOTSR4_M_EXT3TMRSEL1
#define EXT3TMRSEL_3		(PTOTSR4_M_EXT3TMRSEL0 | PTOTSR4_M_EXT3TMRSEL1)

/*
** System Registers 
**
*/

/* 
**
**  Power-On Register (POR) - Index 0x100
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SEQUOIASEL	SEQUOIA Select 
**        <3>     1     DLYBDEN		Delay BD Bus Enable
**        <4>     1     CPUMODESEL	CPU Mode Select 
**        <6>     1     DLYBDBUSEN	Delay BADS# & BDEV# Enable
**	  <7>	  1     MISCCF0		Miscellaneous Configuration 0
**	  <8>	  1     MISCCF1		Miscellaneous Configuration 1
**	  <9>	  1     MISCCF2		Miscellaneous Configuration 2
**	  <11>	  1     EXTRTC		Optional External RTC
**	  <12>	  1     1X2XCLKSEL	1X/2X Clock Select
**
**	  <1:2><5><10><13:15> Reserved
*/
#define SR_POR_REG		0x100
#define POR_V_SEQUOIASEL	0
#define POR_M_SEQUOIASEL	1 << POR_V_SEQUOIASEL
#define POR_V_DLYBDEN		3
#define POR_M_DLYBDEN		1 << POR_V_DLYBDEN
#define POR_V_CPUMODESEL	4
#define POR_M_CPUMODESEL	1 << POR_V_CPUMODESEL
#define POR_V_DLYBDBUSEN	6
#define POR_M_DLYBDBUSEN	1 << POR_V_DLYBDBUSEN
#define POR_V_MISCCF		7
#define POR_M_MISCCF 		0x7 << POR_V_MISCCF
#define POR_V_MISCCF0		7
#define POR_M_MISCCF0 		1 << POR_V_MISCCF0
#define POR_V_MISCCF1		8
#define POR_M_MISCCF1 		1 << POR_V_MISCCF1
#define POR_V_MISCCF2		9
#define POR_M_MISCCF2 		1 << POR_V_MISCCF2
#define POR_V_EXTRTC		11
#define POR_M_EXTRTC		1 << POR_V_EXTRTC
#define POR_V_1X2XCLKSEL	12
#define POR_M_1X2XCLKSEL	1 << POR_V_1X2XCLKSEL

/* 
**
**  Non-Cacheable Region 1 Register (NCR1R) - Index 0x101
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     NCR1EN		Enable Non-Cacheable Region 1
**        <1>     1     NCR1BS0		Non-Cacheable Region 1 Block Size 0
**        <2>     1     NCR1BS1		Non-Cacheable Region 1 Block Size 1
**        <3>     13    NCR1A		Non-Cacheable Region 1 Starting Addr
**
*/
#define SR_NCR1R_REG		0x101
#define SR_NCR1R_INIT		0x0000
#define NCR1R_V_NCR1EN		0
#define NCR1R_M_NCR1EN		1 << NCR1R_V_NCR1EN
#define NCR1R_V_NCR1BS		1
#define NCR1R_M_NCR1BS		0x3 << NCR1R_V_NCR1BS
#define NCR1R_V_NCR1BS0		1
#define NCR1R_M_NCR1BS0		1 << NCR1R_V_NCR1BS0
#define NCR1R_V_NCR1BS1		1
#define NCR1R_M_NCR1BS1		1 << NCR1R_V_NCR1BS1
#define NCR1R_V_NCR1A		3
#define NCR1R_M_NCR1A		0xFFF8 << NCR1R_V_NCR1A

/*
** Non-Cacheable Region 1 Block Size (Kbytes)
*/
#define NCR1BS_32K		0x000
#define NCR1BS_64K		NCR1R_M_NCR1BS0
#define NCR1BS_128K		NCR1R_M_NCR1BS1
#define NCR1BS_256K		(NCR1R_M_NCR1BS0 | NCR1R_M_NCR1BS1)

/* 
**
**  Non-Cacheable Region 2 Register (NCR2R) - Index 0x102
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     NCR2EN		Enable Non-Cacheable Region 2
**        <1>     1     NCR2BS0		Non-Cacheable Region 2 Block Size 0
**        <2>     1     NCR2BS1		Non-Cacheable Region 2 Block Size 1
**        <3>     13    NCR2A		Non-Cacheable Region 2 Starting Addr
**
*/
#define SR_NCR2R_REG		0x102
#define SR_NCR2R_INIT		0x0000
#define NCR2R_V_NCR2EN		0
#define NCR2R_M_NCR2EN		1 << NCR2R_V_NCR2EN
#define NCR2R_V_NCR2BS		1
#define NCR2R_M_NCR2BS		0x3 << NCR2R_V_NCR2BS
#define NCR2R_V_NCR2BS0		1
#define NCR2R_M_NCR2BS0		1 << NCR2R_V_NCR2BS0
#define NCR2R_V_NCR2BS1		1
#define NCR2R_M_NCR2BS1		1 << NCR2R_V_NCR2BS1
#define NCR2R_V_NCR2A		3
#define NCR2R_M_NCR2A		0xFFF8 << NCR2R_V_NCR2A

/*
** Non-Cacheable Region 2 Block Size (Kbytes)
*/
#define NCR2BS_32K		0x000
#define NCR2BS_64K		NCR2R_M_NCR2BS0
#define NCR2BS_128K		NCR2R_M_NCR2BS1
#define NCR2BS_256K		(NCR2R_M_NCR2BS0 | NCR2R_M_NCR2BS1)

/* 
**
**  SYS Miscellaneous Control Register 1 (SYSMCR1) - Index 0x103
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     LDSMIHLDER	Load SMI Handler into SMM Space
**        <1>     1     MOVRLYEN	Memory Overlay Enable
**        <2>     1     SMMMAPSEL	SMM Memory Map Select 
**        <3>     1     SMMDETECT	SMM Detect
**        <4>     1     FLUSHWSMI	Generate FLUSH# with SMI# Active
**        <5>     1     SMIFLUSHIN	SMI Flush In
**        <6>     1     SMIFLUSHOUT	SMI Flush Out
**        <7>     1     SMMKENDIS	SMM KEN Disable
**        <8>     1     KDISSMMREG	SMM Region KEN Disable
**        <9>     1     SMISEL		SMI Select
**        <10>    1     SMMBLASTL	Force BLAST# Low During SMM Cycles Enable
**        <11>    1     SMMMSKA20	SMM Mask A20
**        <13>    1     SMMMAPD		SMM Memory Map Using D Segment
**        <14>    1     SMMMAPE		SMM Memory Map Using E Segment
**        <15>    1     SMISTRTSEL	SMI Start Select
**
**	  <12> Reserved
*/
#define SR_SYSMCR1_REG		0x103
#define SR_SYSMCR1_INIT		SYSMCR1_M_SMISEL
#define SYSMCR1_V_LDSMIHLDER	0
#define SYSMCR1_M_LDSMIHLDER	1 << SYSMCR1_V_LDSMIHLDER
#define SYSMCR1_V_MOVRLYEN	1
#define SYSMCR1_M_MOVRLYEN	1 << SYSMCR1_V_MOVRLYEN
#define SYSMCR1_V_SMMMAPSEL	2
#define SYSMCR1_M_SMMMAPSEL	1 << SYSMCR1_V_SMMMAPSEL
#define SYSMCR1_V_SMMDETECT	3
#define SYSMCR1_M_SMMDETECT	1 << SYSMCR1_V_SMMDETECT
#define SYSMCR1_V_FLUSHWSMI	4
#define SYSMCR1_M_FLUSHWSMI	1 << SYSMCR1_V_FLUSHWSMI
#define SYSMCR1_V_SMIFLUSHIN	5
#define SYSMCR1_M_SMIFLUSHIN	1 << SYSMCR1_V_SMIFLUSHIN
#define SYSMCR1_V_SMIFLUSHOUT	6
#define SYSMCR1_M_SMIFLUSHOUT	1 << SYSMCR1_V_SMIFLUSHOUT
#define SYSMCR1_V_SMMKENDIS	7
#define SYSMCR1_M_SMMKENDIS	1 << SYSMCR1_V_SMMKENDIS
#define SYSMCR1_V_KDISSMMREG	8
#define SYSMCR1_M_KDISSMMREG	1 << SYSMCR1_V_KDISSMMREG
#define SYSMCR1_V_SMISEL	9
#define SYSMCR1_M_SMISEL	1 << SYSMCR1_V_SMISEL
#define SYSMCR1_V_SMMBLASTL	10
#define SYSMCR1_M_SMMBLASTL	1 << SYSMCR1_V_SMMBLASTL
#define SYSMCR1_V_SMMMSKA20	11
#define SYSMCR1_M_SMMMSKA20	1 << SYSMCR1_V_SMMMSKA20
#define SYSMCR1_V_SMMMAPD	13
#define SYSMCR1_M_SMMMAPD	1 << SYSMCR1_V_SMMMAPD
#define SYSMCR1_V_SMMMAPE	14
#define SYSMCR1_M_SMMMAPE	1 << SYSMCR1_V_SMMMAPE
#define SYSMCR1_V_SMISTRTSEL	15
#define SYSMCR1_M_SMISTRTSEL	1 << SYSMCR1_V_SMISTRTSEL

/* 
**
**  SYS Miscellaneous Control Register 2 (SYSMCR2) - Index 0x104
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SMIHLDOFRST	Enable Hold-Off of SRESET During SMI Cycles
**        <1>     1     LCLKDIS		KEN Disable Always
**        <2>     1     KFLUSH		Cache Flush
**        <4>     1     LSTIOLTCHEN	Last I/O Latch Enable
**        <6>     1     VLMSTLDOFF	Lead-off Wait-State for VL-Master Cycle
**        <13>    1     ENRDYN		Generate RDY# at the End of a Burst Cycle
**        <14>    1     LMABLOCK	Local Memory Access at A0000-BFFFFH Lock
**        <15>    1     SHDWRLOCK	Shadow RAM Write Lock
**
**	  <3><5><7:12> Reserved
*/
#define SR_SYSMCR2_REG		0x104
#define SR_SYSMCR2_INIT		( SYSMCR2_M_SMIHLDOFRST | SYSMCR2_M_VLMSTLDOFF \
				  SYSMCR2_M_ENRDYN      )
#define SYSMCR2_V_SMIHLDOFRST	0
#define SYSMCR2_M_SMIHLDOFRST	1 << SYSMCR2_V_SMIHLDOFRST
#define SYSMCR2_V_LCLKDIS	1
#define SYSMCR2_M_LCLKDIS	1 << SYSMCR2_V_LCLKDIS
#define SYSMCR2_V_KFLUSH	2
#define SYSMCR2_M_KFLUSH	1 << SYSMCR2_V_KFLUSH
#define SYSMCR2_V_LSTIOLTCHEN	4
#define SYSMCR2_M_LSTIOLTCHEN	1 << SYSMCR2_V_LSTIOLTCHEN
#define SYSMCR2_V_VLMSTLDOFF	6
#define SYSMCR2_M_VLMSTLDOFF	1 << SYSMCR2_V_VLMSTLDOFF
#define SYSMCR2_V_ENRDYN	13
#define SYSMCR2_M_ENRDYN	1 << SYSMCR2_V_ENRDYN
#define SYSMCR2_V_LMABLOCK	14
#define SYSMCR2_M_LMABLOCK	1 << SYSMCR2_V_LMABLOCK
#define SYSMCR2_V_SHDWRLOCK	15
#define SYSMCR2_M_SHDWRLOCK	1 << SYSMCR2_V_SHDWRLOCK

/* 
**
**  Parity Address Register 1 (PARADR1) - Index 0x105
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <2>    14     PARADR		Parity Error Address
**
**	  <0:1> Reserved
*/
#define SR_PARADR1_REG		0x105
#define PARADR1_V_PARADR	2
#define PARADR1_M_PARADR	0xFFFC << PARADR1_V_PARADR

/* 
**
**  Parity Address Register 2 (PARADR2) - Index 0x106
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>    12     PARADR		Parity Error Address
**        <12>    1     PARBE0		Byte Enables for Parity Error Address 0
**        <13>    1     PARBE1		Byte Enables for Parity Error Address 1
**        <14>    1     PARBE2		Byte Enables for Parity Error Address 2
**        <15>    1     PARBE3		Byte Enables for Parity Error Address 3
**
*/
#define SR_PARADR2_REG		0x106
#define PARADR2_V_PARADR	0
#define PARADR2_M_PARADR	0xFFF << PARADR2_V_PARADR
#define PARADR2_V_PARBE		12
#define PARADR2_M_PARBE		0xF << PARADR2_V_PARBE
#define PARADR2_V_PARBE0	12
#define PARADR2_M_PARBE0	1 << PARADR2_V_PARBE0
#define PARADR2_V_PARBE1	13
#define PARADR2_M_PARBE1	1 << PARADR2_V_PARBE1
#define PARADR2_V_PARBE2	14
#define PARADR2_M_PARBE2	1 << PARADR2_V_PARBE2
#define PARADR2_V_PARBE3	15
#define PARADR2_M_PARBE3	1 << PARADR2_V_PARBE3

/*
** SEQUOIA-1 Pin Function Select Registers
**
*/

/* 
**
**  SEQUOIA-1 Pin Select Register 1 (SEQPSR1) - Index 0x110
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     NPWRGDDBDIS	Negative PWRGD Edge Debounce Disable
**        <1>     1     PORT92DIS	Port 92 Disable
**        <2>     1     TAGCSSEL	TAGCS# Select
**        <3>     1     LOCKPINSEL	LOCK# Pin Select
**        <4>     1     CCSPINEN	CCS Pin Enable #
**        <5>     1     TAGDEN		TAG RAM Data Enable #
**	  <8>	  1     GPIOC0PINEN	GPIOC0 Pin Enable
**	  <10>	  1     HTRGOUTEN	Heat Regulator Output Enable
**	  <11>	  1     GPIOA0PINEN	GPIOA0 Pin Enable
**
**	  <6:7><9><12:15> Reserved
*/
#define SEQR_SEQPSR1_REG	0x110
#define SEQR_SEQPSR1_INIT	( SEQPSR1_M_PORT92DIS   | SEQPSR1_M_CCSPINEN | \
				  SEQPSR1_M_TAGDEN)
#define SEQPSR1_V_NPWRGDDBDIS	0
#define SEQPSR1_M_NPWRGDDBDIS	1 << SEQPSR1_V_NPWRGDDBDIS
#define SEQPSR1_V_PORT92DIS	1
#define SEQPSR1_M_PORT92DIS	1 << SEQPSR1_V_PORT92DIS
#define SEQPSR1_V_TAGCSSEL	2
#define SEQPSR1_M_TAGCSSEL	1 << SEQPSR1_V_TAGCSSEL
#define SEQPSR1_V_LOCKPINSEL	3
#define SEQPSR1_M_LOCKPINSEL	1 << SEQPSR1_V_LOCKPINSEL
#define SEQPSR1_V_CCSPINEN	4
#define SEQPSR1_M_CCSPINEN	1 << SEQPSR1_V_CCSPINEN
#define SEQPSR1_V_TAGDEN	5
#define SEQPSR1_M_TAGDEN	1 << SEQPSR1_V_TAGDEN
#define SEQPSR1_V_GPIOC0PINEN	8
#define SEQPSR1_M_GPIOC0PINEN	1 << SEQPSR1_V_GPIOC0PINEN
#define SEQPSR1_V_HTRGOUTEN	10
#define SEQPSR1_M_HTRGOUTEN	1 << SEQPSR1_V_HTRGOUTEN
#define SEQPSR1_V_GPIOA0PINEN	11
#define SEQPSR1_M_GPIOA0PINEN	1 << SEQPSR1_V_GPIOA0PINEN

/* 
**
**  SEQUOIA-1 Pin Select Register 2 (SEQPSR2) - Index 0x111
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     OPGPEXTEN	Optional GPEXT Enable
**        <2>     1     EXTACT3PINEN	EXTACT3 Pin Enable
**        <3>     1     NMIPINEN	NMI Pin Enable
**        <4>     1     DIRTYPINEN	DIRTY Pin Enable #
**        <5>     1     GPIOB0PINEN	GPIOB0 Pin Enable
**        <6>     1     DRTWEPINEN	DRTWE# Pin Enable #
**	  <7>	  1     GPIOB1PINEN	GPIOB1 Pin Enable
**	  <8>	  1     KBRSTPINEN	KB reset Pin Enable
**	  <11>	  1     GPIOC1PINEN	GPIOC1 Pin Enable
**	  <12>	  1     GPIO0PINEN	GPIO0 Pin Enable #
**	  <13>	  1     GPIO1PINEN	GPIO1 Pin Enable #
**	  <14>	  1     GPIO2PINEN	GPIO2 Pin Enable #
**	  <15>	  1     GPIO3PINEN	GPIO3 Pin Enable #
**
**	  <1><9:10> Reserved
*/
#define SEQR_SEQPSR2_REG	0x111
#define SEQR_SEQPSR2_INIT	( SEQPSR2_M_NMIPINEN    | SEQPSR2_M_DIRTYPINEN |\
				  SEQPSR2_M_GPIOB0PINEN | SEQPSR2_M_DRTWEPINEN |\
				  SEQPSR2_M_GPIOB1PINEN | SEQPSR2_M_KBRSTPINEN |\
				  SEQPSR2_M_BIT9        | SEQPSR2_M_BIT10      |\
				  SEQPSR2_M_GPIOC1PINEN )
#define SEQPSR2_V_OPGPEXTEN	0
#define SEQPSR2_M_OPGPEXTEN	1 << SEQPSR2_V_OPGPEXTEN
#define SEQPSR2_V_EXTACT3PINEN	2
#define SEQPSR2_M_EXTACT3PINEN	1 << SEQPSR2_V_EXTACT3PINEN
#define SEQPSR2_V_NMIPINEN	3
#define SEQPSR2_M_NMIPINEN	1 << SEQPSR2_V_NMIPINEN
#define SEQPSR2_V_DIRTYPINEN	4
#define SEQPSR2_M_DIRTYPINEN	1 << SEQPSR2_V_DIRTYPINEN
#define SEQPSR2_V_GPIOB0PINEN	5
#define SEQPSR2_M_GPIOB0PINEN	1 << SEQPSR2_V_GPIOB0PINEN
#define SEQPSR2_V_DRTWEPINEN	6
#define SEQPSR2_M_DRTWEPINEN	1 << SEQPSR2_V_DRTWEPINEN
#define SEQPSR2_V_GPIOB1PINEN	7
#define SEQPSR2_M_GPIOB1PINEN	1 << SEQPSR2_V_GPIOB1PINEN
#define SEQPSR2_V_KBRSTPINEN	8
#define SEQPSR2_M_KBRSTPINEN	1 << SEQPSR2_V_KBRSTPINEN
#define SEQPSR2_V_BIT9		9
#define SEQPSR2_M_BIT9		1 << SEQPSR2_V_BIT9
#define SEQPSR2_V_BIT10		10
#define SEQPSR2_M_BIT10		1 << SEQPSR2_V_BIT10
#define SEQPSR2_V_GPIOC1PINEN	11
#define SEQPSR2_M_GPIOC1PINEN	1 << SEQPSR2_V_GPIOC1PINEN
#define SEQPSR2_V_GPIO0PINEN	12
#define SEQPSR2_M_GPIO0PINEN	1 << SEQPSR2_V_GPIO0PINEN
#define SEQPSR2_V_GPIO1PINEN	13
#define SEQPSR2_M_GPIO1PINEN	1 << SEQPSR2_V_GPIO1PINEN
#define SEQPSR2_V_GPIO2PINEN	14
#define SEQPSR2_M_GPIO2PINEN	1 << SEQPSR2_V_GPIO2PINEN
#define SEQPSR2_V_GPIO3PINEN	15
#define SEQPSR2_M_GPIO3PINEN	1 << SEQPSR2_V_GPIO3PINEN

/* 
**
**  SEQUOIA-1 Pin Select Register 3 (SEQPSR3) - Index 0x112
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PC0PINEN	PC0 Pin Enable #
**        <2>     1     PC2PINEN	PC2 Pin Enable #
**        <3>     1     PC3PINEN	PC3 Pin Enable #
**        <4>     1     PC4PINEN	PC4 Pin Enable #
**        <5>     1     PC5PINEN	PC5 Pin Enable #
**        <6>     1     PC6PINEN	PC6 Pin Enable #
**        <7>     1     PC7PINEN	PC7 Pin Enable #
**        <8>     1     PC8PINEN	PC8 Pin Enable #
**        <9>     1     PC9PINEN	PC9 Pin Enable #
**        <12>    1     EXTACT0PINEN	EXTACT0 Pin Enable
**        <15>    1     EXTACT1PINEN	EXTACT1 Pin Enable
**
**	  <1><10:11><13:14> Reserved
*/
#define SEQR_SEQPSR3_REG	0x112
#define SEQR_SEQPSR3_INIT	( SEQPSR3_M_PC2PINEN | SEQPSR3_M_PC3PINEN | \
				  SEQPSR3_M_PC4PINEN | SEQPSR3_M_PC5PINEN | \
				  SEQPSR3_M_PC6PINEN | SEQPSR3_M_PC7PINEN | \
				  SEQPSR3_M_PC8PINEN | SEQPSR3_M_PC9PINEN )
#define SEQPSR3_V_PC0PINEN	0
#define SEQPSR3_M_PC0PINEN	1 << SEQPSR3_V_PC0PINEN
#define SEQPSR3_V_PC2PINEN	2
#define SEQPSR3_M_PC2PINEN	1 << SEQPSR3_V_PC2PINEN
#define SEQPSR3_V_PC3PINEN	3
#define SEQPSR3_M_PC3PINEN	1 << SEQPSR3_V_PC3PINEN
#define SEQPSR3_V_PC4PINEN	4
#define SEQPSR3_M_PC4PINEN	1 << SEQPSR3_V_PC4PINEN
#define SEQPSR3_V_PC5PINEN	5
#define SEQPSR3_M_PC5PINEN	1 << SEQPSR3_V_PC5PINEN
#define SEQPSR3_V_PC6PINEN	6
#define SEQPSR3_M_PC6PINEN	1 << SEQPSR3_V_PC6PINEN
#define SEQPSR3_V_PC7PINEN	7
#define SEQPSR3_M_PC7PINEN	1 << SEQPSR3_V_PC7PINEN
#define SEQPSR3_V_PC8PINEN	8
#define SEQPSR3_M_PC8PINEN	1 << SEQPSR3_V_PC8PINEN
#define SEQPSR3_V_PC9PINEN	9
#define SEQPSR3_M_PC9PINEN	1 << SEQPSR3_V_PC9PINEN
#define SEQPSR3_V_EXTACT0PINEN	12
#define SEQPSR3_M_EXTACT0PINEN	1 << SEQPSR3_V_EXTACT0PINEN
#define SEQPSR3_V_EXTACT1PINEN	15
#define SEQPSR3_M_EXTACT1PINEN	1 << SEQPSR3_V_EXTACT1PINEN

/* 
**
**  Modular Clock Control Register (MODCLKCR) - Index 0x118
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <1>     1     VLCLKSTPEN	VL Clock Stop Enable
**        <5>     1     ADSMDIS		ADS Mask Disable
**        <8>     1     MDCHGIMD	Mode Change Immediate
**        <11>    1     SLWATCKEN	SLOW AT Clock Enable
**
**	  <0><2:4><6:7><9:10><12:15> Reserved
*/
#define SEQR_MODCLKCR_REG	0x118
#define SEQR_MODCLKCR_INIT	MODCLKCR_M_VLCLKSTPEN
#define MODCLKCR_V_VLCLKSTPEN	1
#define MODCLKCR_M_VLCLKSTPEN	1 << MODCLKCR_V_VLCLKSTPEN
#define MODCLKCR_V_ADSMDIS	5
#define MODCLKCR_M_ADSMDIS	1 << MODCLKCR_V_ADSMDIS
#define MODCLKCR_V_MDCHGIMD	8
#define MODCLKCR_M_MDCHGIMD	1 << MODCLKCR_V_MDCHGIMD
#define MODCLKCR_V_SLWATCKEN	11
#define MODCLKCR_M_SLWWATCKEN	1 << MODCLKCR_V_SLWATCKEN

/* 
**
**  Burst Bus Control Register (BBUSCR) - Index 0x180
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <3>     1     SQ1GIDEEN	SEQUOIA-1 Global Turbo IDE Enable
**        <5>     1     RSTREADSEN	Restart EADS Enable
**        <15>    1     SECIDEN		Secondary IDE Drive Enable
**
**	  <0:2><4><6:14> Reserved
*/
#define SEQR_BBUSCR_REG	0x180
#define BBUSCR_V_SQ1GIDEEN	3
#define BBUSCR_M_SQ1GIDEEN	1 << BBUSCR_V_SQ1GIDEEN
#define BBUSCR_V_RSTREADSEN	5
#define BBUSCR_M_RSTREADSEN	1 << BBUSCR_V_RSTREADSEN
#define BBUSCR_V_SECIDEN	15
#define BBUSCR_M_SECIDEN	1 << BBUSCR_V_SECIDEN

/*
** DRAM Control Registers
**
*/

/* 
**
**  Shadow RAM Read Enable Control Register (SRAMRDENCR) - Index 0x200
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     LMEMRDEN0	Local Memory C0000H-C3FFFH Read Enable
**        <1>     1     LMEMRDEN1	Local Memory C4000H-C7FFFH Read Enable
**        <2>     1     LMEMRDEN2	Local Memory C8000H-CBFFFH Read Enable
**        <3>     1     LMEMRDEN3	Local Memory CC000H-CFFFFH Read Enable
**        <4>     1     LMEMRDEN4	Local Memory D0000H-D3FFFH Read Enable
**        <5>     1     LMEMRDEN5	Local Memory D4000H-D7FFFH Read Enable
**        <6>     1     LMEMRDEN6	Local Memory D8000H-DBFFFH Read Enable
**        <7>     1     LMEMRDEN7	Local Memory DC000H-DFFFFH Read Enable
**        <8>     1     LMEMRDEN8	Local Memory E0000H-E3FFFH Read Enable
**        <9>     1     LMEMRDEN9	Local Memory E4000H-E7FFFH Read Enable
**        <10>    1     LMEMRDEN10	Local Memory E8000H-EBFFFH Read Enable
**        <11>    1     LMEMRDEN11	Local Memory EC000H-EFFFFH Read Enable
**        <12>    1     LMEMRDEN12	Local Memory F0000H-FFFFFH Read Enable
**
**	  <13:15> Reserved
*/
#define DRAMCR_SRAMRDENCR_REG	0x200
#define DRAMCR_SRAMRDENCR_INIT	0x0000
#define SRAMRDENCR_V_LMEMRDEN0	0
#define SRAMRDENCR_M_LMEMRDEN0	1 << SRAMRDENCR_V_LMEMRDEN0
#define SRAMRDENCR_V_LMEMRDEN1	1
#define SRAMRDENCR_M_LMEMRDEN1	1 << SRAMRDENCR_V_LMEMRDEN1
#define SRAMRDENCR_V_LMEMRDEN2	2
#define SRAMRDENCR_M_LMEMRDEN2	1 << SRAMRDENCR_V_LMEMRDEN2
#define SRAMRDENCR_V_LMEMRDEN3	3
#define SRAMRDENCR_M_LMEMRDEN3	1 << SRAMRDENCR_V_LMEMRDEN3
#define SRAMRDENCR_V_LMEMRDEN4	4
#define SRAMRDENCR_M_LMEMRDEN4	1 << SRAMRDENCR_V_LMEMRDEN4
#define SRAMRDENCR_V_LMEMRDEN5	5
#define SRAMRDENCR_M_LMEMRDEN5	1 << SRAMRDENCR_V_LMEMRDEN5
#define SRAMRDENCR_V_LMEMRDEN6	6
#define SRAMRDENCR_M_LMEMRDEN6	1 << SRAMRDENCR_V_LMEMRDEN6
#define SRAMRDENCR_V_LMEMRDEN7	7
#define SRAMRDENCR_M_LMEMRDEN7	1 << SRAMRDENCR_V_LMEMRDEN7
#define SRAMRDENCR_V_LMEMRDEN8	8
#define SRAMRDENCR_M_LMEMRDEN8	1 << SRAMRDENCR_V_LMEMRDEN8
#define SRAMRDENCR_V_LMEMRDEN9	9                       
#define SRAMRDENCR_M_LMEMRDEN9	1 << SRAMRDENCR_V_LMEMRDEN9
#define SRAMRDENCR_V_LMEMRDEN10	10
#define SRAMRDENCR_M_LMEMRDEN10	1 << SRAMRDENCR_V_LMEMRDEN10
#define SRAMRDENCR_V_LMEMRDEN11	11
#define SRAMRDENCR_M_LMEMRDEN11	1 << SRAMRDENCR_V_LMEMRDEN11
#define SRAMRDENCR_V_LMEMRDEN12	12
#define SRAMRDENCR_M_LMEMRDEN12	1 << SRAMRDENCR_V_LMEMRDEN12

/* 
**
**  Shadow RAM Write Enable Control Register (SRAMWRENCR) - Index 0x201
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     LMEMWREN0	Local Memory C0000H-C3FFFH Write Enable
**        <1>     1     LMEMWREN1	Local Memory C4000H-C7FFFH Write Enable
**        <2>     1     LMEMWREN2	Local Memory C8000H-CBFFFH Write Enable
**        <3>     1     LMEMWREN3	Local Memory CC000H-CFFFFH Write Enable
**        <4>     1     LMEMWREN4	Local Memory D0000H-D3FFFH Write Enable
**        <5>     1     LMEMWREN5	Local Memory D4000H-D7FFFH Write Enable
**        <6>     1     LMEMWREN6	Local Memory D8000H-DBFFFH Write Enable
**        <7>     1     LMEMWREN7	Local Memory DC000H-DFFFFH Write Enable
**        <8>     1     LMEMWREN8	Local Memory E0000H-E3FFFH Write Enable
**        <9>     1     LMEMWREN9	Local Memory E4000H-E7FFFH Write Enable
**        <10>    1     LMEMWREN10	Local Memory E8000H-EBFFFH Write Enable
**        <11>    1     LMEMWREN11	Local Memory EC000H-EFFFFH Write Enable
**        <12>    1     LMEMWREN12	Local Memory F0000H-FFFFFH Write Enable
**
**	  <13:15> Reserved
*/
#define DRAMCR_SRAMWRENCR_REG	0x201
#define DRAMCR_SRAMWRENCR_INIT	0x0000
#define SRAMWRENCR_V_LMEMWREN0	0
#define SRAMWRENCR_M_LMEMWREN0	1 << SRAMWRENCR_V_LMEMWREN0
#define SRAMWRENCR_V_LMEMWREN1	1
#define SRAMWRENCR_M_LMEMWREN1	1 << SRAMWRENCR_V_LMEMWREN1
#define SRAMWRENCR_V_LMEMWREN2	2
#define SRAMWRENCR_M_LMEMWREN2	1 << SRAMWRENCR_V_LMEMWREN2
#define SRAMWRENCR_V_LMEMWREN3	3
#define SRAMWRENCR_M_LMEMWREN3	1 << SRAMWRENCR_V_LMEMWREN3
#define SRAMWRENCR_V_LMEMWREN4	4
#define SRAMWRENCR_M_LMEMWREN4	1 << SRAMWRENCR_V_LMEMWREN4
#define SRAMWRENCR_V_LMEMWREN5	5
#define SRAMWRENCR_M_LMEMWREN5	1 << SRAMWRENCR_V_LMEMWREN5
#define SRAMWRENCR_V_LMEMWREN6	6
#define SRAMWRENCR_M_LMEMWREN6	1 << SRAMWRENCR_V_LMEMWREN6
#define SRAMWRENCR_V_LMEMWREN7	7
#define SRAMWRENCR_M_LMEMWREN7	1 << SRAMWRENCR_V_LMEMWREN7
#define SRAMWRENCR_V_LMEMWREN8	8
#define SRAMWRENCR_M_LMEMWREN8	1 << SRAMWRENCR_V_LMEMWREN8
#define SRAMWRENCR_V_LMEMWREN9	9                       
#define SRAMWRENCR_M_LMEMWREN9	1 << SRAMWRENCR_V_LMEMWREN9
#define SRAMWRENCR_V_LMEMWREN10	10
#define SRAMWRENCR_M_LMEMWREN10	1 << SRAMWRENCR_V_LMEMWREN10
#define SRAMWRENCR_V_LMEMWREN11	11
#define SRAMWRENCR_M_LMEMWREN11	1 << SRAMWRENCR_V_LMEMWREN11
#define SRAMWRENCR_V_LMEMWREN12	12
#define SRAMWRENCR_M_LMEMWREN12	1 << SRAMWRENCR_V_LMEMWREN12

/* 
**
**  Bank 0 Control Register (BK0CR) - Index 0x202
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     8     B0A		Bank 0 Starting Address
**        <8>     1     B0S0		Bank 0 DRAM Size 0
**        <9>     1     B0S1		Bank 0 DRAM Size 1
**        <10>    1     B0S2		Bank 0 DRAM Size 2
**        <11>    1     BANKEN0		Bank 0 Enable
**        <12>    1     ENMDEN0N	Enable MDEN# for Bank 0
**        <13>    1     COLADR00	Number of Column Address Bits for Bank 0 0
**        <14>    1     COLADR01	Number of Column Address Bits for Bank 0 1
**        <15>    1     COLADR02	Number of Column Address Bits for Bank 0 2
**
*/
#define DRAMCR_BK0CR_REG	0x202
#define DRAMCR_BK0CR_INIT	0x0000
#define BK0CR_V_B0A		0
#define BK0CR_M_B0A		0xFF << BK0CR_V_B0A
#define BK0CR_V_B0S		8
#define BK0CR_M_B0S		0x7 << BK0CR_V_B0S
#define BK0CR_V_B0S0		8
#define BK0CR_M_B0S0		1 << BK0CR_V_B0S0
#define BK0CR_V_B0S1		9
#define BK0CR_M_B0S1		1 << BK0CR_V_B0S1
#define BK0CR_V_B0S2		10
#define BK0CR_M_B0S2		1 << BK0CR_V_B0S2
#define BK0CR_V_BANKEN0		11
#define BK0CR_M_BANKEN0		1 << BK0CR_V_BANKEN0
#define BK0CR_V_ENMDEN0N	12
#define BK0CR_M_ENMDEN0N	1 << BK0CR_V_ENMDEN0N
#define BK0CR_V_COLADR0		13
#define BK0CR_M_COLADR0		0xE << BK0CR_V_COLADR0
#define BK0CR_V_COLADR00	13
#define BK0CR_M_COLADR00	1 << BK0CR_V_COLADR00
#define BK0CR_V_COLADR01	14
#define BK0CR_M_COLADR01	1 << BK0CR_V_COLADR01
#define BK0CR_V_COLADR02	15
#define BK0CR_M_COLADR02	1 << BK0CR_V_COLADR02

/*
** Bank 0 DRAM Size
*/
#define B0S_256KB		0x000
#define B0S_512KB		BK0CR_M_B0S0
#define B0S_1MB			BK0CR_M_B0S1
#define B0S_2MB			(BK0CR_M_B0S0 | BK0CR_M_B0S1)
#define B0S_4MB			BK0CR_M_B0S2
#define B0S_16MB		(BK0CR_M_B0S1 | BK0CR_M_B0S2)

/*
** Bank 0 Column Address Bits 
*/
#define COLADR0_8		0x000
#define COLADR0_9		BK0CR_M_COLADR00
#define COLADR0_10		BK0CR_M_COLADR01
#define COLADR0_11		(BK0CR_M_COLADR00 | BK0CR_M_COLADR01)
#define COLADR0_12		BK0CR_M_COLADR02

/* 
**
**  Bank 1 Control Register (BK1CR) - Index 0x203
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     8     B1A		Bank 1 Starting Address
**        <8>     1     B1S0		Bank 1 DRAM Size 0
**        <9>     1     B1S1		Bank 1 DRAM Size 1
**        <10>    1     B1S2		Bank 1 DRAM Size 2
**        <11>    1     BANKEN1		Bank 1 Enable
**        <12>    1     ENMDEN1N	Enable MDEN# for Bank 1
**        <13>    1     COLADR10	Number of Column Address Bits for Bank 1 0
**        <14>    1     COLADR11	Number of Column Address Bits for Bank 1 1
**        <15>    1     COLADR12	Number of Column Address Bits for Bank 1 2
**
*/
#define DRAMCR_BK1CR_REG	0x203
#define DRAMCR_BK1CR_INIT	0x0000
#define BK1CR_V_B1A		0
#define BK1CR_M_B1A		0xFF << BK1CR_V_B1A
#define BK1CR_V_B1S		8
#define BK1CR_M_B1S		0x7 << BK1CR_V_B1S
#define BK1CR_V_B1S0		8
#define BK1CR_M_B1S0		1 << BK1CR_V_B1S0
#define BK1CR_V_B1S1		9
#define BK1CR_M_B1S1		1 << BK1CR_V_B1S1
#define BK1CR_V_B1S2		10
#define BK1CR_M_B1S2		1 << BK1CR_V_B1S2
#define BK1CR_V_BANKEN1		11
#define BK1CR_M_BANKEN1		1 << BK1CR_V_BANKEN1
#define BK1CR_V_ENMDEN1N	12
#define BK1CR_M_ENMDEN1N	1 << BK1CR_V_ENMDEN1N
#define BK1CR_V_COLADR1		13
#define BK1CR_M_COLADR1		0xE << BK1CR_V_COLADR1
#define BK1CR_V_COLADR10	13
#define BK1CR_M_COLADR10	1 << BK1CR_V_COLADR10
#define BK1CR_V_COLADR11	14
#define BK1CR_M_COLADR11	1 << BK1CR_V_COLADR11
#define BK1CR_V_COLADR12	15
#define BK1CR_M_COLADR12	1 << BK1CR_V_COLADR12

/*
** Bank 1 DRAM Size
*/
#define B1S_256KB		0x000
#define B1S_512KB		BK1CR_M_B1S0
#define B1S_1MB			BK1CR_M_B1S1
#define B1S_2MB			(BK1CR_M_B1S0 | BK1CR_M_B1S1)
#define B1S_4MB			BK1CR_M_B1S2
#define B1S_16MB		(BK1CR_M_B1S1 | BK1CR_M_B1S2)

/*
** Bank 1 Column Address Bits 
*/
#define COLADR1_8		0x000
#define COLADR1_9		BK1CR_M_COLADR10
#define COLADR1_10		BK1CR_M_COLADR11
#define COLADR1_11		(BK1CR_M_COLADR10 | BK1CR_M_COLADR11)
#define COLADR1_12		BK1CR_M_COLADR12

/* 
**
**  Bank 0/1 Timing Control Register (BK01TCR) - Index 0x204
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     B01WRCPW0	Bank 0/1 Write CAS Cycle Time 0
**        <1>     1     B01WRCPW1	Bank 0/1 Write CAS Cycle Time 1
**        <2>     1     MA01WRDLY	Bank 0/1 Write MA Delay
**        <3>     1     B01RDCPW0	Bank 0/1 Read CAS Cycle Time 0
**        <4>     1     B01RDCPW1	Bank 0/1 Read CAS Cycle Time 1
**        <5>     1     MA01RDDLY	Bank 0/1 Read MA Delay
**        <6>     1     B01CPRE		Bank 0/1 CAS Precharge
**        <7>     1     B01RTMA		Bank 0/1 RAS to CAS Delay
**        <8>     1     B01MATC		Bank 0/1 MA to CAS Delay
**        <9>     1     B01RPRE0	Bank 0/1 RAS Precharge Time 0
**        <10>    1     B01RPRE1	Bank 0/1 RAS Precharge Time 1
**        <11>    1     B01RPRE2	Bank 0/1 RAS Precharge Time 2
**        <12>    1     B01ILAVDT	Bank 0/1 Interleave Avoid Time
**        <13>    1     BNK01_ITL	Bank 0/1 Interleave Enable
**
**	  <14:15> Reserved
*/
#define DRAMCR_BK01TCR_REG	0x204
#define DRAMCR_BK01TCR_INIT	( BK01TCR_M_B01WRCPW  | BK01TCR_M_MA01WRDLY | \
				  BK01TCR_M_B01RDCPW  | BK01TCR_M_MA01RDDLY | \
				  BK01TCR_M_B01CPRE   | BK01TCR_M_B01RTMA   | \
				  BK01TCR_M_B01MATC   | BK01TCR_M_B01RPRE   | \
				  BK01TCR_M_B01ILAVDT )
#define BK01TCR_V_B01WRCPW	0
#define BK01TCR_M_B01WRCPW	0x3 << BK01TCR_V_B01WRCPW
#define BK01TCR_V_B01WRCPW0	0
#define BK01TCR_M_B01WRCPW0	1 << BK01TCR_V_B01WRCPW0
#define BK01TCR_V_B01WRCPW1	1
#define BK01TCR_M_B01WRCPW1	1 << BK01TCR_V_B01WRCPW1
#define BK01TCR_V_MA01WRDLY	2
#define BK01TCR_M_MA01WRDLY	1 << BK01TCR_V_MA01WRDLY
#define BK01TCR_V_B01RDCPW	3
#define BK01TCR_M_B01RDCPW	0x3 << BK01TCR_V_B01RDCPW
#define BK01TCR_V_B01RDCPW0	3
#define BK01TCR_M_B01RDCPW0	1 << BK01TCR_V_B01RDCPW0
#define BK01TCR_V_B01RDCPW1	4
#define BK01TCR_M_B01RDCPW1	1 << BK01TCR_V_B01RDCPW1
#define BK01TCR_V_MA01RDDLY	5
#define BK01TCR_M_MA01RDDLY	1 << BK01TCR_V_MA01RDDLY
#define BK01TCR_V_B01CPRE	6
#define BK01TCR_M_B01CPRE	1 << BK01TCR_V_B01CPRE
#define BK01TCR_V_B01RTMA	7
#define BK01TCR_M_B01RTMA	1 << BK01TCR_V_B01RTMA
#define BK01TCR_V_B01MATC	8
#define BK01TCR_M_B01MATC	1 << BK01TCR_V_B01MATC
#define BK01TCR_V_B01RPRE	9
#define BK01TCR_M_B01RPRE	0x7 << BK01TCR_V_B01RPRE
#define BK01TCR_V_B01RPRE0	9
#define BK01TCR_M_B01RPRE0	1 << BK01TCR_V_B01RPRE0
#define BK01TCR_V_B01RPRE1	10
#define BK01TCR_M_B01RPRE1	1 << BK01TCR_V_B01RPRE1
#define BK01TCR_V_B01RPRE2	11
#define BK01TCR_M_B01RPRE2	1 << BK01TCR_V_B01RPRE2
#define BK01TCR_V_B01ILAVDT	12
#define BK01TCR_M_B01ILAVDT	1 << BK01TCR_V_B01ILAVDT
#define BK01TCR_V_BNK01_ITL	13
#define BK01TCR_M_BNK01_ITL	1 << BK01TCR_V_BNK01_ITL

/*
** Bank 0/1 Write CAS Cycle Time 
*/
#define B01WRCPW_1T		BK01TCR_M_B01WRCPW0
#define B01WRCPW_2T		BK01TCR_M_B01WRCPW1
#define B01WRCPW_3T		(BK01TCR_M_B01WRCPW0 | BK01TCR_M_B01WRCPW1)

/*
** Bank 0/1 Read CAS Cycle Time 
*/
#define B01RDCPW_1T		0x000
#define B01RDCPW_2T		BK01TCR_M_B01WRCPW0
#define B01RDCPW_3T		BK01TCR_M_B01WRCPW1
#define B01RDCPW_4T		(BK01TCR_M_B01WRCPW0 | BK01TCR_M_B01WRCPW1)

/*
** Bank 0/1 RAS Precharge Time 
*/
#define B01RPRE_1PT5T		BK01TCR_M_B01RPRE1
#define B01RPRE_2T		(BK01TCR_M_B01RPRE0 | BK01TCR_M_B01RPRE1)
#define B01RPRE_2PT5T		BK01TCR_M_B01RPRE2
#define B01RPRE_3T		(BK01TCR_M_B01RPRE0 | BK01TCR_M_B01RPRE2)
#define B01RPRE_3PT5T		(BK01TCR_M_B01RPRE1 | BK01TCR_M_B01RPRE2)
#define B01RPRE_4T		(BK01TCR_M_B01RPRE0 | BK01TCR_M_B01RPRE1 | BK01TCR_M_B01RPRE2)

/* 
**
**  Bank 2 Control Register (BK2CR) - Index 0x205
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     8     B2A		Bank 2 Starting Address
**        <8>     1     B2S0		Bank 2 DRAM Size 0
**        <9>     1     B2S1		Bank 2 DRAM Size 1
**        <10>    1     B2S2		Bank 2 DRAM Size 2
**        <11>    1     BANKEN2		Bank 2 Enable
**        <12>    1     ENMDEN2N	Enable MDEN# for Bank 2
**        <13>    1     COLADR20	Number of Column Address Bits for Bank 2 0
**        <14>    1     COLADR21	Number of Column Address Bits for Bank 2 1
**        <15>    1     COLADR22	Number of Column Address Bits for Bank 2 2
**
*/
#define DRAMCR_BK2CR_REG	0x205
#define DRAMCR_BK2CR_INIT	0x0000
#define BK2CR_V_B2A		0
#define BK2CR_M_B2A		0xFF << BK2CR_V_B2A
#define BK2CR_V_B2S		8
#define BK2CR_M_B2S		0x7 << BK2CR_V_B2S
#define BK2CR_V_B2S0		8
#define BK2CR_M_B2S0		1 << BK2CR_V_B2S0
#define BK2CR_V_B2S1		9
#define BK2CR_M_B2S1		1 << BK2CR_V_B2S1
#define BK2CR_V_B2S2		10
#define BK2CR_M_B2S2		1 << BK2CR_V_B2S2
#define BK2CR_V_BANKEN2		11
#define BK2CR_M_BANKEN2		1 << BK2CR_V_BANKEN2
#define BK2CR_V_ENMDEN2N	12
#define BK2CR_M_ENMDEN2N	1 << BK2CR_V_ENMDEN2N
#define BK2CR_V_COLADR2		13
#define BK2CR_M_COLADR2		0xE << BK2CR_V_COLADR2
#define BK2CR_V_COLADR20	13
#define BK2CR_M_COLADR20	1 << BK2CR_V_COLADR20
#define BK2CR_V_COLADR21	14
#define BK2CR_M_COLADR21	1 << BK2CR_V_COLADR21
#define BK2CR_V_COLADR22	15
#define BK2CR_M_COLADR22	1 << BK2CR_V_COLADR22

/*
** Bank 2 DRAM Size
*/
#define B2S_256KB		0x000
#define B2S_512KB		BK2CR_M_B2S0
#define B2S_1MB			BK2CR_M_B2S1
#define B2S_2MB			(BK2CR_M_B2S0 | BK2CR_M_B2S1)
#define B2S_4MB			BK2CR_M_B2S2
#define B2S_16MB		(BK2CR_M_B2S1 | BK2CR_M_B2S2)

/*
** Bank 2 Column Address Bits 
*/
#define COLADR2_8		0x000
#define COLADR2_9		BK2CR_M_COLADR20
#define COLADR2_10		BK2CR_M_COLADR21
#define COLADR2_11		(BK2CR_M_COLADR20 | BK2CR_M_COLADR21)
#define COLADR2_12		BK2CR_M_COLADR22

/* 
**
**  Bank 3 Control Register (BK3CR) - Index 0x206
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     8     B3A		Bank 3 Starting Address
**        <8>     1     B3S0		Bank 3 DRAM Size 0
**        <9>     1     B3S1		Bank 3 DRAM Size 1
**        <10>    1     B3S2		Bank 3 DRAM Size 2
**        <11>    1     BANKEN3		Bank 3 Enable
**        <12>    1     ENMDEN3N	Enable MDEN# for Bank 3
**        <13>    1     COLADR30	Number of Column Address Bits for Bank 3 0
**        <14>    1     COLADR31	Number of Column Address Bits for Bank 3 1
**        <15>    1     COLADR32	Number of Column Address Bits for Bank 3 2
**
*/
#define DRAMCR_BK3CR_REG	0x206
#define DRAMCR_BK3CR_INIT	0x0000
#define BK3CR_V_B3A		0
#define BK3CR_M_B3A		0xFF << BK3CR_V_B3A
#define BK3CR_V_B3S		8
#define BK3CR_M_B3S		0x7 << BK3CR_V_B3S
#define BK3CR_V_B3S0		8
#define BK3CR_M_B3S0		1 << BK3CR_V_B3S0
#define BK3CR_V_B3S1		9
#define BK3CR_M_B3S1		1 << BK3CR_V_B3S1
#define BK3CR_V_B3S2		10
#define BK3CR_M_B3S2		1 << BK3CR_V_B3S2
#define BK3CR_V_BANKEN3		11
#define BK3CR_M_BANKEN3		1 << BK3CR_V_BANKEN3
#define BK3CR_V_ENMDEN3N	12
#define BK3CR_M_ENMDEN3N	1 << BK3CR_V_ENMDEN3N
#define BK3CR_V_COLADR3		13
#define BK3CR_M_COLADR3		0xE << BK3CR_V_COLADR3
#define BK3CR_V_COLADR30	13
#define BK3CR_M_COLADR30	1 << BK3CR_V_COLADR30
#define BK3CR_V_COLADR31	14
#define BK3CR_M_COLADR31	1 << BK3CR_V_COLADR31
#define BK3CR_V_COLADR32	15
#define BK3CR_M_COLADR32	1 << BK3CR_V_COLADR32

/*
** Bank 3 DRAM Size
*/
#define B3S_256KB		0x000
#define B3S_512KB		BK3CR_M_B3S0
#define B3S_1MB			BK3CR_M_B3S1
#define B3S_2MB			(BK3CR_M_B3S0 | BK3CR_M_B3S1)
#define B3S_4MB			BK3CR_M_B3S2
#define B3S_16MB		(BK3CR_M_B3S1 | BK3CR_M_B3S2)

/*
** Bank 3 Column Address Bits 
*/
#define COLADR3_8		0x000
#define COLADR3_9		BK3CR_M_COLADR30
#define COLADR3_10		BK3CR_M_COLADR31
#define COLADR3_11		(BK3CR_M_COLADR30 | BK3CR_M_COLADR31)
#define COLADR3_12		BK3CR_M_COLADR32
           
/* 
**
**  Bank 2/3 Timing Control Register (BK23TCR) - Index 0x207
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     B23WRCPW0	Bank 2/3 Write CAS Cycle Time 0
**        <1>     1     B23WRCPW1	Bank 2/3 Write CAS Cycle Time 1
**        <2>     1     MA23WRDLY	Bank 2/3 Write MA Delay
**        <3>     1     B23RDCPW0	Bank 2/3 Read CAS Cycle Time 0
**        <4>     1     B23RDCPW1	Bank 2/3 Read CAS Cycle Time 1
**        <5>     1     MA23RDDLY	Bank 2/3 Read MA Delay
**        <6>     1     B23CPRE		Bank 2/3 CAS Precharge
**        <7>     1     B23RTMA		Bank 2/3 RAS to CAS Delay
**        <8>     1     B23MATC		Bank 2/3 MA to CAS Delay
**        <9>     1     B23RPRE0	Bank 2/3 RAS Precharge Time 0
**        <10>    1     B23RPRE1	Bank 2/3 RAS Precharge Time 1
**        <11>    1     B23RPRE2	Bank 2/3 RAS Precharge Time 2
**        <13>    1     BNK23_ITL	Bank 2/3 Interleave Enable
**
**	  <12><14:15> Reserved
*/
#define DRAMCR_BK23TCR_REG	0x207
#define DRAMCR_BK23TCR_INIT	( BK23TCR_M_B23WRCPW | BK23TCR_M_MA23WRDLY | \
				  BK23TCR_M_B23RDCPW | BK23TCR_M_MA23RDDLY | \
				  BK23TCR_M_B23CPRE  | BK23TCR_M_B23RTMA   | \
				  BK23TCR_M_B23MATC  | BK23TCR_M_B23RPRE   )
#define BK23TCR_V_B23WRCPW	0
#define BK23TCR_M_B23WRCPW	0x3 << BK23TCR_V_B23WRCPW
#define BK23TCR_V_B23WRCPW0	0
#define BK23TCR_M_B23WRCPW0	1 << BK23TCR_V_B23WRCPW0
#define BK23TCR_V_B23WRCPW1	1
#define BK23TCR_M_B23WRCPW1	1 << BK23TCR_V_B23WRCPW1
#define BK23TCR_V_MA23WRDLY	2
#define BK23TCR_M_MA23WRDLY	1 << BK23TCR_V_MA23WRDLY
#define BK23TCR_V_B23RDCPW	3
#define BK23TCR_M_B23RDCPW	0x3 << BK23TCR_V_B23RDCPW
#define BK23TCR_V_B23RDCPW0	3
#define BK23TCR_M_B23RDCPW0	1 << BK23TCR_V_B23RDCPW0
#define BK23TCR_V_B23RDCPW1	4
#define BK23TCR_M_B23RDCPW1	1 << BK23TCR_V_B23RDCPW1
#define BK23TCR_V_MA23RDDLY	5
#define BK23TCR_M_MA23RDDLY	1 << BK23TCR_V_MA23RDDLY
#define BK23TCR_V_B23CPRE	6
#define BK23TCR_M_B23CPRE	1 << BK23TCR_V_B23CPRE
#define BK23TCR_V_B23RTMA	7
#define BK23TCR_M_B23RTMA	1 << BK23TCR_V_B23RTMA
#define BK23TCR_V_B23MATC	8
#define BK23TCR_M_B23MATC	1 << BK23TCR_V_B23MATC
#define BK23TCR_V_B23RPRE	9
#define BK23TCR_M_B23RPRE	0x7 << BK23TCR_V_B23RPRE
#define BK23TCR_V_B23RPRE0	9
#define BK23TCR_M_B23RPRE0	1 << BK23TCR_V_B23RPRE0
#define BK23TCR_V_B23RPRE1	10
#define BK23TCR_M_B23RPRE1	1 << BK23TCR_V_B23RPRE1
#define BK23TCR_V_B23RPRE2	11
#define BK23TCR_M_B23RPRE2	1 << BK23TCR_V_B23RPRE2
#define BK23TCR_V_BNK23_ITL	13
#define BK23TCR_M_BNK23_ITL	1 << BK23TCR_V_BNK23_ITL

/*
** Bank 2/3 Write CAS Cycle Time 
*/
#define B23WRCPW_1T		BK23TCR_M_B23WRCPW0
#define B23WRCPW_2T		BK23TCR_M_B23WRCPW1
#define B23WRCPW_3T		(BK23TCR_M_B23WRCPW0 | BK23TCR_M_B23WRCPW1)

/*
** Bank 2/3 Read CAS Cycle Time 
*/
#define B23RDCPW_1T		0x000
#define B23RDCPW_2T		BK23TCR_M_B23RDCPW0
#define B23RDCPW_3T		BK23TCR_M_B23RDCPW1
#define B23RDCPW_4T		(BK23TCR_M_B23RDCPW0 | BK23TCR_M_B23RDCPW1)

/*
** Bank 2/3 RAS Precharge Time 
*/
#define B23RPRE_1PT5T		BK23TCR_M_B23RPRE1
#define B23RPRE_2T		(BK23TCR_M_B23RPRE0 | BK23TCR_M_B23RPRE1)
#define B23RPRE_2PT5T		BK23TCR_M_B23RPRE2
#define B23RPRE_3T		(BK23TCR_M_B23RPRE0 | BK23TCR_M_B23RPRE2)
#define B23RPRE_3PT5T		(BK23TCR_M_B23RPRE1 | BK23TCR_M_B23RPRE2)
#define B23RPRE_4T		(BK23TCR_M_B23RPRE0 | BK23TCR_M_B23RPRE1 | BK23TCR_M_B23RPRE2)

/* 
**
**  Bank 4 Control Register (BK4CR) - Index 0x208
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     8     B4A		Bank 4 Starting Address
**        <8>     1     B4S0		Bank 4 DRAM Size 0
**        <9>     1     B4S1		Bank 4 DRAM Size 1
**        <10>    1     B4S2		Bank 4 DRAM Size 2
**        <11>    1     BANKEN4		Bank 4 Enable
**        <12>    1     ENMDEN4N	Enable MDEN# for Bank 4
**        <13>    1     COLADR40	Number of Column Address Bits for Bank 4 0
**        <14>    1     COLADR41	Number of Column Address Bits for Bank 4 1
**        <15>    1     COLADR42	Number of Column Address Bits for Bank 4 2
**
*/
#define DRAMCR_BK4CR_REG	0x208
#define DRAMCR_BK4CR_INIT	0x0000
#define BK4CR_V_B4A		0
#define BK4CR_M_B4A		0xFF << BK4CR_V_B4A
#define BK4CR_V_B4S		8
#define BK4CR_M_B4S		0x7 << BK4CR_V_B4S
#define BK4CR_V_B4S0		8
#define BK4CR_M_B4S0		1 << BK4CR_V_B4S0
#define BK4CR_V_B4S1		9
#define BK4CR_M_B4S1		1 << BK4CR_V_B4S1
#define BK4CR_V_B4S2		10
#define BK4CR_M_B4S2		1 << BK4CR_V_B4S2
#define BK4CR_V_BANKEN4		11
#define BK4CR_M_BANKEN4		1 << BK4CR_V_BANKEN4
#define BK4CR_V_ENMDEN4N	12
#define BK4CR_M_ENMDEN4N	1 << BK4CR_V_ENMDEN4N
#define BK4CR_V_COLADR4		13
#define BK4CR_M_COLADR4		0xE << BK4CR_V_COLADR4
#define BK4CR_V_COLADR40	13
#define BK4CR_M_COLADR40	1 << BK4CR_V_COLADR40
#define BK4CR_V_COLADR41	14
#define BK4CR_M_COLADR41	1 << BK4CR_V_COLADR41
#define BK4CR_V_COLADR42	15
#define BK4CR_M_COLADR42	1 << BK4CR_V_COLADR42

/*
** Bank 4 DRAM Size
*/
#define B4S_256KB		0x000
#define B4S_512KB		BK4CR_M_B4S0
#define B4S_1MB			BK4CR_M_B4S1
#define B4S_2MB			(BK4CR_M_B4S0 | BK4CR_M_B4S1)
#define B4S_4MB			BK4CR_M_B4S2
#define B4S_16MB		(BK4CR_M_B4S1 | BK4CR_M_B4S2)

/*
** Bank 4 Column Address Bits 
*/
#define COLADR4_8		0x000
#define COLADR4_9		BK4CR_M_COLADR40
#define COLADR4_10		BK4CR_M_COLADR41
#define COLADR4_11		(BK4CR_M_COLADR40 | BK4CR_M_COLADR41)
#define COLADR4_12		BK4CR_M_COLADR42

/* 
**
**  Bank 5 Control Register (BK5CR) - Index 0x209
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     8     B5A		Bank 5 Starting Address
**        <8>     1     B5S0		Bank 5 DRAM Size 0
**        <9>     1     B5S1		Bank 5 DRAM Size 1
**        <10>    1     B5S2		Bank 5 DRAM Size 2
**        <11>    1     BANKEN5		Bank 5 Enable
**        <12>    1     ENMDEN5N	Enable MDEN# for Bank 5
**        <13>    1     COLADR50	Number of Column Address Bits for Bank 5 0
**        <14>    1     COLADR51	Number of Column Address Bits for Bank 5 1
**        <15>    1     COLADR52	Number of Column Address Bits for Bank 5 2
**
*/
#define DRAMCR_BK5CR_REG	0x209
#define DRAMCR_BK5CR_INIT	0x0000
#define BK5CR_V_B5A		0
#define BK5CR_M_B5A		0xFF << BK5CR_V_B5A
#define BK5CR_V_B5S		8
#define BK5CR_M_B5S		0x7 << BK5CR_V_B5S
#define BK5CR_V_B5S0		8
#define BK5CR_M_B5S0		1 << BK5CR_V_B5S0
#define BK5CR_V_B5S1		9
#define BK5CR_M_B5S1		1 << BK5CR_V_B5S1
#define BK5CR_V_B5S2		10
#define BK5CR_M_B5S2		1 << BK5CR_V_B5S2
#define BK5CR_V_BANKEN5		11
#define BK5CR_M_BANKEN5		1 << BK5CR_V_BANKEN5
#define BK5CR_V_ENMDEN5N	12
#define BK5CR_M_ENMDEN5N	1 << BK5CR_V_ENMDEN5N
#define BK5CR_V_COLADR5		13
#define BK5CR_M_COLADR5		0xE << BK5CR_V_COLADR5
#define BK5CR_V_COLADR50	13
#define BK5CR_M_COLADR50	1 << BK5CR_V_COLADR50
#define BK5CR_V_COLADR51	14
#define BK5CR_M_COLADR51	1 << BK5CR_V_COLADR51
#define BK5CR_V_COLADR52	15
#define BK5CR_M_COLADR52	1 << BK5CR_V_COLADR52

/*
** Bank 5 DRAM Size
*/
#define B5S_256KB		0x000
#define B5S_512KB		BK5CR_M_B5S0
#define B5S_1MB			BK5CR_M_B5S1
#define B5S_2MB			(BK5CR_M_B5S0 | BK5CR_M_B5S1)
#define B5S_4MB			BK5CR_M_B5S2
#define B5S_16MB		(BK5CR_M_B5S1 | BK5CR_M_B5S2)

/*
** Bank 5 Column Address Bits 
*/
#define COLADR5_8		0x000
#define COLADR5_9		BK5CR_M_COLADR50
#define COLADR5_10		BK5CR_M_COLADR51
#define COLADR5_11		(BK5CR_M_COLADR50 | BK5CR_M_COLADR51)
#define COLADR5_12		BK5CR_M_COLADR52

/* 
**
**  Bank 4/5 Timing Control Register (BK45TCR) - Index 0x20A
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     B45WRCPW0	Bank 4/5 Write CAS Cycle Time 0
**        <1>     1     B45WRCPW1	Bank 4/5 Write CAS Cycle Time 1
**        <2>     1     MA45WRDLY	Bank 4/5 Write MA Delay
**        <3>     1     B45RDCPW0	Bank 4/5 Read CAS Cycle Time 0
**        <4>     1     B45RDCPW1	Bank 4/5 Read CAS Cycle Time 1
**        <5>     1     MA45RDDLY	Bank 4/5 Read MA Delay
**        <6>     1     B45CPRE		Bank 4/5 CAS Precharge
**        <7>     1     B45RTMA		Bank 4/5 RAS to CAS Delay
**        <8>     1     B45MATC		Bank 4/5 MA to CAS Delay
**        <9>     1     B45RPRE0	Bank 4/5 RAS Precharge Time 0
**        <10>    1     B45RPRE1	Bank 4/5 RAS Precharge Time 1
**        <11>    1     B45RPRE2	Bank 4/5 RAS Precharge Time 2
**        <13>    1     BNK45_ITL	Bank 4/5 Interleave Enable
**
**	  <12><14:15> Reserved
*/
#define DRAMCR_BK45TCR_REG	0x20A
#define DRAMCR_BK45TCR_INIT	( BK45TCR_M_B45WRCPW | BK45TCR_M_MA45WRDLY | \
				  BK45TCR_M_B45RDCPW | BK45TCR_M_MA45RDDLY | \
				  BK45TCR_M_B45CPRE  | BK45TCR_M_B45RTMA   | \
				  BK45TCR_M_B45MATC  | BK45TCR_M_B45RPRE   )
#define BK45TCR_V_B45WRCPW	0
#define BK45TCR_M_B45WRCPW	0x3 << BK45TCR_V_B45WRCPW
#define BK45TCR_V_B45WRCPW0	0
#define BK45TCR_M_B45WRCPW0	1 << BK45TCR_V_B45WRCPW0
#define BK45TCR_V_B45WRCPW1	1
#define BK45TCR_M_B45WRCPW1	1 << BK45TCR_V_B45WRCPW1
#define BK45TCR_V_MA45WRDLY	2
#define BK45TCR_M_MA45WRDLY	1 << BK45TCR_V_MA45WRDLY
#define BK45TCR_V_B45RDCPW	3
#define BK45TCR_M_B45RDCPW	0x3 << BK45TCR_V_B45RDCPW
#define BK45TCR_V_B45RDCPW0	3
#define BK45TCR_M_B45RDCPW0	1 << BK45TCR_V_B45RDCPW0
#define BK45TCR_V_B45RDCPW1	4
#define BK45TCR_M_B45RDCPW1	1 << BK45TCR_V_B45RDCPW1
#define BK45TCR_V_MA45RDDLY	5
#define BK45TCR_M_MA45RDDLY	1 << BK45TCR_V_MA45RDDLY
#define BK45TCR_V_B45CPRE	6
#define BK45TCR_M_B45CPRE	1 << BK45TCR_V_B45CPRE
#define BK45TCR_V_B45RTMA	7
#define BK45TCR_M_B45RTMA	1 << BK45TCR_V_B45RTMA
#define BK45TCR_V_B45MATC	8
#define BK45TCR_M_B45MATC	1 << BK45TCR_V_B45MATC
#define BK45TCR_V_B45RPRE	9
#define BK45TCR_M_B45RPRE	0x7 << BK45TCR_V_B45RPRE
#define BK45TCR_V_B45RPRE0	9
#define BK45TCR_M_B45RPRE0	1 << BK45TCR_V_B45RPRE0
#define BK45TCR_V_B45RPRE1	10
#define BK45TCR_M_B45RPRE1	1 << BK45TCR_V_B45RPRE1
#define BK45TCR_V_B45RPRE2	11
#define BK45TCR_M_B45RPRE2	1 << BK45TCR_V_B45RPRE2
#define BK45TCR_V_BNK45_ITL	13
#define BK45TCR_M_BNK45_ITL	1 << BK45TCR_V_BNK45_ITL

/*
** Bank 4/5 Write CAS Cycle Time 
*/
#define B45WRCPW_1T		BK45TCR_M_B45WRCPW0
#define B45WRCPW_2T		BK45TCR_M_B45WRCPW1
#define B45WRCPW_3T		(BK45TCR_M_B45WRCPW0 | BK45TCR_M_B45WRCPW1)

/*
** Bank 4/5 Read CAS Cycle Time 
*/
#define B45RDCPW_1T		0x000
#define B45RDCPW_2T		BK45TCR_M_B45RDCPW0
#define B45RDCPW_3T		BK45TCR_M_B45RDCPW1
#define B45RDCPW_4T		(BK45TCR_M_B45RDCPW0 | BK45TCR_M_B45RDCPW1)

/*
** Bank 4/5 RAS Precharge Time 
*/
#define B45RPRE_1PT5T		BK45TCR_M_B45RPRE1
#define B45RPRE_2T		(BK45TCR_M_B45RPRE0 | BK45TCR_M_B45RPRE1)
#define B45RPRE_2PT5T		BK45TCR_M_B45RPRE2
#define B45RPRE_3T		(BK45TCR_M_B45RPRE0 | BK45TCR_M_B45RPRE2)
#define B45RPRE_3PT5T		(BK45TCR_M_B45RPRE1 | BK45TCR_M_B45RPRE2)
#define B45RPRE_4T		(BK45TCR_M_B45RPRE0 | BK45TCR_M_B45RPRE1 | BK45TCR_M_B45RPRE2)

/* 
**
**  DRAM Configuration Register 1 (DRAMCR1) - Index 0x20B
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SIXRASEN	Six RAS# Control Line Enable 
**        <3>     1     BRSTWREN	Burst WRite Enable
**        <4>     1     PS0		DRAM Page Size 0
**        <5>     1     PS1		DRAM Page Size 1
**        <6>     1     PS2		DRAM Page Size 2
**        <7>     1     FSTPHDEC	Fast Page Hit Decode 
**        <9>     1     RONLYRF		DRAM Refresh Scheme 
**        <10>    1     RFRPRE0		RAS Precharge Time for Refresh Cycles 0
**        <11>    1     RFRPRE1		RAS Precharge Time for Refresh Cycles 1
**        <12>    1     RFRPW0		RAS Pulse Width for Refresh Cycles 0
**        <13>    1     RFRPW1		RAS Pulse Width for Refresh Cycles 1
**        <14>    1     XWITDOFFR	Extra Wait State for Lead-off Read Cycles
**        <15>    1     XWITDOFFW	Extra Wait State for Lead-off Write Cycles
**
**	  <1:2><8> Reserved
*/
#define DRAMCR_DRAMCR1_REG	0x20B
#define DRAMCR_DRAMCR1_INIT	( DRAMCR1_M_XWITDOFFR | DRAMCR1_M_XWITDOFFW )
#define DRAMCR1_V_SIXRASEN	0
#define DRAMCR1_M_SIXRASEN	1 << DRAMCR1_V_SIXRASEN
#define DRAMCR1_V_BRSTWREN	3
#define DRAMCR1_M_BRSTWREN	1 << DRAMCR1_V_BRSTWREN
#define DRAMCR1_V_PS		4
#define DRAMCR1_M_PS		0x7 << DRAMCR1_V_PS
#define DRAMCR1_V_PS0		4
#define DRAMCR1_M_PS0		1 << DRAMCR1_V_PS0
#define DRAMCR1_V_PS1		5
#define DRAMCR1_M_PS1		1 << DRAMCR1_V_PS1
#define DRAMCR1_V_PS2		6
#define DRAMCR1_M_PS2		1 << DRAMCR1_V_PS2
#define DRAMCR1_V_FSTPHDEC	7
#define DRAMCR1_M_FSTPHDEC	1 << DRAMCR1_V_FSTPHDEC
#define DRAMCR1_V_RONLYRF	9
#define DRAMCR1_M_RONLYRF	1 << DRAMCR1_V_RONLYRF
#define DRAMCR1_V_RFRPRE	10
#define DRAMCR1_M_RFRPRE	0x3 << DRAMCR1_V_RFRPRE
#define DRAMCR1_V_RFRPRE0	10
#define DRAMCR1_M_RFRPRE0	1 << DRAMCR1_V_RFRPRE0
#define DRAMCR1_V_RFRPRE1	11
#define DRAMCR1_M_RFRPRE1	1 << DRAMCR1_V_RFRPRE1
#define DRAMCR1_V_RFRPW		12
#define DRAMCR1_M_RFRPW		0x3 << DRAMCR1_V_RFRPW
#define DRAMCR1_V_RFRPW0	12
#define DRAMCR1_M_RFRPW0	1 << DRAMCR1_V_RFRPW0
#define DRAMCR1_V_RFRPW1	13
#define DRAMCR1_M_RFRPW1	1 << DRAMCR1_V_RFRPW1
#define DRAMCR1_V_XWITDOFFR	14
#define DRAMCR1_M_XWITDOFFR	1 << DRAMCR1_V_XWITDOFFR
#define DRAMCR1_V_XWITDOFFW	15
#define DRAMCR1_M_XWITDOFFW	1 << DRAMCR1_V_XWITDOFFW

/*
** DRAM Page Size
*/
#define PS_1KB			0x000
#define PS_2KB			DRAMCR1_M_PS0
#define PS_4KB			DRAMCR1_M_PS1
#define PS_8KB			(DRAMCR1_M_PS0 | DRAMCR1_M_PS1)
#define PS_16KB			DRAMCR1_M_PS2
#define PS_32KB			(DRAMCR1_M_PS0 | DRAMCR1_M_PS2)

/*
** RAS Precharge Time for Refresh Cycles
*/
#define RFRPRE_5T		0x000
#define RFRPRE_4T		DRAMCR1_M_RFRPRE0
#define RFRPRE_3T		DRAMCR1_M_RFRPRE1
#define RFRPRE_2T		(DRAMCR1_M_RFRPRE0 | DRAMCR1_M_RFRPRE1)

/*
** RAS Pulse Width for Refresh Cycles
*/
#define RFRPW_5T		0x000
#define RFRPW_4T		DRAMCR1_M_RFRPW0
#define RFRPW_3T		DRAMCR1_M_RFRPW1
#define RFRPW_2T		(DRAMCR1_M_RFRPW0 | DRAMCR1_M_RFRPW1)

/* 
**
**  DRAM Configuration Register 2 (DRAMCR2) - Index 0x20C
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <9>     1     ENPARADRL	Enable Parity Error Address Latch 
**        <10>    1     ENPARCK0	Enable Parity Check for Bank 0
**        <11>    1     ENPARCK1	Enable Parity Check for Bank 1
**        <12>    1     ENPARCK2	Enable Parity Check for Bank 2
**        <13>    1     ENPARCK3	Enable Parity Check for Bank 3
**        <14>    1     ENPARCK4	Enable Parity Check for Bank 4
**        <15>    1     ENPARCK5	Enable Parity Check for Bank 5
**
**	  <0:8> Reserved
*/
#define DRAMCR_DRAMCR2_REG	0x20C
#define DRAMCR_DRAMCR2_INIT	0x0000
#define DRAMCR2_V_ENPARADRL	9
#define DRAMCR2_M_ENPARADRL	1 << DRAMCR2_V_ENPARADRL
#define DRAMCR2_V_ENPARCK0	10
#define DRAMCR2_M_ENPARCK0	1 << DRAMCR2_V_ENPARCK0
#define DRAMCR2_V_ENPARCK1	11
#define DRAMCR2_M_ENPARCK1	1 << DRAMCR2_V_ENPARCK1
#define DRAMCR2_V_ENPARCK2	12
#define DRAMCR2_M_ENPARCK2	1 << DRAMCR2_V_ENPARCK2
#define DRAMCR2_V_ENPARCK3	13
#define DRAMCR2_M_ENPARCK3	1 << DRAMCR2_V_ENPARCK3
#define DRAMCR2_V_ENPARCK4	14
#define DRAMCR2_M_ENPARCK4	1 << DRAMCR2_V_ENPARCK4
#define DRAMCR2_V_ENPARCK5	15
#define DRAMCR2_M_ENPARCK5	1 << DRAMCR2_V_ENPARCK5

/* 
**
**  DRAM Configuration Register 3 (DRAMCR3) - Index 0x20D
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     RAS0DRV0	Driving Strength of RAS0# Output 0
**        <1>     1     RAS0DRV1	Driving Strength of RAS0# Output 1
**        <2>     1     RAS0DRV2	Driving Strength of RAS0# Output 2
**        <3>     1     RAS0DRV3	Driving Strength of RAS0# Output 3
**        <4>     1     RAS1DRV0	Driving Strength of RAS1# Output 0
**        <5>     1     RAS1DRV1	Driving Strength of RAS1# Output 1
**        <6>     1     RAS1DRV2	Driving Strength of RAS1# Output 2
**        <7>     1     RAS1DRV3	Driving Strength of RAS1# Output 3
**        <8>     1     RAS2DRV0	Driving Strength of RAS2# Output 0
**        <9>     1     RAS2DRV1	Driving Strength of RAS2# Output 1
**        <10>    1     RAS2DRV2	Driving Strength of RAS2# Output 2
**        <11>    1     RAS2DRV3	Driving Strength of RAS2# Output 3
**        <12>    1     RAS3DRV0	Driving Strength of RAS3# Output 0
**        <13>    1     RAS3DRV1	Driving Strength of RAS3# Output 1
**        <14>    1     RAS3DRV2	Driving Strength of RAS3# Output 2
**        <15>    1     RAS3DRV3	Driving Strength of RAS3# Output 3
**
*/
#define DRAMCR_DRAMCR3_REG	0x20D
#define DRAMCR_DRAMCR3_INIT	( DRAMCR3_M_RAS0DRV0 | DRAMCR3_M_RAS0DRV1 | \
				  DRAMCR3_M_RAS0DRV2 | DRAMCR3_M_RAS1DRV0 | \
				  DRAMCR3_M_RAS1DRV1 | DRAMCR3_M_RAS1DRV2 | \
				  DRAMCR3_M_RAS2DRV0 | DRAMCR3_M_RAS2DRV1 | \
				  DRAMCR3_M_RAS2DRV2 | DRAMCR3_M_RAS3DRV0 | \
				  DRAMCR3_M_RAS3DRV1 | DRAMCR3_M_RAS3DRV2 )
#define DRAMCR3_V_RAS0DRV	0
#define DRAMCR3_M_RAS0DRV	0xF << DRAMCR3_V_RAS0DRV
#define DRAMCR3_V_RAS0DRV0	0
#define DRAMCR3_M_RAS0DRV0	1 << DRAMCR3_V_RAS0DRV0
#define DRAMCR3_V_RAS0DRV1	1
#define DRAMCR3_M_RAS0DRV1	1 << DRAMCR3_V_RAS0DRV1
#define DRAMCR3_V_RAS0DRV2	2
#define DRAMCR3_M_RAS0DRV2	1 << DRAMCR3_V_RAS0DRV2
#define DRAMCR3_V_RAS0DRV3	3
#define DRAMCR3_M_RAS0DRV3	1 << DRAMCR3_V_RAS0DRV3
#define DRAMCR3_V_RAS1DRV	4
#define DRAMCR3_M_RAS1DRV	0xF << DRAMCR3_V_RAS1DRV
#define DRAMCR3_V_RAS1DRV0	4
#define DRAMCR3_M_RAS1DRV0	1 << DRAMCR3_V_RAS1DRV0
#define DRAMCR3_V_RAS1DRV1	5
#define DRAMCR3_M_RAS1DRV1	1 << DRAMCR3_V_RAS1DRV1
#define DRAMCR3_V_RAS1DRV2	6
#define DRAMCR3_M_RAS1DRV2	1 << DRAMCR3_V_RAS1DRV2
#define DRAMCR3_V_RAS1DRV3	7
#define DRAMCR3_M_RAS1DRV3	1 << DRAMCR3_V_RAS1DRV3
#define DRAMCR3_V_RAS2DRV	8
#define DRAMCR3_M_RAS2DRV	0xF << DRAMCR3_V_RAS2DRV
#define DRAMCR3_V_RAS2DRV0	8
#define DRAMCR3_M_RAS2DRV0	1 << DRAMCR3_V_RAS2DRV0
#define DRAMCR3_V_RAS2DRV1	9
#define DRAMCR3_M_RAS2DRV1	1 << DRAMCR3_V_RAS2DRV1
#define DRAMCR3_V_RAS2DRV2	10
#define DRAMCR3_M_RAS2DRV2	1 << DRAMCR3_V_RAS2DRV2
#define DRAMCR3_V_RAS2DRV3	11
#define DRAMCR3_M_RAS2DRV3	1 << DRAMCR3_V_RAS2DRV3
#define DRAMCR3_V_RAS3DRV	12
#define DRAMCR3_M_RAS3DRV	0xF << DRAMCR3_V_RAS3DRV
#define DRAMCR3_V_RAS3DRV0	12
#define DRAMCR3_M_RAS3DRV0	1 << DRAMCR3_V_RAS3DRV0
#define DRAMCR3_V_RAS3DRV1	13
#define DRAMCR3_M_RAS3DRV1	1 << DRAMCR3_V_RAS3DRV1
#define DRAMCR3_V_RAS3DRV2	14
#define DRAMCR3_M_RAS3DRV2	1 << DRAMCR3_V_RAS3DRV2
#define DRAMCR3_V_RAS3DRV3	15
#define DRAMCR3_M_RAS3DRV3	1 << DRAMCR3_V_RAS3DRV3

/*
** Driving Strength of RAS0# Output (mA)
*/
#define RAS0DRV_4MA		DRAMCR3_M_RAS0DRV0
#define RAS0DRV_8MA		(DRAMCR3_M_RAS0DRV0 | DRAMCR3_M_RAS0DRV1)
#define RAS0DRV_12MA		(DRAMCR3_M_RAS0DRV0 | DRAMCR3_M_RAS0DRV1 | DRAMCR3_M_RAS0DRV2

/*
** Driving Strength of RAS1# Output (mA)
*/
#define RAS1DRV_4MA		DRAMCR3_M_RAS1DRV0
#define RAS1DRV_8MA		(DRAMCR3_M_RAS1DRV0 | DRAMCR3_M_RAS1DRV1)
#define RAS1DRV_12MA		(DRAMCR3_M_RAS1DRV0 | DRAMCR3_M_RAS1DRV1 | DRAMCR3_M_RAS1DRV2

/*
** Driving Strength of RAS2# Output (mA)
*/
#define RAS2DRV_4MA		DRAMCR3_M_RAS2DRV0
#define RAS2DRV_8MA		(DRAMCR3_M_RAS2DRV0 | DRAMCR3_M_RAS2DRV1)
#define RAS2DRV_12MA		(DRAMCR3_M_RAS2DRV0 | DRAMCR3_M_RAS2DRV1 | DRAMCR3_M_RAS2DRV2

/*
** Driving Strength of RAS3# Output (mA)
*/
#define RAS3DRV_4MA		DRAMCR3_M_RAS3DRV0
#define RAS3DRV_8MA		(DRAMCR3_M_RAS3DRV0 | DRAMCR3_M_RAS3DRV1)
#define RAS3DRV_12MA		(DRAMCR3_M_RAS3DRV0 | DRAMCR3_M_RAS3DRV1 | DRAMCR3_M_RAS3DRV2

/* 
**
**  DRAM Configuration Register 4 (DRAMCR4) - Index 0x20E
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     MADRV0		Driving Strength of MA Output 0
**        <1>     1     MADRV1		Driving Strength of MA Output 1
**        <2>     1     MADRV2		Driving Strength of MA Output 2
**        <3>     1     MADRV3		Driving Strength of MA Output 3
**        <4>     1     CASADRV0	Driving Strength of CASA Output 0
**        <5>     1     CASADRV1	Driving Strength of CASA Output 1
**        <6>     1     CASADRV2	Driving Strength of CASA Output 2
**        <7>     1     CASADRV3	Driving Strength of CASA Output 3
**        <8>     1     CASBDRV0	Driving Strength of CASB Output 0
**        <9>     1     CASBDRV1	Driving Strength of CASB Output 1
**        <10>    1     CASBDRV2	Driving Strength of CASB Output 2
**        <11>    1     CASBDRV3	Driving Strength of CASB Output 3
**
*/
#define DRAMCR_DRAMCR4_REG	0x20E
#define DRAMCR_DRAMCR4_INIT	( DRAMCR4_M_MADRV0   | DRAMCR4_M_MADRV1   | \
				  DRAMCR4_M_MADRV2   | DRAMCR4_M_CASADRV0 | \
				  DRAMCR4_M_CASADRV1 | DRAMCR4_M_CASADRV2 | \
				  DRAMCR4_M_CASBDRV0 | DRAMCR4_M_CASBDRV1 | \
				  DRAMCR4_M_CASBDRV2 )
#define DRAMCR4_V_MADRV		0
#define DRAMCR4_M_MADRV		0xF << DRAMCR4_V_MADRV
#define DRAMCR4_V_MADRV0	0
#define DRAMCR4_M_MADRV0	1 << DRAMCR4_V_MADRV0
#define DRAMCR4_V_MADRV1	1
#define DRAMCR4_M_MADRV1	1 << DRAMCR4_V_MADRV1
#define DRAMCR4_V_MADRV2	2
#define DRAMCR4_M_MADRV2	1 << DRAMCR4_V_MADRV2
#define DRAMCR4_V_MADRV3	3
#define DRAMCR4_M_MADRV3	1 << DRAMCR4_V_MADRV3
#define DRAMCR4_V_CASADRV	4
#define DRAMCR4_M_CASADRV	0xF << DRAMCR4_V_CASADRV
#define DRAMCR4_V_CASADRV0	4
#define DRAMCR4_M_CASADRV0	1 << DRAMCR4_V_CASADRV0
#define DRAMCR4_V_CASADRV1	5
#define DRAMCR4_M_CASADRV1	1 << DRAMCR4_V_CASADRV1
#define DRAMCR4_V_CASADRV2	6
#define DRAMCR4_M_CASADRV2	1 << DRAMCR4_V_CASADRV2
#define DRAMCR4_V_CASADRV3	7
#define DRAMCR4_M_CASADRV3	1 << DRAMCR4_V_CASADRV3
#define DRAMCR4_V_CASBDRV	8
#define DRAMCR4_M_CASBDRV	0xF << DRAMCR4_V_CASBDRV
#define DRAMCR4_V_CASBDRV0	8
#define DRAMCR4_M_CASBDRV0	1 << DRAMCR4_V_CASBDRV0
#define DRAMCR4_V_CASBDRV1	9
#define DRAMCR4_M_CASBDRV1	1 << DRAMCR4_V_CASBDRV1
#define DRAMCR4_V_CASBDRV2	10
#define DRAMCR4_M_CASBDRV2	1 << DRAMCR4_V_CASBDRV2
#define DRAMCR4_V_CASBDRV3	11
#define DRAMCR4_M_CASBDRV3	1 << DRAMCR4_V_CASBDRV3

/*
** Driving Strength of MA Output (mA)
*/
#define MADRV_4MA		DRAMCR2_M_MADRV0
#define MADRV_8MA		(DRAMCR2_M_MADRV0 | DRAMCR2_M_MADRV1)
#define MADRV_12MA		(DRAMCR2_M_MADRV0 | DRAMCR2_M_MADRV1 | DRAMCR2_M_MADRV2

/*
** Driving Strength of CASA Output (mA)
*/
#define CASADRV_4MA		DRAMCR2_M_CASADRV0
#define CASADRV_8MA		(DRAMCR2_M_CASADRV0 | DRAMCR2_M_CASADRV1)
#define CASADRV_12MA		(DRAMCR2_M_CASADRV0 | DRAMCR2_M_CASADRV1 | DRAMCR2_M_CASADRV2

/*
** Driving Strength of CASB Output (mA)
*/
#define CASBDRV_4MA		DRAMCR2_M_CASBDRV0
#define CASBDRV_8MA		(DRAMCR2_M_CASBDRV0 | DRAMCR2_M_CASBDRV1)
#define CASBDRV_12MA		(DRAMCR2_M_CASBDRV0 | DRAMCR2_M_CASBDRV1 | DRAMCR2_M_CASBDRV2

/* 
**
**  DRAM Configuration Register 5 (DRAMCR5) - Index 0x20F
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <1>     1     FLASHENB	Flash Enable
**        <10>    1     ENC0ROMCS	Enable ROMCS# for C0000H-C7FFFH Region
**        <11>    1     ENC8ROMCS	Enable ROMCS# for C8000H-CFFFFH Region
**        <12>    1     END0ROMCS	Enable ROMCS# for D0000H-D7FFFH Region
**        <13>    1     END8ROMCS	Enable ROMCS# for D8000H-DFFFFH Region
**        <14>    1     ENE0ROMCS	Enable ROMCS# for E0000H-E7FFFH Region
**        <15>    1     ENE8ROMCS	Enable ROMCS# for E8000H-EFFFFH Region
**
**	  <0><2:9> Reserved
*/
#define DRAMCR_DRAMCR5_REG	0x20F
#define DRAMCR_DRAMCR5_INIT	0x0000
#define DRAMCR5_V_FLASHENB	0
#define DRAMCR5_M_FLASHENB	1 << DRAMCR5_V_FLASHENB
#define DRAMCR5_V_ENC0ROMCS	10
#define DRAMCR5_M_ENC0ROMCS	1 << DRAMCR5_V_ENC0ROMCS
#define DRAMCR5_V_ENC8ROMCS	11
#define DRAMCR5_M_ENC8ROMCS	1 << DRAMCR5_V_ENC8ROMCS
#define DRAMCR5_V_END0ROMCS	12
#define DRAMCR5_M_END0ROMCS	1 << DRAMCR5_V_ENC0ROMCS
#define DRAMCR5_V_END8ROMCS	13
#define DRAMCR5_M_END8ROMCS	1 << DRAMCR5_V_ENC8ROMCS
#define DRAMCR5_V_ENE0ROMCS	14
#define DRAMCR5_M_ENE0ROMCS	1 << DRAMCR5_V_ENC0ROMCS
#define DRAMCR5_V_ENE8ROMCS	15
#define DRAMCR5_M_ENE8ROMCS	1 << DRAMCR5_V_ENC8ROMCS

/*
** Cache Controller Registers
**
*/

/* 
**
**  Cache Control Register 1 (CCR1) - Index 0x400
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     L2CST0		Level 2 Cache Status 0
**        <1>     1     L2CST1		Level 2 Cache Status 1
**        <2>     1     L2CST2		Level 2 Cache Status 2
**        <3>     1     L2CST3		Level 2 Cache Status 3
**        <4>     1     CSIZE0		Level 2 Cache Data 0
**        <5>     1     CSIZE1		Level 2 Cache Data 1
**        <6>     1     CSIZE2		Level 2 Cache Data 2
**        <7>     1     CSIZE3		Level 2 Cache Data 3
**        <8>     1     L1WB		Enable Level 1 Write Back Support
**        <9>     1     WPL2KEN		L2 Cache Write Protect 
**
**	  <10:15> Reserved
*/
#define CCR_CCR1_REG		0x400
#define CCR1_V_L2CST		0
#define CCR1_M_L2CST		0xF << CCR1_V_L2CST
#define CCR1_V_L2CST0		0
#define CCR1_M_L2CST0		1 << CCR1_V_L2CST0
#define CCR1_V_L2CST1		1
#define CCR1_M_L2CST1		1 << CCR1_V_L2CST1
#define CCR1_V_L2CST2		2
#define CCR1_M_L2CST2		1 << CCR1_V_L2CST2
#define CCR1_V_L2CST3		3
#define CCR1_M_L2CST3		1 << CCR1_V_L2CST3
#define CCR1_V_CSIZE		4
#define CCR1_M_CSIZE		0xF << CCR1_V_CSIZE
#define CCR1_V_CSIZE0		4
#define CCR1_M_CSIZE0		1 << CCR1_V_CSIZE0
#define CCR1_V_CSIZE1		5
#define CCR1_M_CSIZE1		1 << CCR1_V_CSIZE1
#define CCR1_V_CSIZE2		6
#define CCR1_M_CSIZE2		1 << CCR1_V_CSIZE2
#define CCR1_V_CSIZE3		7
#define CCR1_M_CSIZE3		1 << CCR1_V_CSIZE3
#define CCR1_V_L1WB		8
#define CCR1_M_L1WB		1 << CCR1_V_L1WB
#define CCR1_V_WPL2KEN	9
#define CCR1_M_WPL2KEN	1 << CCR1_V_WPL2KEN

/*
** Level 2 Cache Status 
*/
#define L2CST_STNDBY		0x0000
#define L2CST_INIT		CCR1_M_L2CST0
#define L2CST_WRTHRU		CCR1_M_L2CST1
#define L2CST_WRBACK		(CCR1_M_L2CST0 | CCR1_M_L2CST1)
#define L2CST_FLUSH		CCR1_M_L2CST2
#define L2CST_WRTHRUDR		(CCR1_M_L2CST0 | CCR1_M_L2CST2)
#define L2CST_WRBACKDR		(CCR1_M_L2CST1 | CCR1_M_L2CST2)

/*
** Level 2 Cache Size
*/
#define CSIZE_64KB_2BNK		CCR1_M_CSIZE0
#define CSIZE_128KB_1BNK 	CCR1_M_CSIZE1
#define CSIZE_256KB_2BNK	(CCR1_M_CSIZE0 | CCR1_M_CSIZE1)
#define CSIZE_512KB_1BNK	CCR1_M_CSIZE2
#define CSIZE_1MB_2BNK		(CCR1_M_CSIZE0 | CCR1_M_CSIZE2)
#define CSIZE_256KB_1BNK	(CCR1_M_CSIZE0 | CCR1_M_CSIZE1 | CCR1_M_CSIZE3)

/* 
**
**  Cache Control Register 2 (CCR2) - Index 0x401
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     TWIDTH		Width of L2 Cache TAG Data
**        <1>     1     DTYEN		Configuration of L2 Cache TAG Field
**        <2>     1     DRTWEN		Configuration of L2 Cache TAG Write
**        <7>     1     EHITEN		External Comparator is Used
**
**	  <3:6><8:15> Reserved
*/
#define CCR_CCR2_REG		0x401
#define CCR2_V_TWIDTH		0
#define CCR2_M_TWIDTH		1 << CCR2_V_TWIDTH
#define CCR2_V_DTYEN		1
#define CCR2_M_DTYEN		1 << CCR2_V_DTYEN
#define CCR2_V_DRTWEN		2
#define CCR2_M_DRTWEN		1 << CCR2_V_DRTWEN
#define CCR2_V_EHITEN		7
#define CCR2_M_EHITEN		1 << CCR2_V_EHITEN

/* 
**
**  Cache Control Register 3 (CCR3) - Index 0x402
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     RDL0		CPU Cache Read Lead-off Cycles 0
**        <1>     1     RDL1		CPU Cache Read Lead-off Cycles 1
**        <2>     1     RDB2T		CPU Cache Read Burst Cycles
**        <3>     1     WRL0		CPU Cache Write Lead-off Cycles 0
**        <4>     1     WRL1		CPU Cache Write Lead-off Cycles 1
**        <5>     1     WRB2T		CPU Cache Write Burst Cycles
**        <6>     1     WRTEG0		Cache Write Enable Trailing Edge Timing 0
**        <7>     1     WRTEG1		Cache Write Enable Trailing Edge Timing 1
**        <8>     1     RDTEG0		Cache Write Enable Trailing Edge Timing 0
**        <9>     1     RDTEG1		Cache Write Enable Trailing Edge Timing 1
**
**	  <10:15> Reserved
*/
#define CCR_CCR3_REG		0x402
#define CCR3_V_RDL		0
#define CCR3_M_RDL		0x3 << CCR3_V_RDL
#define CCR3_V_RDL0		0
#define CCR3_M_RDL0		1 << CCR3_V_RDL0
#define CCR3_V_RDL1		2
#define CCR3_M_RDL1		1 << CCR3_V_RDL1
#define CCR3_V_RDB2T		3
#define CCR3_M_RDB2T		1 << CCR3_V_RDB2T
#define CCR3_V_WRL		3
#define CCR3_M_WRL		0x3 << CCR3_V_WRL
#define CCR3_V_WRL0		3
#define CCR3_M_WRL0		1 << CCR3_V_WRL0
#define CCR3_V_WRL1		4
#define CCR3_M_WRL1		1 << CCR3_V_WRL1
#define CCR3_V_WRB2T		5
#define CCR3_M_WRB2T		1 << CCR3_V_WRB2T
#define CCR3_V_WRTEG		6
#define CCR3_M_WRTEG		0x3 << CCR3_V_WRTEG
#define CCR3_V_WRTEG0		6
#define CCR3_M_WRTEG0		1 << CCR3_V_WRTEG0
#define CCR3_V_WRTEG1		7
#define CCR3_M_WRTEG1		1 << CCR3_V_WRTEG1
#define CCR3_V_RDTEG		8
#define CCR3_M_RDTEG		0x3 << CCR3_V_RDTEG
#define CCR3_V_RDTEG0		8
#define CCR3_M_RDTEG0		1 << CCR3_V_RDTEG0
#define CCR3_V_RDTEG1		9
#define CCR3_M_RDTEG1		1 << CCR3_V_RDTEG1

/*
** CPU Cache Read Lead-off Cycles
*/
#define RDL_2T			0x000
#define RDL_3T			CCR3_M_RDL0
#define RDL_4T			CCR3_M_RDL1

/*
** CPU Cache Write Lead-off Cycles
*/
#define WRL_2T			0x000
#define WRL_3T			CCR3_M_WRL0
#define WRL_4T			CCR3_M_WRL1

/*
** Cache Write Enable Trailing Edge Timing for Write Hit Cycles
*/
#define WRTEG_SYNCH		0x000
#define WRTEG_EARLY		CCR3_M_WRTEG0

/*
** Cache Write Enable Trailing Edge Timing for Line Fill Cycles
*/
#define RDTEG_SYNCH		0x000
#define RDTEG_EARLY		CCR3_M_RDTEG0

/* 
**
**  Cache Control Register 4 (CCR4) - Index 0x403
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <1>     1     TAGCK0		TAG RAM Timing Checking Point 0
**        <2>     1     TAGCK1		TAG RAM Timing Checking Point 1
**        <3>     1     DRTWETR0	Dirty Bit Write Timing Point 0
**        <4>     1     DRTWETR1	Dirty Bit Write Timing Point 1
**
**	  <0><5:15> Reserved
*/
#define CCR_CCR4_REG		0x403
#define CCR4_V_TAGCK		0
#define CCR4_M_TAGCK		0x3 << CCR4_V_TAGCK
#define CCR4_V_TAGCK0		0
#define CCR4_M_TAGCK0		1 << CCR4_V_TAGCK0
#define CCR4_V_TAGCK1		1
#define CCR4_M_TAGCK1		1 << CCR4_V_TAGCK1
#define CCR4_V_DRTWETR		0
#define CCR4_M_DRTWETR		0x3 << CCR4_V_DRTWETR
#define CCR4_V_DRTWETR0		0
#define CCR4_M_DRTWETR0		1 << CCR4_V_DRTWETR0
#define CCR4_V_DRTWETR1		1
#define CCR4_M_DRTWETR1		1 << CCR4_V_DRTWETR1

/*
** TAG RAM Timing Checking Point
*/
#define TAGCK_ENDT1		0x000
#define TAGCK_MIDT2_EARLY	CCR4_M_TAGCK0
#define TAGCK_MIDT2_NORM	CCR4_M_TAGCK1
#define TAGCK_ENDT2		(CCR4_M_TAGCK0 | CCR4_M_TAGCK1)

/*
** Dirty Bit Write Enable Trailing Edge Timing Point
*/
#define DRTWETR_EARLYEST	CCR4_M_DRTWETR0
#define DRTWETR_EARLYER		CCR4_M_DRTWETR1
#define DRTWETR_SYNCH		(CCR4_M_DRTWETR0 | CCR4_M_DRTWETR1)

/* 
**
**  Global Control Register 1 (GCR1) - Index 0x700
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     CPUHITMWS	CPU HITM# Sampling Wait State Selection
**
**	  <2:15> Reserved
*/
#define CCR_GCR1_REG		0x700
#define CCR_GCR1_INIT		0x0000
#define GCR1_V_CPUHITMWS	0
#define GCR1_M_CPUHITMWS	1 << GCR1_V_CPUHITMWS

/****************************************************
**		      	SEQUOIA-2                  **
*****************************************************/

/* 
**
**  AT Miscellaneous Control Register 1 (ATMCR1) - Index 0x300
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SYSDIV0		SYSCLK Divisor Select 0
**        <1>     1     SYSDIV1		SYSCLK Divisor Select 1
**        <2>     1     SYSDIV2		SYSCLK Divisor Select 2
**        <4>     1     IDERDYDLY0	Turbo IDE Cycle Ready Delay Select 0
**        <5>     1     IDERDYDLY1	Turbo IDE Cycle Ready Delay Select 1
**        <6>     1     B2BD0		Back-to-Back I/O Delay Select 0
**        <7>     1     B2BD1		Back-to-Back I/O Delay Select 1
**	  <9>	  1     PARITYEN	Global Parity Enable
**	  <10>	  1     ATREFDIS	AT Bus Refresh Disable
**	  <11>	  1     HIDREFEN	Hidden AT Refresh Enable
**
**	  <3><8><12:15> Reserved
*/
#define SEQ2_ATMCR_REG		0x300
#define SEQ2_ATMCR_INIT		( SYSDIV_4 | IDERDYDLY_2FS1XCLK | B2BD_3 | \
				  ATMCR1_M_ATREFDIS | ATMCR1_M_HIDREFEN )
#define ATMCR1_V_SYSDIV		0
#define ATMCR1_M_SYSDIV		0x7 << ATMCR1_V_SYSDIV
#define ATMCR1_V_SYSDIV0	0
#define ATMCR1_M_SYSDIV0	1 << ATMCR1_V_SYSDIV0
#define ATMCR1_V_SYSDIV1	1
#define ATMCR1_M_SYSDIV1	1 << ATMCR1_V_SYSDIV1
#define ATMCR1_V_SYSDIV2	2
#define ATMCR1_M_SYSDIV2	1 << ATMCR1_V_SYSDIV2
#define ATMCR1_V_IDERDYDLY	4
#define ATMCR1_M_IDERDYDLY	0x3 << ATMCR1_V_IDERDYDLY
#define ATMCR1_V_IDERDYDLY0	4
#define ATMCR1_M_IDERDYDLY0	1 << ATMCR1_V_IDERDYDLY0
#define ATMCR1_V_IDERDYDLY1	5
#define ATMCR1_M_IDERDYDLY1	1 << ATMCR1_V_IDERDYDLY1
#define ATMCR1_V_B2BD		6
#define ATMCR1_M_B2BD		0x3 << ATMCR1_V_B2BD
#define ATMCR1_V_B2BD0		6
#define ATMCR1_M_B2BD0		1 << ATMCR1_V_B2BD0
#define ATMCR1_V_B2BD1		7
#define ATMCR1_M_B2BD1		1 << ATMCR1_V_B2BD1
#define ATMCR1_V_PARITYEN	9
#define ATMCR1_M_PARITYEN	1 << ATMCR1_V_PARITYEN
#define ATMCR1_V_ATREFDIS	10
#define ATMCR1_M_ATREFDIS	1 << ATMCR1_V_ATREFDIS
#define ATMCR1_V_HIDREFEN	11
#define ATMCR1_M_HIDREFEN	1 << ATMCR1_V_HIDREFEN

/*
** SYSCLK Divisor Select
*/
#define SYSDIV_3		ATMCR1_M_SYSDIV0
#define SYSDIV_4		ATMCR1_M_SYSDIV1
#define SYSDIV_5		(ATMCR1_M_SYSDIV0 | ATMCR1_M_SYSDIV1)
#define SYSDIV_6		ATMCR1_M_SYSDIV2

/*
** Turbo IDE Cycle Ready Delay Select
*/
#define IDERDYDLY_NONE		0x000
#define IDERDYDLY_1FS1XCLK	ATMCR1_M_IDERDYDLY0
#define IDERDYDLY_2FS1XCLK 	ATMCR1_M_IDERDYDLY1

/*
** Back-to-Back Delay Select
*/
#define B2BD_0			0x000
#define B2BD_1			ATMCR1_M_B2BD0
#define B2BD_2			ATMCR1_M_B2BD1
#define B2BD_3			(ATMCR1_M_B2BD0 | ATMCR1_M_B2BD1)

/* 
**
**  AT Miscellaneous Control Register 2 (ATMCR2) - Index 0x301
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     IDECMDW0	Turbo IDE Command Width Select 0
**        <1>     1     IDECMDW1	Turbo IDE Command Width Select 1
**        <2>     1     IDECMDW2	Turbo IDE Command Width Select 2
**        <3>     1     IDEB2BDLY0	Turbo IDE Back-to-Back Cycle Delay Select 0
**        <4>     1     IDEB2BDLY1	Turbo IDE Back-to-Back Cycle Delay Select 1
**        <5>     1     IDEB2BDLY2	Turbo IDE Back-to-Back Cycle Delay Select 2
**        <6>     1     GIDENABLE	Global IDE Enable
**	  <11>	  1     EXTATADD	Extended AT Address
**	  <12>	  1     DLYLOCAL	Delay Sampling Point for LOCAL#
**	  <14>	  1     BSEREN		BSER Enable
**	  <15>	  1     DMACLKDIS	DMA Modular Clock Disable
**
**	  <7:10><13> Reserved
*/
#define SEQ2_ATMCR2_REG		0x301
#define SEQ2_ATMCR2_INIT	( ATMCR2_M_IDECMDW    | ATMCR2_M_IDEB2BDLY  | \
				  ATMCR2_M_IDEB2BDLY1 | ATMCR2_M_IDEB2BDLY2 | \
				  ATMCR2_M_GIDENABLE  | ATMCR2_M_EXTATADD | \
				  ATMCR2_M_DLYLOCAL   | ATMCR2_M_BSEREN )
#define ATMCR2_V_IDECMDW	0
#define ATMCR2_M_IDECMDW	(0x7 << ATMCR2_V_IDECMDW)
#define ATMCR2_V_IDECMDW0	0
#define ATMCR2_M_IDECMDW0	(1 << ATMCR2_V_IDECMDW0)
#define ATMCR2_V_IDECMDW1	1
#define ATMCR2_M_IDECMDW1	(1 << ATMCR2_V_IDECMDW1)
#define ATMCR2_V_IDECMDW2	2
#define ATMCR2_M_IDECMDW2	(1 << ATMCR2_V_IDECMDW2)
#define ATMCR2_V_IDEB2BDLY	3
#define ATMCR2_M_IDEB2BDLY	(0x7 << ATMCR2_V_IDEB2BDLY)
#define ATMCR2_V_IDEB2BDLY0	3
#define ATMCR2_M_IDEB2BDLY0	(1 << ATMCR2_V_IDEB2BDLY0)
#define ATMCR2_V_IDEB2BDLY1	4
#define ATMCR2_M_IDEB2BDLY1	(1 << ATMCR2_V_IDEB2BDLY1)
#define ATMCR2_V_IDEB2BDLY2	5
#define ATMCR2_M_IDEB2BDLY2	(1 << ATMCR2_V_IDEB2BDLY2)
#define ATMCR2_V_GIDENABLE	6
#define ATMCR2_M_GIDENABLE	(1 << ATMCR2_V_GIDENABLE)
#define ATMCR2_V_EXTATADD	11
#define ATMCR2_M_EXTATADD	(1 << ATMCR2_V_EXTATADD)
#define ATMCR2_V_DLYLOCAL	12
#define ATMCR2_M_DLYLOCAL	(1 << ATMCR2_V_DLYLOCAL)
#define ATMCR2_V_BSEREN		14
#define ATMCR2_M_BSEREN		(1 << ATMCR2_V_BSEREN)
#define ATMCR2_V_DMACLKDIS	15
#define ATMCR2_M_DMACLKDIS	(1 << ATMCR2_V_DMACLKDIS)

/*
** Turbo IDE Command Width Select
*/
#define IDECMDW_2FS1XCLK	0x000
#define IDECMDW_3FS1XCLK	ATMCR2_M_IDECMDW0
#define IDECMDW_4FS1XCLK	ATMCR2_M_IDECMDW1
#define IDECMDW_5FS1XCLK	(ATMCR2_M_IDECMDW0 | ATMCR2_M_IDECMDW1)
#define IDECMDW_6FS1XCLK	ATMCR2_M_IDECMDW2
#define IDECMDW_7FS1XCLK	(ATMCR2_M_IDECMDW0 | ATMCR2_M_IDECMDW2)
#define IDECMDW_8FS1XCLK	(ATMCR2_M_IDECMDW1 | ATMCR2_M_IDECMDW2)
#define IDECMDW_9FS1XCLK	(ATMCR2_M_IDECMDW0 | ATMCR2_M_IDECMDW1 | ATMCR2_M_IDECMDW2)

/*
** Turbo IDE Back=to-Back Cycle Delay Select
*/
#define IDEB2BDLY_2FS1XCLK	0x000
#define IDEB2BDLY_3FS1XCLK	ATMCR2_M_IDEB2BDLY0
#define IDEB2BDLY_4FS1XCLK	ATMCR2_M_IDEB2BDLY1
#define IDEB2BDLY_5FS1XCLK	(ATMCR2_M_IDEB2BDLY0 | ATMCR2_M_IDEB2BDLY1)
#define IDEB2BDLY_6FS1XCLK	ATMCR2_M_IDEB2BDLY2
#define IDEB2BDLY_7FS1XCLK	(ATMCR2_M_IDEB2BDLY0 | ATMCR2_M_IDEB2BDLY2)
#define IDEB2BDLY_8FS1XCLK	(ATMCR2_M_IDEB2BDLY1 | ATMCR2_M_IDEB2BDLY2)
#define IDEB2BDLY_9FS1XCLK	(ATMCR2_M_IDEB2BDLY0 | ATMCR2_M_IDEB2BDLY1 | ATMCR2_M_IDEB2BDLY2)

/* 
**
**  SEQUOIA-2 Pin Select Register  (SEQ2PSR) - Index 0x302
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     DPBUSEN		DP Bus Enable
**        <1>     1     GPIOPINEN	General Purpose I/O [7:4] Pin Enable
**        <4>     1     IDEPINEN	IDE Pin Enable
**        <5>     1     LOCALPINEN	Additional LOCAL Pin Enable
**        <8>     1     ATREFADDEN	AT Refresh Address Enable
**	  <14>	  1     IDECMDEN	IDE Command Enable
**	  <15>	  1     SQ2TYPESEL	SEQUOIA-2 Type Select
**
**	  <2:3><6:7><9:13> Reserved
*/
#define SEQ2_SEQ2PSR_REG	0x302
#define SEQ2_SEQ2PSR_INIT	( SEQ2PSR_M_DPBUSEN | SEQ2PSR_M_LOCALPINEN )
#define SEQ2PSR_V_DPBUSEN	0
#define SEQ2PSR_M_DPBUSEN	1 << SEQ2PSR_V_DPBUSEN
#define SEQ2PSR_V_GPIOPINEN	1
#define SEQ2PSR_M_GPIOPINEN	1 << SEQ2PSR_V_GPIOPINEN
#define SEQ2PSR_V_IDEPINEN	4
#define SEQ2PSR_M_IDEPINEN	1 << SEQ2PSR_V_IDEPINEN
#define SEQ2PSR_V_LOCALPINEN	5
#define SEQ2PSR_M_LOCALPINEN	1 << SEQ2PSR_V_LOCALPINEN
#define SEQ2PSR_V_ATREFADDEN	8
#define SEQ2PSR_M_ATREFADDEN	1 << SEQ2PSR_V_ATREFADDEN
#define SEQ2PSR_V_IDECMDEN	14
#define SEQ2PSR_M_IDECMDEN	1 << SEQ2PSR_V_IDECMDEN
#define SEQ2PSR_V_SQ2TYPESEL	15
#define SEQ2PSR_M_SQ2TYPESEL	1 << SEQ2PSR_V_SQ2TYPESEL

/* 
**
**  Modular Clock Control Register  (MCLKCR) - Index 0x303
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     ATMODCLKEN	AT Modular Clock Enable
**        <3>     1     SYNCTMREN	Synchronous Timer Enable
**        <4>     1     TMRCLKDIS	Timer Clock Disable
**        <5>     1     SYSCLKDIS	ISA SYSCLK Disable
**
**	  <1:2><6:15> Reserved
*/
#define SEQ2_MCLKCR_REG		0x303
#define SEQ2_MCLKCR_INIT	0x0000
#define MCLKCR_V_ATMODCLKEN	0
#define MCLKCR_M_ATMODCLKEN	1 << MCLKCR_V_ATMODCLKEN
#define MCLKCR_V_SYNCTMREN	3
#define MCLKCR_M_SYNCTMREN	1 << MCLKCR_V_SYNCTMREN
#define MCLKCR_V_TMRCLKDIS	4
#define MCLKCR_M_TMRCLKDIS	1 << MCLKCR_V_TMRCLKDIS
#define MCLKCR_V_SYSCLKDIS	5
#define MCLKCR_M_SYSCLKDIS	1 << MCLKCR_V_SYSCLKDIS

/* 
**
**  Optional GPIO Control Register  (OGPIOCR) - Index 0x304
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**	       <0:7>   8     Reserved
**        <8>     4     GPIODATA	   General Purpose I/O Data [4:7]
**        <12>    4     GPIODIR		General Purpose I/O Direction [4:7]
**
*/
#define SEQ2_OGPIOCR_REG	0x304
#define SEQ2_OGPIOCR_INIT	   OGPIOCR_M_GPIODIR
#define OOGPIOCR_V_GPIODATA	8
#define OGPIOCR_M_GPIODATA		0xF << OGPIOCR_V_GPIODATA
#define OGPIOCR_V_GPIODATA4	8
#define OGPIOCR_M_GPIODATA4	1 << OGPIOCR_V_GPIODATA4
#define OGPIOCR_V_GPIODATA5	9
#define OGPIOCR_M_GPIODATA5	1 << OGPIOCR_V_GPIODATA5
#define OGPIOCR_V_GPIODATA6	10
#define OGPIOCR_M_GPIODATA6	1 << OGPIOCR_V_GPIODATA6
#define OGPIOCR_V_GPIODATA7	11
#define OGPIOCR_M_GPIODATA7	1 << OGPIOCR_V_GPIODATA7
#define OGPIOCR_V_GPIODIR		12
#define OGPIOCR_M_GPIODIR		0xF << OGPIOCR_V_GPIODIR
#define OGPIOCR_V_GPIODIR4		12
#define OGPIOCR_M_GPIODIR4		1 << OGPIOCR_V_GPIODIR4
#define OGPIOCR_V_GPIODIR5		13
#define OGPIOCR_M_GPIODIR5		1 << OGPIOCR_V_GPIODIR5
#define OGPIOCR_V_GPIODIR6		14
#define OGPIOCR_M_GPIODIR6		1 << OGPIOCR_V_GPIODIR6
#define OGPIOCR_V_GPIODIR7		15
#define OGPIOCR_M_GPIODIR7		1 << OGPIOCR_V_GPIODIR7

/* 
**
**  SEQUOIA-2 ID Register  (SEQ2IDR) - Index 0x310
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     4     S2ID		SEQUOIA-2 Revision ID Number
**
**	  <1:15> Reserved
*/
#define SEQ2_SEQ2IDR_REG	0x310
#define SEQ2IDR_V_S2ID		0
#define SEQ2IDR_M_S2ID		0xF << SEQ2IDR_V_S2ID

/* 
**
**  Miscellaneous DMA Control Register 1 (MDMACR1) - Index 0x330
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <8>     1     ENCDDMA		Encoded DMA Acknowledges
**
**	  <0:7><9:15> Reserved
*/
#define SEQ2_MDMACR1_REG	0x330
#define SEQ2_MDMACR1_INIT	0x0000
#define MDMACR1_V_ENCDDMA	8
#define MDMACR1_M_ENCDDMA	1 << MDMACR1_V_ENCDDMA

/* 
**
**  Miscellaneous DMA Control Register 2 (MDMACR2) - Index 0x331
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SEL3DMA		Select 3 DMA
**        <4>     1     DMA0CHSEL0	DMA Channel 0 Select 0
**        <5>     1     DMA0CHSEL1	DMA Channel 0 Select 1
**        <6>     1     DMA0CHSEL2	DMA Channel 0 Select 2
**        <8>     1     DMA1CHSEL0	DMA Channel 1 Select 0
**        <9>     1     DMA1CHSEL1	DMA Channel 1 Select 1
**        <10>    1     DMA1CHSEL2	DMA Channel 1 Select 2
**        <12>    1     DMA2CHSEL0	DMA Channel 2 Select 0
**        <13>    1     DMA2CHSEL1	DMA Channel 2 Select 1
**        <14>    1     DMA2CHSEL2	DMA Channel 2 Select 2
**
**	  <1:3><7><11><15> Reserved
*/
#define SEQ2_MDMACR2_REG	0x331
#define SEQ2_MDMACR2_INIT	0x0000
#define MDMACR2_V_SEL3DMA	0
#define MDMACR2_M_SEL3DMA	1 << MDMACR2_V_SEL3DMA
#define MDMACR2_V_DMA0CHSEL	4
#define MDMACR2_M_DMA0CHSEL	0x7 << MDMACR2_V_DMA0CHSEL
#define MDMACR2_V_DMA0CHSEL0	4
#define MDMACR2_M_DMA0CHSEL0	1 << MDMACR2_V_DMA0CHSEL0
#define MDMACR2_V_DMA0CHSEL1	5
#define MDMACR2_M_DMA0CHSEL1	1 << MDMACR2_V_DMA0CHSEL1
#define MDMACR2_V_DMA0CHSEL2	6
#define MDMACR2_M_DMA0CHSEL2	1 << MDMACR2_V_DMA0CHSEL2
#define MDMACR2_V_DMA1CHSEL	7
#define MDMACR2_M_DMA1CHSEL	0x7 << MDMACR2_V_DMA1CHSEL
#define MDMACR2_V_DMA1CHSEL0	7
#define MDMACR2_M_DMA1CHSEL0	1 << MDMACR2_V_DMA1CHSEL0
#define MDMACR2_V_DMA1CHSEL1	8
#define MDMACR2_M_DMA1CHSEL1	1 << MDMACR2_V_DMA1CHSEL1
#define MDMACR2_V_DMA1CHSEL2	9
#define MDMACR2_M_DMA1CHSEL2	1 << MDMACR2_V_DMA1CHSEL2
#define MDMACR2_V_DMA2CHSEL	12
#define MDMACR2_M_DMA2CHSEL	0x7 << MDMACR2_V_DMA2CHSEL
#define MDMACR2_V_DMA2CHSEL0	12
#define MDMACR2_M_DMA2CHSEL0	1 << MDMACR2_V_DMA2CHSEL0
#define MDMACR2_V_DMA2CHSEL1	13
#define MDMACR2_M_DMA2CHSEL1	1 << MDMACR2_V_DMA2CHSEL1
#define MDMACR2_V_DMA2CHSEL2	14
#define MDMACR2_M_DMA2CHSEL2	1 << MDMACR2_V_DMA2CHSEL2

/*
** DMA Channel 0 Select
*/
#define DMA0CHSEL_0		0x000
#define DMA0CHSEL_1		MDMACR2_M_DMA0CHSEL0
#define DMA0CHSEL_2		MDMACR2_M_DMA0CHSEL1
#define DMA0CHSEL_3		(MDMACR2_M_DMA0CHSEL0 | MDMACR2_M_DMA0CHSEL1)
#define DMA0CHSEL_5		(MDMACR2_M_DMA0CHSEL0 | MDMACR2_M_DMA0CHSEL2)
#define DMA0CHSEL_6		(MDMACR2_M_DMA0CHSEL1 | MDMACR2_M_DMA0CHSEL2)
#define DMA0CHSEL_7		(MDMACR2_M_DMA0CHSEL0 | MDMACR2_M_DMA0CHSEL1 | MDMACR2_M_DMA0CHSEL1)

/*
** DMA Channel 1 Select
*/
#define DMA1CHSEL_0		0x000
#define DMA1CHSEL_1		MDMACR2_M_DMA1CHSEL0
#define DMA1CHSEL_2		MDMACR2_M_DMA1CHSEL1
#define DMA1CHSEL_3		(MDMACR2_M_DMA1CHSEL0 | MDMACR2_M_DMA1CHSEL1)
#define DMA1CHSEL_5		(MDMACR2_M_DMA1CHSEL0 | MDMACR2_M_DMA1CHSEL2)
#define DMA1CHSEL_6		(MDMACR2_M_DMA1CHSEL1 | MDMACR2_M_DMA1CHSEL2)
#define DMA1CHSEL_7		(MDMACR2_M_DMA1CHSEL0 | MDMACR2_M_DMA1CHSEL1 | MDMACR2_M_DMA1CHSEL1)

/*
** DMA Channel 2 Select
*/
#define DMA2CHSEL_0		0x000
#define DMA2CHSEL_1		MDMACR2_M_DMA2CHSEL0
#define DMA2CHSEL_2		MDMACR2_M_DMA2CHSEL1
#define DMA2CHSEL_3		(MDMACR2_M_DMA2CHSEL0 | MDMACR2_M_DMA2CHSEL1)
#define DMA2CHSEL_5		(MDMACR2_M_DMA2CHSEL0 | MDMACR2_M_DMA2CHSEL2)
#define DMA2CHSEL_6		(MDMACR2_M_DMA2CHSEL1 | MDMACR2_M_DMA2CHSEL2)
#define DMA2CHSEL_7		(MDMACR2_M_DMA2CHSEL0 | MDMACR2_M_DMA2CHSEL1 | MDMACR2_M_DMA2CHSEL1)

/* 
**
**  Burst Bus Interrupt Control Register (BBICR) - Index 0x340
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     ENBINT		Enable Burst Bus Interrupt
**        <1>     1     BINTLOCKCLR	Burst Interrupt Interlock Clear
**        <2>     1     BINTLOCKDIS	Burst Interrupt Interlock Disable
**
**	  <3:15> Reserved
*/
#define SEQ2_BBICR_REG		0x340
#define SEQ2_BBICR_INIT		0x0000
#define BBICR_V_ENBINT		0
#define BBICR_M_ENBINT		1 << BBICR_V_ENBINT
#define BBICR_V_BINTLOCKCLR	1
#define BBICR_M_BINTLOCKCLR	1 << BBICR_V_BINTLOCKCLR
#define BBICR_V_BINTLOCKDIS	2
#define BBICR_M_BINTLOCKDIS	1 << BBICR_V_BINTLOCKDIS

/* 
**
**  Primary Activities IRQ Mask Register (PAIMR) - Index 0x350
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <1>     1     PAMSKIRQ1	IRQ1 Primary Activity Mask Enable 
**        <3>     1     PAMSKIRQ3	IRQ3 Primary Activity Mask Enable 
**        <4>     1     PAMSKIRQ4	IRQ4 Primary Activity Mask Enable 
**        <5>     1     PAMSKIRQ5	IRQ5 Primary Activity Mask Enable 
**        <6>     1     PAMSKIRQ6	IRQ6 Primary Activity Mask Enable 
**        <7>     1     PAMSKIRQ7	IRQ7 Primary Activity Mask Enable 
**        <8>     1     PAMSKIRQ8	IRQ8 Primary Activity Mask Enable 
**        <9>     1     PAMSKIRQ9	IRQ9 Primary Activity Mask Enable 
**        <10>    1     PAMSKIRQ10	IRQ10 Primary Activity Mask Enable 
**        <11>    1     PAMSKIRQ11	IRQ11 Primary Activity Mask Enable 
**        <12>    1     PAMSKIRQ12	IRQ12 Primary Activity Mask Enable 
**        <13>    1     PAMSKIRQ13	IRQ13 Primary Activity Mask Enable 
**        <14>    1     PAMSKIRQ14	IRQ14 Primary Activity Mask Enable 
**        <15>    1     PAMSKIRQ15	IRQ15 Primary Activity Mask Enable 
**
**	  <0><2> Reserved
*/
#define SEQ2_PAIMR_REG		0x350
#define SEQ2_PAIMR_INIT		( PAIMR_M_PAMSKIRQ1  | PAIMR_M_PAMSKIRQ3  | \
				  PAIMR_M_PAMSKIRQ4  | PAIMR_M_PAMSKIRQ5  | \
				  PAIMR_M_PAMSKIRQ6  | PAIMR_M_PAMSKIRQ7  | \
				  PAIMR_M_PAMSKIRQ8  | PAIMR_M_PAMSKIRQ9  | \
				  PAIMR_M_PAMSKIRQ10 | PAIMR_M_PAMSKIRQ11 | \
				  PAIMR_M_PAMSKIRQ12 | PAIMR_M_PAMSKIRQ13 | \
				  PAIMR_M_PAMSKIRQ14 )
#define PAIMR_V_PAMSKIRQ1	1
#define PAIMR_M_PAMSKIRQ1	1 << PAIMR_V_PAMSKIRQ1
#define PAIMR_V_PAMSKIRQ3	3
#define PAIMR_M_PAMSKIRQ3	1 << PAIMR_V_PAMSKIRQ3
#define PAIMR_V_PAMSKIRQ4	4
#define PAIMR_M_PAMSKIRQ4	1 << PAIMR_V_PAMSKIRQ4
#define PAIMR_V_PAMSKIRQ5	5
#define PAIMR_M_PAMSKIRQ5	1 << PAIMR_V_PAMSKIRQ5
#define PAIMR_V_PAMSKIRQ6	6
#define PAIMR_M_PAMSKIRQ6	1 << PAIMR_V_PAMSKIRQ6
#define PAIMR_V_PAMSKIRQ7	7
#define PAIMR_M_PAMSKIRQ7	1 << PAIMR_V_PAMSKIRQ7
#define PAIMR_V_PAMSKIRQ8	8
#define PAIMR_M_PAMSKIRQ8	1 << PAIMR_V_PAMSKIRQ8
#define PAIMR_V_PAMSKIRQ9	9                    
#define PAIMR_M_PAMSKIRQ9	1 << PAIMR_V_PAMSKIRQ9
#define PAIMR_V_PAMSKIRQ10	10
#define PAIMR_M_PAMSKIRQ10	1 << PAIMR_V_PAMSKIRQ10
#define PAIMR_V_PAMSKIRQ11	11
#define PAIMR_M_PAMSKIRQ11	1 << PAIMR_V_PAMSKIRQ11
#define PAIMR_V_PAMSKIRQ12	12
#define PAIMR_M_PAMSKIRQ12	1 << PAIMR_V_PAMSKIRQ12
#define PAIMR_V_PAMSKIRQ13	13
#define PAIMR_M_PAMSKIRQ13	1 << PAIMR_V_PAMSKIRQ13
#define PAIMR_V_PAMSKIRQ14	14
#define PAIMR_M_PAMSKIRQ14	1 << PAIMR_V_PAMSKIRQ14
#define PAIMR_V_PAMSKIRQ15	15
#define PAIMR_M_PAMSKIRQ15	1 << PAIMR_V_PAMSKIRQ15

/* 
**
**  PMI Trigger Source IRQ Active Register (PTSIAR) - Index 0x351
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     PMIDETBACTV	PMI Trigger Source DeTurbo Switch Active
**        <1>     1     PMIIRQ1ACTV	PMI Trigger Source IRQ1 Active
**        <3>     1     PMIIRQ3ACTV	PMI Trigger Source IRQ3 Active
**        <4>     1     PMIIRQ4ACTV	PMI Trigger Source IRQ4 Active
**        <5>     1     PMIIRQ5ACTV	PMI Trigger Source IRQ5 Active
**        <6>     1     PMIIRQ6ACTV	PMI Trigger Source IRQ6 Active
**        <7>     1     PMIIRQ7ACTV	PMI Trigger Source IRQ7 Active
**        <8>     1     PMIIRQ8ACTV	PMI Trigger Source IRQ8 Active
**        <9>     1     PMIIRQ9ACTV	PMI Trigger Source IRQ9 Active
**        <10>    1     PMIIRQ10ACTV	PMI Trigger Source IRQ10 Active
**        <11>    1     PMIIRQ11ACTV	PMI Trigger Source IRQ11 Active
**        <12>    1     PMIIRQ12ACTV	PMI Trigger Source IRQ12 Active
**        <13>    1     PMIIRQ13ACTV	PMI Trigger Source IRQ13 Active
**        <14>    1     PMIIRQ14ACTV	PMI Trigger Source IRQ14 Active
**        <15>    1     PMIIRQ15ACTV	PMI Trigger Source IRQ15 Active
**
**	  <2> Reserved
*/
#define SEQ2_PTSIAR_REG		0x351
#define SEQ2_PTSIAR_INIT	0x0000
#define PTSIAR_V_PMIDETBACTV	0
#define PTSIAR_M_PMIDETBACTV	1 << PTSIAR_V_PMIDETBACTV
#define PTSIAR_V_PMIIRQ1ACTV	1
#define PTSIAR_M_PMIIRQ1ACTV	1 << PTSIAR_V_PMIIRQ1ACTV
#define PTSIAR_V_PMIIRQ3ACTV	3
#define PTSIAR_M_PMIIRQ3ACTV	1 << PTSIAR_V_PMIIRQ3ACTV
#define PTSIAR_V_PMIIRQ4ACTV	4
#define PTSIAR_M_PMIIRQ4ACTV	1 << PTSIAR_V_PMIIRQ4ACTV
#define PTSIAR_V_PMIIRQ5ACTV	5
#define PTSIAR_M_PMIIRQ5ACTV	1 << PTSIAR_V_PMIIRQ5ACTV
#define PTSIAR_V_PMIIRQ6ACTV	6
#define PTSIAR_M_PMIIRQ6ACTV	1 << PTSIAR_V_PMIIRQ6ACTV
#define PTSIAR_V_PMIIRQ7ACTV	7                  
#define PTSIAR_M_PMIIRQ7ACTV	1 << PTSIAR_V_PMIIRQ7ACTV
#define PTSIAR_V_PMIIRQ8ACTV	8
#define PTSIAR_M_PMIIRQ8ACTV	1 << PTSIAR_V_PMIIRQ8ACTV
#define PTSIAR_V_PMIIRQ9ACTV	9
#define PTSIAR_M_PMIIRQ9ACTV	1 << PTSIAR_V_PMIIRQ9ACTV
#define PTSIAR_V_PMIIRQ10ACTV	10
#define PTSIAR_M_PMIIRQ10ACTV	1 << PTSIAR_V_PMIIRQ10ACTV
#define PTSIAR_V_PMIIRQ11ACTV	11
#define PTSIAR_M_PMIIRQ11ACTV	1 << PTSIAR_V_PMIIRQ11ACTV
#define PTSIAR_V_PMIIRQ12ACTV	12
#define PTSIAR_M_PMIIRQ12ACTV	1 << PTSIAR_V_PMIIRQ12ACTV
#define PTSIAR_V_PMIIRQ13ACTV	13
#define PTSIAR_M_PMIIRQ13ACTV	1 << PTSIAR_V_PMIIRQ13ACTV
#define PTSIAR_V_PMIIRQ14ACTV	14
#define PTSIAR_M_PMIIRQ14ACTV	1 << PTSIAR_V_PMIIRQ14ACTV
#define PTSIAR_V_PMIIRQ15ACTV	15
#define PTSIAR_M_PMIIRQ15ACTV	1 << PTSIAR_V_PMIIRQ15ACTV

/* 
**
**  PMI Trigger Source IRQ Mask Register (PTSIMR) - Index 0x352
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     IMSKDETURBO	DeTurbo Switch Triggering PMI Mask Enable 
**        <1>     1     IMSKIRQ1	IRQ1 Triggering PMI Mask Enable 
**        <3>     1     IMSKIRQ3	IRQ3 Triggering PMI Mask Enable 
**        <4>     1     IMSKIRQ4	IRQ4 Triggering PMI Mask Enable 
**        <5>     1     IMSKIRQ5	IRQ5 Triggering PMI Mask Enable 
**        <6>     1     IMSKIRQ6	IRQ6 Triggering PMI Mask Enable 
**        <7>     1     IMSKIRQ7	IRQ7 Triggering PMI Mask Enable 
**        <8>     1     IMSKIRQ8	IRQ8 Triggering PMI Mask Enable 
**        <9>     1     IMSKIRQ9	IRQ9 Triggering PMI Mask Enable 
**        <10>    1     IMSKIRQ10	IRQ10 Triggering PMI Mask Enable 
**        <11>    1     IMSKIRQ11	IRQ11 Triggering PMI Mask Enable 
**        <12>    1     IMSKIRQ12	IRQ12 Triggering PMI Mask Enable 
**        <13>    1     IMSKIRQ13	IRQ13 Triggering PMI Mask Enable 
**        <14>    1     IMSKIRQ14	IRQ14 Triggering PMI Mask Enable 
**        <15>    1     IMSKIRQ15	IRQ15 Triggering PMI Mask Enable 
**
**	  <2> Reserved
*/
#define SEQ2_PTSIMR_REG		0x352
#define SEQ2_PTSIMR_INIT	( PTSIMR_M_IMSKDETURBO | PTSIMR_M_IMSKIRQ1  | \
				  PTSIMR_M_IMSKIRQ3    | PTSIMR_M_IMSKIRQ4  | \
				  PTSIMR_M_IMSKIRQ5    | PTSIMR_M_IMSKIRQ6  | \
				  PTSIMR_M_IMSKIRQ7    | PTSIMR_M_IMSKIRQ8  | \
				  PTSIMR_M_IMSKIRQ9    | PTSIMR_M_IMSKIRQ10 | \
				  PTSIMR_M_IMSKIRQ11   | PTSIMR_M_IMSKIRQ12 | \
				  PTSIMR_M_IMSKIRQ13   | PTSIMR_M_IMSKIRQ14 | \
				  PTSIMR_M_IMSKIRQ15   )
#define PTSIMR_V_IMSKDETURBO	0
#define PTSIMR_M_IMSKDETURBO	1 << PTSIMR_V_IMSKDETURBO
#define PTSIMR_V_IMSKIRQ1	1
#define PTSIMR_M_IMSKIRQ1	1 << PTSIMR_V_IMSKIRQ1
#define PTSIMR_V_IMSKIRQ3	3
#define PTSIMR_M_IMSKIRQ3	1 << PTSIMR_V_IMSKIRQ3
#define PTSIMR_V_IMSKIRQ4	4
#define PTSIMR_M_IMSKIRQ4	1 << PTSIMR_V_IMSKIRQ4
#define PTSIMR_V_IMSKIRQ5	5
#define PTSIMR_M_IMSKIRQ5	1 << PTSIMR_V_IMSKIRQ5
#define PTSIMR_V_IMSKIRQ6	6
#define PTSIMR_M_IMSKIRQ6	1 << PTSIMR_V_IMSKIRQ6
#define PTSIMR_V_IMSKIRQ7	7
#define PTSIMR_M_IMSKIRQ7	1 << PTSIMR_V_IMSKIRQ7
#define PTSIMR_V_IMSKIRQ8	8
#define PTSIMR_M_IMSKIRQ8	1 << PTSIMR_V_IMSKIRQ8
#define PTSIMR_V_IMSKIRQ9	9                    
#define PTSIMR_M_IMSKIRQ9	1 << PTSIMR_V_IMSKIRQ9
#define PTSIMR_V_IMSKIRQ10	10
#define PTSIMR_M_IMSKIRQ10	1 << PTSIMR_V_IMSKIRQ10
#define PTSIMR_V_IMSKIRQ11	11
#define PTSIMR_M_IMSKIRQ11	1 << PTSIMR_V_IMSKIRQ11
#define PTSIMR_V_IMSKIRQ12	12
#define PTSIMR_M_IMSKIRQ12	1 << PTSIMR_V_IMSKIRQ12
#define PTSIMR_V_IMSKIRQ13	13
#define PTSIMR_M_IMSKIRQ13	1 << PTSIMR_V_IMSKIRQ13
#define PTSIMR_V_IMSKIRQ14	14
#define PTSIMR_M_IMSKIRQ14	1 << PTSIMR_V_IMSKIRQ14
#define PTSIMR_V_IMSKIRQ15	15
#define PTSIMR_M_IMSKIRQ15	1 << PTSIMR_V_IMSKIRQ15

/* 
**
**  IRQ Secondary Activity Enable Register (ISAER) - Index 0x353
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     SAMSKIRQ	Secondary Activity Mask for IRQs
**
**	  <1:15> Reserved
*/
#define SEQ2_ISAER_REG		0x353
#define SEQ2_ISAER_INIT		ISAER_M_SAMSKIRQ
#define ISAER_V_SAMSKIRQ	0
#define ISAER_M_SAMSKIRQ	1 << ISAER_V_SAMSKIRQ

/*
** Shadow Read Registers
**
*/

#define SR_M_DATAMSK		0xF

/* 
** 8254 Counter Registers
*/
#define SRR_54CTR0_REG1		0x500
#define SRR_54CTR0_REG2		0x501
#define SRR_54CTR0_REG3		0x506
#define SRR_54CTR1_REG1		0x502
#define SRR_54CTR1_REG2		0x503
#define SRR_54CTR1_REG3		0x507
#define SRR_54CTR2_REG1		0x504
#define SRR_54CTR2_REG2		0x505
#define SRR_54CTR2_REG3		0x508

/* 
** 8237 DMA Controller Registers
*/
#define SRR_37CTRL0_REG1	0x510
#define SRR_37CTRL0_REG2	0x511
#define SRR_37CTRL0_REG3	0x512
#define SRR_37CTRL1_REG4	0x513
#define SRR_37CTRL1_REG5	0x514
#define SRR_37CTRL1_REG6	0x515
#define SRR_37CTRL2_REG7	0x516
#define SRR_37CTRL2_REG8	0x517

/* 
** 8259 Interrupt Controller Registers
*/
#define SRR_59CTRL0_REG1	0x520
#define SRR_59CTRL0_REG2	0x521
#define SRR_59CTRL0_REG3	0x522
#define SRR_59CTRL1_REG4	0x523
#define SRR_59CTRL1_REG5	0x524
#define SRR_59CTRL1_REG6	0x525
#define SRR_59CTRL2_REG7	0x526
#define SRR_59CTRL2_REG8    	0x527
#define SRR_59CTRL2_REG9    	0x528
#define SRR_59CTRL2_REG10	0x529
#define SRR_59CTRL2_REG11	0x52A
#define SRR_59CTRL2_REG12	0x52B

/*
** IBM-AT Standard Registers
**
*/

/* 
**
**  Register 1 (REG1) - Access Address 0x061
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     TMR2EN		Timer 2 Enable
**        <1>     1     SPKREN		Speaker Enable
**        <2>     1     PARDIS		Parity Disable
**        <3>     1     IOCHKDIS	I/O Channel Check Disable
**        <4>     1     REFRTGL		Refresh Toggle
**        <5>     1     TMR2OUT		Timer 2 Output
**        <6>     1     IOCKERR		I/O Channel Check Error
**        <7>     1     PARERR		Parity Error
**
*/
#define ATSR_REG1_REG		0x061
#define ATSR_REG1_INIT		REG1_M_PARDIS
#define REG1_V_TMR2EN		0
#define REG1_M_TMR2EN		1 << REG1_V_TMR2EN
#define REG1_V_SPKREN		1
#define REG1_M_SPKREN		1 << REG1_V_SPKREN
#define REG1_V_PARDIS		2
#define REG1_M_PARDIS		1 << REG1_V_PARDIS
#define REG1_V_IOCHKDIS		3
#define REG1_M_IOCHKDIS		1 << REG1_V_IOCHKDIS
#define REG1_V_REFRTGL		4
#define REG1_M_REFRTGL		1 << REG1_V_REFRTGL
#define REG1_V_TMR2OUT		5
#define REG1_M_TMR2OUT		1 << REG1_V_TMR2OUT
#define REG1_V_IOCKERR		6
#define REG1_M_IOCKERR		1 << REG1_V_IOCKERR
#define REG1_V_PARERR		7
#define REG1_M_PARERR		1 << REG1_V_PARERR

/* 
**
**  Register 2 (REG2) - Access Address 0x070
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     6     RTCINDX		Real Timer Clock Index
**        <7>     1     NMIDIS		NMI Mask Disable 
**
*/
#define ATSR_REG2_REG		0x070
#define ATSR_REG2_INIT		REG2_M_NMIDIS
#define REG2_V_RTCINDX		0
#define REG2_M_RTCINDX		0x7F << REG2_V_RTCINDX
#define REG2_V_NMIDIS		7
#define REG2_M_NMIDIS		1 << REG2_V_NMIDIS

/* 
**
**  Register 3 (REG3) - Access Address 0x092
**
**        Loc   Size    Name    	Function
**       -----  ----    ----    	---------------------------------
**        <0>     1     FSTRST		Alternate Fast CPU Reset
**        <1>     1     FGATEA20	Fast GATEA20
**        <3>     1     SCLOCK1		Security Lock 1
**
**	  <2><4:7> Reserved
*/
#define ATSR_REG3_REG		0x092
#define ATSR_REG3_INIT		0x0000
#define REG3_V_FSTRST		0
#define REG3_M_FSTRST		1 << REG3_V_FSTRST
#define REG3_V_FGATEA20		1
#define REG3_M_FGATEA20		1 << REG3_V_FGATEA20
#define REG3_V_SCLOCK1		3
#define REG3_M_SCLOCK1		1 << REG3_V_SCLOCK1






#ifndef __LANGUAGE_ASM__
#ifndef _LOCORE

void sequoiaInit(void);
void sequoiaWrite __P((int reg,u_int16_t value));     
void sequoiaRead  __P((int reg,u_int16_t * value_ptr));     

/* x console functions */
void    consXTvOn   __P((void));
void    consXTvOff  __P((void));


/* smart card reader functions */
int     scrGetDetect             __P((void));
void    scrSetPower              __P((int value));
void    scrSetClock              __P((int value));
void    scrSetReset              __P((int value));
void    scrSetDataHighZ          __P((void));
void    scrSetData               __P((int value));   
int     scrGetData               __P((void));	


/* just used to debug scr - remove when done - ejg */		
void    scrToggleTestPin		   __P((void));
void sequoiaOneAccess(void);


/* biled functions */
void ledNetActive   __P((void));
void ledNetBlock    __P((void));
void ledNetUnblock  __P((void));
void ledPanic       __P((void));


/* function to get the hw rev */
int  hwGetRev       __P((void));

/* debug led functions */
#define LED_DEBUG_STATE_0           0
#define LED_DEBUG_STATE_1           1
#define LED_DEBUG_STATE_2           2
#define LED_DEBUG_STATE_3           3

#define LED_DEBUG_YELLOW_OFF        4
#define LED_DEBUG_YELLOW_ON         5

#define LED_DEBUG_GREEN_OFF         6
#define LED_DEBUG_GREEN_ON          7


void ledSetDebug    __P((int command));





#endif /* _LOCORE */
#endif /* __LANGUAGE_ASM__ */

#endif /* SEQUOIAH */








