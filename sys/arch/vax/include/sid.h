/*	$NetBSD: sid.h,v 1.21.190.1 2017/08/28 17:51:54 skrll Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 * Board-Type (?_BTYP_?) and Sub-Type (?_STYP_?) are synonima.
 * Michael Kukat changed this 01/27/2001, STYP is really a subtype now.
 * other synonima are:
 */
#define cpudata	    vax_cpudata
#define cputype	    vax_cputype
#define cpusubtype  vax_boardtype

/*
 * Chip CPU types / chip CPU Subtypes
 *
 * The type of a VAX is given by the high-order byte of the System
 * identification register (SID) and describes families or series of VAXen.
 * Board-Types/Sub-Types within series are described by the SIED register.
 */

/*
 * 700 series (1977)
 */
#define VAX_TYP_780	1	/* VAX-11/780, 785, 782 */
#define VAX_TYP_750	2	/* VAX-11/750 */
#define VAX_TYP_730	3	/* VAX-11/730, 725 */
#define VAX_TYP_790	4	/* VAX 8600, 8650 */
 
#define VAX_BTYP_780	0x01000000	/* generic 11/780 */
#define VAX_BTYP_750	0x02000000	/* generic 11/750 */
#define VAX_BTYP_730	0x03000000	/* generic 11/730 */
#define VAX_BTYP_790	0x04000000	/* generic 11/790 */

/*
 * 8000 series (1986)
 */
#define VAX_TYP_8SS	5	/* VAX 8200, 8300, 8250, 8350, VS 8000 */
#define VAX_TYP_8NN	6	/* VAX 8530, 8550, 8700, 8800 */
 
#define VAX_BTYP_8000	0x05000000	/* generic VAX 8000 */
 
#define VAX_BTYP_8800	0x06000000	/* generic Nautilus */
#define VAX_BTYP_8700	0x06000001
#define VAX_BTYP_8550	0x06000006
#define VAX_BTYP_8500	0x06000007

/*
 * MicroVAX I (1984)
 */
#define VAX_TYP_UV1	7	/* MicroVAX I, VAXstation I */
     /* VAX_TYP_610	7 */
 
#define VAX_BTYP_610	0x07000000	/* generic MicroVAX-I */
 
/*
 * MicroVAX II series (1985)
 */
#define VAX_TYP_UV2	8
     /* VAX_TYP_78032	8 */
 
#define VAX_BTYP_630	0x08000001	/* MicroVAX II, VAXstation II */
#define VAX_BTYP_410	0x08000004	/* MicroVAX 2000, VAXstation 2000 */
 
/*
 * CVAX chip series (1987)
 */
#define VAX_TYP_CVAX	10
     /* VAX_TYP_650	10 */

 
#define VAX_BTYP_650	0x0A000001	/* MicroVAX 3500, 3600 */
     /* VAX_BTYP_65D	0x0A000001	   VAXstation 3200, 3500 XXX */
     /* VAX_BTYP_640	0x0A000001	   MicroVAX 3300, 3400 XXX */
     /* VAX_BTYP_655	0x0A000001	   MicroVAX 3800, 3900 XXX */
#define VAX_BTYP_9CC	0x0A000002	/* VAX 6000 model 210/310 */
#define VAX_BTYP_60	0x0A000003	/* VAXstation 3520, 3540 */
#define VAX_BTYP_420	0x0A000004	/* VAXstation 3100 models 10 - 48 */
#define	VAX_BTYP_IS1	0x0A000006	/* Infoserver 1000 */
#define VAX_BTYP_510	0x0A000007	/* VAXft model 110 */
     /* VAX_BTYP_520	0x0A000007	   VAXft model 310 */

/*
 * SID Extension register definitions for CVAX series
 */
#define	VAX_SIE_KA640	0x2		/* KA640 MicroVAX 3300, 3400 */
#define	VAX_SIE_KA650	0x1		/* KA650 MicroVAX 3500, 3600 */
#define	VAX_SIE_KA655	0x3		/* KA655 MicroVAX 3800, 3900 */
 
/*
 * Rigel chip series (1990)
 */
#define VAX_TYP_RIGEL	11
     /* VAX_TYP_9RR	11 */
 
