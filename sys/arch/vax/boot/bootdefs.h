/*	$NetBSD: bootdefs.h,v 1.1 1995/03/29 21:24:04 ragge Exp $ */
/*
 * Copyright (c) 1995 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by
 * Bertram Barth.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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

 /* All bugs are subject to removal without further notice */
		


/* 
 * from <sys/disklabel.h> 
 */
#define LABELSECTOR     0                       /* sector containing label */
#define LABELOFFSET     64                      /* offset of label in sector */
#define LABELSTART	LABELOFFSET
#define LABELSIZE	276			/* found in disklabel.h */
#define LABELEND	(LABELSTART+LABELSIZE)
#ifndef DISKMAGIC
#define DISKMAGIC	0x82564557		/* The disk magic number */
#endif

/*
 * some uVAX specific constants
 */
#define IAREA_SIZE	(8*1 + 4*4)	/* 8 bytes, 4 longs */
#define IAREA_OFFSET	(LABELEND/2)	/* offset in words !!! */
#define IAREASTART	IAREA_OFFSET
#define IAREAEND	(IAREASTART+IAREASIZE)

#define VOLINFO		0	/* 1=single-sided  81=double-sided volumes */
#define SISIZE		20	/* size in blocks of secondary image */
#define SILOAD		0	/* load offset (usually 0) from the default */
#define SIOFF		0x0A	/* byte offset into secondary image */

#define LOADSIZE	40

/*
 * Processor Register Summary  (KA630 UG, 4-4)
 *
 * Categories:
 *   1: VAX processor registers implemented as described in the
 *	"VAX Architecture Reference Manual".
 *   2: VAX processor register implemented external to the MicroVAX
 * 	CPU Chip by the KA630-AA logic.
 *   3: Processor registers read as 0, no operation (NOP) on write.
 *   4: Processor registers implemented by MicroVAX CPU chip uniquely
 *	(that is, registers not described in the "VAX Arch. Ref. Man.").
 *   5: Processor register access not allowed. Attempted access results
 *      in reserved operand fault.
 *   R: An 'R' following the category number indicates that the register
 *	is cleared by power-up and by the negation of DC OK.
 */

/*
 * 	Mnemonic	Number		Type, Category, Register Name
 *----------------------------------------------------------------------*/
#define PR_KSP		 0	/* R/W, 1,  Kernel Stack Pointer */
#define PR_ESP		 1 	/* R/W, 1,  Executive Stack Pointer */
#define PR_SSP		 2 	/* R/W, 1,  Supervisor Stack Pointer */
#define PR_USP		 3 	/* R/W, 1,  User Stack Pointer */
#define PR_ISP		 4	/* R/W, 1,  Interrupt Stack Pointer */
#define PR_RESERVED_05	 5	/*      5 	*/
#define PR_RESERVED_06	 6	/*      5 	*/
#define PR_RESERVER_07	 7	/*      5 	*/

#define PR_P0BR		 8 	/* R/W, 1,  P0 Base Register */
#define PR_P0LR		 9	/* R/W, 1,  P0 Length Register */
#define PR_P1BR		10 	/* R/W, 1,  P1 Base Register */
#define PR_P1LR		11	/* R/W, 1,  P1 Length Register */
#define PR_SBR		12	/* R/W, 1,  System Base Register */
#define PR_SLR		13	/* R/W, 1,  System Length Register */
#define PR_RESERVED_14	14	/*      5 	*/
#define PR_RESERVED_15	15	/*      5 	*/

#define PR_PCBB		16	/* R/W, 1,  Process Control Block Base */
#define PR_SCBB		17	/* R/W, 1,  System Control Block Base */
#define PR_IPL		18	/* R/W, 1R, Interrupt Priority Level */
#define PR_ASTLVL	19	/* R/W, 1R, AST Level */
#define PR_SIRR		20 	/* W,   1,  Software Interrupt Request */
#define PR_SISR		21	/* R/W, 1R, Software Interrupt Summary */
#define PR_IPIR		22	/* R/W, 5,  Interprocessor Interrupt */
#define PR_CMIERR	23	/* R/W, 5,  CMI Error Register */

#define PR_ICCS		24	/* R/W, 4R, Interval Clock Control/Status */
#define PR_NICR		25	/* W,   3,  Next Interval Count Register */
#define PR_ICR		26	/* R,   3,  Interval Count Register */
#define PR_TODR		27	/* R/W, 3,  TOY Register */
#define PR_CSRS		28	/* R/W, 3,  Console Storage Receiver Status */
#define PR_CSRD		29	/* R,   3,  Console Storage Receiver Data */
#define PR_CSTS		30	/* R/W, 3,  Console Storage Transmit Status */
#define PR_CSTD		31	/* W,   3,  Console Storage Transmit Data */

#define PR_RXCS		32	/* R/W, 2R, Console Receiver Control/Status */
#define PR_RXDB		33	/* R,   2R, Console Receiver Data Buffer */
#define PR_TXCS		34	/* R/W, 2R, Console Transmit Control/Status */
#define PR_TXDB		35	/* W,   2R, Console Transmit Data Buffer */
#define PR_TBDR		36	/* R/W, 3,  Translation Buffer Disable */
#define PR_CADR		37	/* R/W, 3,  Cache Disable Register */
#define PR_MCESR	38	/* R/W, 3,  Machine Check Error Summary */
#define PR_CAER		39	/* R/W, 3,  Cache Error Register */

#define PR_ACCS		40	/* R/W, 5,  Accelerator Control/Status */
#define PR_SAVISP	41	/* R/W, 4,  Console Saved ISP */
#define PR_SAVPC	42	/* R/W, 4,  Console Saved PC */
#define PR_SAVPSL	43	/* R/W, 4,  Console Saved PSL */
#define PR_WCSA		44	/* R/W, 5,  WCS Address */
#define PR_WCSB		45	/* R/W, 5,  WCS Data */
#define PR_RESERVED_46	46	/*      5 	*/
#define PR_RESERVED_47	47	/*      5 	*/

#define PR_SBIFS	48	/* R/W, 3,  SBI Fault/Status */
#define PR_SBIS		49	/* R,   3,  SBI Silo */
#define PR_SBISC	50	/* R/W, 3,  SBI Silo Comparator */
#define PR_SBIMT	51	/* R/W, 3,  SBI Silo Maintenance */
#define PR_SBIER	52	/* R/W, 3,  SBI Error Register */
#define PR_SBITA	53	/* R,   3,  SBI Timeout Address Register */
#define PR_SBIQC	54	/* W,   3,  SBI Quadword Clear */
#define PR_IORESET	55	/* W,   2,  I/O Bus Reset */

#define PR_MAPEN	56	/* R/W, 1R, Memory Management Enable */
#define PR_TBIA		57	/* W,   1,  TB Invalidate All */
#define PR_TBIS		58	/* W,   1,  TB Invalidate Single */
#define PR_TBDATA	59	/* R/W, 3,  Translation Buffer Data */
#define PR_MBRK		60	/* R/W, 3,  Microprogram Break */
#define PR_PMR		61	/* R/W, 3,  Performance Monitor Enable */
#define PR_SID		62	/* R,   1,  System Identification */
#define PR_TBCHK	63	/* W,   1,  Translation Buffer Check */

/* 64 -- 127		reserved (5) */

/*
 * EOF
 */
