/*	$NetBSD: intrcnt.h,v 1.3.26.1 2001/10/01 12:38:02 fvdl Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* interrupt counters */
GLOBAL(intrnames)
	.asciz	"spur"
	.asciz	"clock"
	.asciz	"kbd/mouse"
	.asciz	"fdc/acsi"
	.asciz	"soft"
	.asciz	"5380-SCSI"
	.asciz	"5380-DMA"
	.asciz	"nmi"
	.asciz	"8530-SCC"
	.asciz	"statclock"
	.asciz	"printer"

	.asciz	"spurious"
	.asciz	"autovec1"
	.asciz	"5380-DRQ"
	.asciz	"autovec3"
	.asciz	"autovec4"
	.asciz	"autovec5"
	.asciz	"autovec6"
	.asciz	"autovec7"
	.asciz	"uservec1"	| 0x08
	.asciz	"uservec2"
	.asciz	"uservec3"
	.asciz	"ISA1"
	.asciz	"uservec5"
	.asciz	"uservec6"
	.asciz	"uservec7"
	.asciz	"ST-DMA/IDE"
	.asciz	"uservec9"
	.asciz	"MFP1_XMT_Err"
	.asciz	"MFP1_XMT_Buf_empty"
	.asciz	"MFP1_RCV_Err"
	.asciz	"MFP1_RCV_Buf_full"
	.asciz	"hardclock"
	.asciz	"uservec15"
	.asciz	"ISA2"
	.asciz	"PCI0"
	.asciz	"PCI1"
	.asciz	"PCI2"
	.asciz	"uservec20"
	.asciz	"uservec21"
	.asciz	"uservec22"
	.asciz	"Hades-floppy"
	.asciz	"PCI3"
	.asciz	"uservec25"
	.asciz	"uservec26"
	.asciz	"uservec27"
	.asciz	"uservec28"
	.asciz	"uservec29"
	.asciz	"uservec30"
	.asciz	"uservec31"
	.asciz	"uservec32"
	.asciz	"uservec33"
	.asciz	"uservec34"
	.asciz	"uservec35"
	.asciz	"uservec36"
	.asciz	"uservec37"
	.asciz	"uservec38"
	.asciz	"uservec39"
	.asciz	"uservec40"
	.asciz	"uservec41"
	.asciz	"uservec42"
	.asciz	"uservec43"
	.asciz	"uservec44"
	.asciz	"uservec45"
	.asciz	"uservec46"
	.asciz	"uservec47"
	.asciz	"uservec48"
	.asciz	"uservec49"
	.asciz	"uservec50"
	.asciz	"uservec51"
	.asciz	"uservec52"
	.asciz	"uservec53"
	.asciz	"uservec54"
	.asciz	"uservec55"
	.asciz	"uservec56"
	.asciz	"uservec57"
	.asciz	"uservec58"
	.asciz	"uservec59"
	.asciz	"uservec60"
	.asciz	"uservec61"
	.asciz	"uservec62"
	.asciz	"uservec63"
	.asciz	"uservec64"
	.asciz	"uservec65"
	.asciz	"uservec66"
	.asciz	"uservec67"
	.asciz	"uservec68"
	.asciz	"uservec69"
	.asciz	"uservec70"
	.asciz	"uservec71"
	.asciz	"uservec72"
	.asciz	"uservec73"
	.asciz	"uservec74"
	.asciz	"uservec75"
	.asciz	"uservec76"
	.asciz	"uservec77"
	.asciz	"uservec78"
	.asciz	"uservec79"
	.asciz	"uservec80"
	.asciz	"uservec81"
	.asciz	"uservec82"
	.asciz	"uservec83"
	.asciz	"uservec84"
	.asciz	"uservec85"
	.asciz	"uservec86"
	.asciz	"uservec87"
	.asciz	"uservec88"
	.asciz	"uservec89"
	.asciz	"uservec90"
	.asciz	"uservec91"
	.asciz	"uservec92"
	.asciz	"uservec93"
	.asciz	"uservec94"
	.asciz	"uservec95"
	.asciz	"uservec96"
	.asciz	"uservec97"
	.asciz	"uservec98"
	.asciz	"uservec99"
	.asciz	"uservec100"
	.asciz	"uservec101"
	.asciz	"uservec102"
	.asciz	"uservec103"
	.asciz	"uservec104"
	.asciz	"uservec105"
	.asciz	"uservec106"
	.asciz	"uservec107"
	.asciz	"uservec108"
	.asciz	"uservec109"
	.asciz	"uservec110"
	.asciz	"uservec111"
	.asciz	"uservec112"
	.asciz	"uservec113"
	.asciz	"uservec114"
	.asciz	"uservec115"
	.asciz	"uservec116"
	.asciz	"uservec117"
	.asciz	"uservec118"
	.asciz	"uservec119"
	.asciz	"uservec120"
	.asciz	"uservec121"
	.asciz	"uservec122"
	.asciz	"uservec123"
	.asciz	"uservec124"
	.asciz	"uservec125"
	.asciz	"uservec126"
	.asciz	"uservec127"
	.asciz	"uservec128"
	.asciz	"uservec129"
	.asciz	"uservec130"
	.asciz	"uservec131"
	.asciz	"uservec132"
	.asciz	"uservec133"
	.asciz	"uservec134"
	.asciz	"uservec135"
	.asciz	"uservec136"
	.asciz	"uservec137"
	.asciz	"uservec138"
	.asciz	"uservec139"
	.asciz	"uservec140"
	.asciz	"uservec141"
	.asciz	"uservec142"
	.asciz	"uservec143"
	.asciz	"uservec144"
	.asciz	"uservec145"
	.asciz	"uservec146"
	.asciz	"uservec147"
	.asciz	"uservec148"
	.asciz	"uservec149"
	.asciz	"uservec150"
	.asciz	"uservec151"
	.asciz	"uservec152"
	.asciz	"uservec153"
	.asciz	"uservec154"
	.asciz	"uservec155"
	.asciz	"uservec156"
	.asciz	"uservec157"
	.asciz	"uservec158"
	.asciz	"uservec159"
	.asciz	"uservec160"
	.asciz	"uservec161"
	.asciz	"uservec162"
	.asciz	"uservec163"
	.asciz	"uservec164"
	.asciz	"uservec165"
	.asciz	"uservec166"
	.asciz	"uservec167"
	.asciz	"uservec168"
	.asciz	"uservec169"
	.asciz	"uservec170"
	.asciz	"uservec171"
	.asciz	"uservec172"
	.asciz	"uservec173"
	.asciz	"uservec174"
	.asciz	"uservec175"
	.asciz	"uservec176"
	.asciz	"uservec177"
	.asciz	"uservec178"
	.asciz	"uservec179"
	.asciz	"uservec180"
	.asciz	"uservec181"
	.asciz	"uservec182"
	.asciz	"uservec183"
	.asciz	"uservec184"
	.asciz	"uservec185"
	.asciz	"uservec186"
	.asciz	"uservec187"
	.asciz	"uservec188"
	.asciz	"uservec189"
	.asciz	"uservec190"
	.asciz	"uservec191"
	.asciz	"uservec192"

GLOBAL(eintrnames)
	.even
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0
	.long	0		| spurious
GLOBAL(intrcnt_auto)
	.long	0,0,0,0,0,0,0	| auto-vectors
GLOBAL(intrcnt_user)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
GLOBAL(intrcnt_special)
GLOBAL(eintrcnt)