#define VAX_BTYP_670	0x0B000001	/* VAX 4000 model 300 */
#define VAX_BTYP_9RR	0x0B000002	/* VAX 6000 model 410-460 */
#define VAX_BTYP_43	0x0B000004	/* VAXstation 3100 model 76 */
 
/*
 * Aquarius series (1990)
 */
#define VAX_TYP_9000	14
 
#define VAX_BTYP_9AR	0x0E00000?	/* VAX 9000 models 210, 410-440 */
#define VAX_BTYP_9AQ	0x0E00000?	/* VAX 9000 models 400-800 */
 
/*
 * Polarstar series (1988)
 */
#define VAX_TYP_8PS	17
 
#define VAX_BTYP_8PS	0x11000000	/* VAX 8810 to 8840 */
 
/*
 * Mariah chip series (1991)
 */
#define VAX_TYP_MARIAH	18
#define VAX_TYP_V12	18
 
#define VAX_BTYP_1202	0x12000002	/* VAX 6000 model 510-560 */
#define VAX_BTYP_46	0x12000004	/* VAXstation 4000/60, 3100/80 */
 
/*
 * NVAX chip series (1991)
 */
#define VAX_TYP_NVAX	19
#define VAX_TYP_V13	19
 
#define VAX_BTYP_680	0x13000001	/* VAX 4000 model [45]00 */
#define	VAX_STYP_675	0x0c		/* VAX 4000 model 400 */
#define	VAX_STYP_680	0x06		/* VAX 4000 model 500 */
#define	VAX_STYP_690	0x07		/* VAX 4000 model 600 */
#define VAX_BTYP_1302	0x13000002
#define VAX_BTYP_53	0x13000003	/* VAX 4000 model 105x, MV 3100/9x */
#define	VAX_STYP_51	0x09		/* MicroVAX 3100 model 90 / 95 */
#define	VAX_STYP_52	0x0a		/* VAX 4000 model 100 */
#define	VAX_STYP_53	0x0b		/* VAX 4000 model 10[568] */
#define	VAX_STYP_55	0x08		/* MicroVAX 3100 model 85 */
#define VAX_BTYP_49	0x13000004	/* VAXstation 4000 model 90 */
#define	VAX_BTYP_681	0x13000005	/* VAX 4000 model 500A/705A */
#define	VAX_STYP_681	0x0e		/* VAX 4000 model 500A */
#define	VAX_STYP_691	0x0f		/* VAX 4000 model 605A */
#define	VAX_STYP_694	0x10		/* VAX 4000 model 705A */

/*
 * SOC chip series (1991)
 */
#define VAX_TYP_SOC	20
#define VAX_TYP_V14	20
 
#define VAX_BTYP_660	0x14000001	/* VAX 4000 model 200 */
#define VAX_BTYP_48	0x14000004	/* VAXstation 4000 VLC */
#define VAX_BTYP_550	0x14000007	/* VAXft model 410, 610 */
#define	VAX_BTYP_VXT	0x14000008	/* VXT 2000 */
 
/*
 * NVAX+ chip series (1991)
 */
#define VAX_TYP_NVPLUS	23
#define VAX_TYP_V17	23
 
#define VAX_BTYP_1701	0x17000001
 
/*
 * Highest Number supported by NetBSD/VAX
 */
#define VAX_TYP_MAX	VAX_TYP_NVAX

/*
 * compatibility with old names:
 */
 
#define VAX_780		VAX_TYP_780
#define VAX_750		VAX_TYP_750
#define VAX_730		VAX_TYP_730
#define VAX_8600	VAX_TYP_790
#define VAX_8200	VAX_TYP_8SS
#define VAX_8800	VAX_TYP_8NN
#define VAX_610		VAX_TYP_UV1
#define VAX_78032	VAX_TYP_UV2
#define VAX_650		VAX_TYP_CVAX
 
/*
 * Some common-used external variables.
 */
extern	int vax_cputype;	/* general, highest byte of the SID-register */
extern	int vax_cpudata;	/* general, the contents of the SID-register */
extern	int vax_siedata;	/* contents of the SIE register */
extern	int vax_bustype;	/* HW-dep., setup at consinit() in ka???.c */
extern	int vax_boardtype;	/* HW-dep., msb of SID | SIE (SID-extension) */
extern	int vax_confdata;	/* HW-dep., hardware dependent config-data   */

