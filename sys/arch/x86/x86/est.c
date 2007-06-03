/*	$NetBSD: est.c,v 1.1 2007/06/03 19:32:26 xtraeme Exp $	*/
/*
 * Copyright (c) 2003 Michael Eriksson.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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

/*
 * This is a driver for Intel's Enhanced SpeedStep Technology (EST),
 * as implemented in Pentium M processors.
 *
 * Reference documentation:
 *
 * - IA-32 Intel Architecture Software Developer's Manual, Volume 3:
 *   System Programming Guide.
 *   Section 13.14, Enhanced Intel SpeedStep technology.
 *   Table B-2, MSRs in Pentium M Processors.
 *   http://www.intel.com/design/pentium4/manuals/253668.htm
 *
 * - Intel Pentium M Processor Datasheet.
 *   Table 5, Voltage and Current Specifications.
 *   http://www.intel.com/design/mobile/datashts/252612.htm
 *
 * - Intel Pentium M Processor on 90 nm Process with 2-MB L2 Cache Datasheet
 *   Table 3-4, 3-5, 3-6, Voltage and Current Specifications.
 *   http://www.intel.com/design/mobile/datashts/302189.htm
 *
 * - Linux cpufreq patches, speedstep-centrino.c.
 *   Encoding of MSR_PERF_CTL and MSR_PERF_STATUS.
 *   http://www.codemonkey.org.uk/projects/cpufreq/cpufreq-2.4.22-pre6-1.gz
 *
 *   ACPI objects: _PCT is MSR location, _PSS is freq/voltage, _PPC is caps.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: est.c,v 1.1 2007/06/03 19:32:26 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/once.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/cpu_msr.h>

#include <machine/cpu.h>
#include <machine/specialreg.h>

#include "opt_est.h"
#ifdef EST_FREQ_USERWRITE
#define	EST_TARGET_CTLFLAG	(CTLFLAG_READWRITE | CTLFLAG_ANYWRITE)
#else
#define	EST_TARGET_CTLFLAG	CTLFLAG_READWRITE
#endif

/* Convert MHz and mV into IDs for passing to the MSR. */
#define ID16(MHz, mV, bus_clk) \
	((((MHz * 100 + 50) / bus_clk) << 8) | ((mV ? mV - 700 : 0) >> 4))

/* Possible bus speeds (multiplied by 100 for rounding) */
enum { BUS100 = 10000, BUS133 = 13333, BUS166 = 16666, BUS200 = 20000 };

/* Ultra Low Voltage Intel Pentium M processor 900 MHz */
static const uint16_t pm130_900_ulv[] = {
	ID16( 900, 1004, BUS100),
	ID16( 800,  988, BUS100),
	ID16( 600,  844, BUS100),
};

/* Ultra Low Voltage Intel Pentium M processor 1.00 GHz */
static const uint16_t pm130_1000_ulv[] = {
	ID16(1000, 1004, BUS100),
	ID16( 900,  988, BUS100),
	ID16( 800,  972, BUS100),
	ID16( 600,  844, BUS100),
};

/* Ultra Low Voltage Intel Pentium M processor 1.10 GHz */
static const uint16_t pm130_1100_ulv[] = {
	ID16(1100, 1004, BUS100),
	ID16(1000,  988, BUS100),
	ID16( 900,  972, BUS100),
	ID16( 800,  956, BUS100),
	ID16( 600,  844, BUS100),
};

/* Low Voltage Intel Pentium M processor 1.10 GHz */
static const uint16_t pm130_1100_lv[] = {
	ID16(1100, 1180, BUS100),
	ID16(1000, 1164, BUS100),
	ID16( 900, 1100, BUS100),
	ID16( 800, 1020, BUS100),
	ID16( 600,  956, BUS100),
};

/* Low Voltage Intel Pentium M processor 1.20 GHz */
static const uint16_t pm130_1200_lv[] = {
	ID16(1200, 1180, BUS100),
	ID16(1100, 1164, BUS100),
	ID16(1000, 1100, BUS100),
	ID16( 900, 1020, BUS100),
	ID16( 800, 1004, BUS100),
	ID16( 600,  956, BUS100),
};

/* Low Voltage Intel Pentium M processor 1.30 GHz */
static const uint16_t pm130_1300_lv[] = {
	ID16(1300, 1180, BUS100),
	ID16(1200, 1164, BUS100),
	ID16(1100, 1100, BUS100),
	ID16(1000, 1020, BUS100),
	ID16( 900, 1004, BUS100),
	ID16( 800,  988, BUS100),
	ID16( 600,  956, BUS100),
};

/* Intel Pentium M processor 1.30 GHz */
static const uint16_t pm130_1300[] = {
	ID16(1300, 1388, BUS100),
	ID16(1200, 1356, BUS100),
	ID16(1000, 1292, BUS100),
	ID16( 800, 1260, BUS100),
	ID16( 600,  956, BUS100),
};

/* Intel Pentium M processor 1.40 GHz */
static const uint16_t pm130_1400[] = {
	ID16(1400, 1484, BUS100),
	ID16(1200, 1436, BUS100),
	ID16(1000, 1308, BUS100),
	ID16( 800, 1180, BUS100),
	ID16( 600,  956, BUS100),
};

/* Intel Pentium M processor 1.50 GHz */
static const uint16_t pm130_1500[] = {
	ID16(1500, 1484, BUS100),
	ID16(1400, 1452, BUS100),
	ID16(1200, 1356, BUS100),
	ID16(1000, 1228, BUS100),
	ID16( 800, 1116, BUS100),
	ID16( 600,  956, BUS100),
};

/* Intel Pentium M processor 1.60 GHz */
static const uint16_t pm130_1600[] = {
	ID16(1600, 1484, BUS100),
	ID16(1400, 1420, BUS100),
	ID16(1200, 1276, BUS100),
	ID16(1000, 1164, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  956, BUS100),
};

/* Intel Pentium M processor 1.70 GHz */
static const uint16_t pm130_1700[] = {
	ID16(1700, 1484, BUS100),
	ID16(1400, 1308, BUS100),
	ID16(1200, 1228, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1004, BUS100),
	ID16( 600,  956, BUS100),
};

/* Intel Pentium M processor 723 1.0 GHz */
static const uint16_t pm90_n723[] = {
	ID16(1000,  940, BUS100),
	ID16( 900,  908, BUS100),
	ID16( 800,  876, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 733 1.1 GHz, VID #G */
static const uint16_t pm90_n733g[] = {
	ID16(1100,  956, BUS100),
	ID16(1000,  940, BUS100),
	ID16( 900,  908, BUS100),
	ID16( 800,  876, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 733 1.1 GHz, VID #H */
static const uint16_t pm90_n733h[] = {
	ID16(1100,  940, BUS100),
	ID16(1000,  924, BUS100),
	ID16( 900,  892, BUS100),
	ID16( 800,  876, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 733 1.1 GHz, VID #I */
static const uint16_t pm90_n733i[] = {
	ID16(1100,  924, BUS100),
	ID16(1000,  908, BUS100),
	ID16( 900,  892, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 733 1.1 GHz, VID #J */
static const uint16_t pm90_n733j[] = {
	ID16(1100,  908, BUS100),
	ID16(1000,  892, BUS100),
	ID16( 900,  876, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 733 1.1 GHz, VID #K */
static const uint16_t pm90_n733k[] = {
	ID16(1100,  892, BUS100),
	ID16(1000,  876, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 733 1.1 GHz, VID #L */
static const uint16_t pm90_n733l[] = {
	ID16(1100,  876, BUS100),
	ID16(1000,  876, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 753 1.2 GHz, VID #G */
static const uint16_t pm90_n753g[] = {
	ID16(1200,  956, BUS100),
	ID16(1100,  940, BUS100),
	ID16(1000,  908, BUS100),
	ID16( 900,  892, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 753 1.2 GHz, VID #H */
static const uint16_t pm90_n753h[] = {
	ID16(1200,  940, BUS100),
	ID16(1100,  924, BUS100),
	ID16(1000,  908, BUS100),
	ID16( 900,  876, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 753 1.2 GHz, VID #I */
static const uint16_t pm90_n753i[] = {
	ID16(1200,  924, BUS100),
	ID16(1100,  908, BUS100),
	ID16(1000,  892, BUS100),
	ID16( 900,  876, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 753 1.2 GHz, VID #J */
static const uint16_t pm90_n753j[] = {
	ID16(1200,  908, BUS100),
	ID16(1100,  892, BUS100),
	ID16(1000,  876, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 753 1.2 GHz, VID #K */
static const uint16_t pm90_n753k[] = {
	ID16(1200,  892, BUS100),
	ID16(1100,  892, BUS100),
	ID16(1000,  876, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 753 1.2 GHz, VID #L */
static const uint16_t pm90_n753l[] = {
	ID16(1200,  876, BUS100),
	ID16(1100,  876, BUS100),
	ID16(1000,  860, BUS100),
	ID16( 900,  844, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 773 1.3 GHz, VID #G */
static const uint16_t pm90_n773g[] = {
	ID16(1300,  956, BUS100),
	ID16(1200,  940, BUS100),
	ID16(1100,  924, BUS100),
	ID16(1000,  908, BUS100),
	ID16( 900,  876, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 773 1.3 GHz, VID #H */
static const uint16_t pm90_n773h[] = {
	ID16(1300,  940, BUS100),
	ID16(1200,  924, BUS100),
	ID16(1100,  908, BUS100),
	ID16(1000,  892, BUS100),
	ID16( 900,  876, BUS100),
	ID16( 800,  860, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 773 1.3 GHz, VID #I */
static const uint16_t pm90_n773i[] = {
	ID16(1300,  924, BUS100),
	ID16(1200,  908, BUS100),
	ID16(1100,  892, BUS100),
	ID16(1000,  876, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 773 1.3 GHz, VID #J */
static const uint16_t pm90_n773j[] = {
	ID16(1300,  908, BUS100),
	ID16(1200,  908, BUS100),
	ID16(1100,  892, BUS100),
	ID16(1000,  876, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 773 1.3 GHz, VID #K */
static const uint16_t pm90_n773k[] = {
	ID16(1300,  892, BUS100),
	ID16(1200,  892, BUS100),
	ID16(1100,  876, BUS100),
	ID16(1000,  860, BUS100),
	ID16( 900,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 773 1.3 GHz, VID #L */
static const uint16_t pm90_n773l[] = {
	ID16(1300,  876, BUS100),
	ID16(1200,  876, BUS100),
	ID16(1100,  860, BUS100),
	ID16(1000,  860, BUS100),
	ID16( 900,  844, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  812, BUS100),
};

/* Intel Pentium M processor 738 1.4 GHz */
static const uint16_t pm90_n738[] = {
	ID16(1400, 1116, BUS100),
	ID16(1300, 1116, BUS100),
	ID16(1200, 1100, BUS100),
	ID16(1100, 1068, BUS100),
	ID16(1000, 1052, BUS100),
	ID16( 900, 1036, BUS100),
	ID16( 800, 1020, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 758 1.5 GHz */
static const uint16_t pm90_n758[] = {
	ID16(1500, 1116, BUS100),
	ID16(1400, 1116, BUS100),
	ID16(1300, 1100, BUS100),
	ID16(1200, 1084, BUS100),
	ID16(1100, 1068, BUS100),
	ID16(1000, 1052, BUS100),
	ID16( 900, 1036, BUS100),
	ID16( 800, 1020, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 778 1.6 GHz */
static const uint16_t pm90_n778[] = {
	ID16(1600, 1116, BUS100),
	ID16(1500, 1116, BUS100),
	ID16(1400, 1100, BUS100),
	ID16(1300, 1184, BUS100),
	ID16(1200, 1068, BUS100),
	ID16(1100, 1052, BUS100),
	ID16(1000, 1052, BUS100),
	ID16( 900, 1036, BUS100),
	ID16( 800, 1020, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 710 1.4 GHz, 533 MHz FSB */
static const uint16_t pm90_n710[] = {
	ID16(1400, 1340, BUS133),
	ID16(1200, 1228, BUS133),
	ID16(1000, 1148, BUS133),
	ID16( 800, 1068, BUS133),
	ID16( 600,  998, BUS133),
};

/* Intel Pentium M processor 715 1.5 GHz, VID #A */
static const uint16_t pm90_n715a[] = {
	ID16(1500, 1340, BUS100),
	ID16(1200, 1228, BUS100),
	ID16(1000, 1148, BUS100),
	ID16( 800, 1068, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 715 1.5 GHz, VID #B */
static const uint16_t pm90_n715b[] = {
	ID16(1500, 1324, BUS100),
	ID16(1200, 1212, BUS100),
	ID16(1000, 1148, BUS100),
	ID16( 800, 1068, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 715 1.5 GHz, VID #C */
static const uint16_t pm90_n715c[] = {
	ID16(1500, 1308, BUS100),
	ID16(1200, 1212, BUS100),
	ID16(1000, 1132, BUS100),
	ID16( 800, 1068, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 715 1.5 GHz, VID #D */
static const uint16_t pm90_n715d[] = {
	ID16(1500, 1276, BUS100),
	ID16(1200, 1180, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 725 1.6 GHz, VID #A */
static const uint16_t pm90_n725a[] = {
	ID16(1600, 1340, BUS100),
	ID16(1400, 1276, BUS100),
	ID16(1200, 1212, BUS100),
	ID16(1000, 1132, BUS100),
	ID16( 800, 1068, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 725 1.6 GHz, VID #B */
static const uint16_t pm90_n725b[] = {
	ID16(1600, 1324, BUS100),
	ID16(1400, 1260, BUS100),
	ID16(1200, 1196, BUS100),
	ID16(1000, 1132, BUS100),
	ID16( 800, 1068, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 725 1.6 GHz, VID #C */
static const uint16_t pm90_n725c[] = {
	ID16(1600, 1308, BUS100),
	ID16(1400, 1244, BUS100),
	ID16(1200, 1180, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 725 1.6 GHz, VID #D */
static const uint16_t pm90_n725d[] = {
	ID16(1600, 1276, BUS100),
	ID16(1400, 1228, BUS100),
	ID16(1200, 1164, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 730 1.6 GHz, 533 MHz FSB */
static const uint16_t pm90_n730[] = {
       ID16(1600, 1308, BUS133),
       ID16(1333, 1260, BUS133),
       ID16(1200, 1212, BUS133),
       ID16(1067, 1180, BUS133),
       ID16( 800,  988, BUS133),
};

/* Intel Pentium M processor 735 1.7 GHz, VID #A */
static const uint16_t pm90_n735a[] = {
	ID16(1700, 1340, BUS100),
	ID16(1400, 1244, BUS100),
	ID16(1200, 1180, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 735 1.7 GHz, VID #B */
static const uint16_t pm90_n735b[] = {
	ID16(1700, 1324, BUS100),
	ID16(1400, 1244, BUS100),
	ID16(1200, 1180, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 735 1.7 GHz, VID #C */
static const uint16_t pm90_n735c[] = {
	ID16(1700, 1308, BUS100),
	ID16(1400, 1228, BUS100),
	ID16(1200, 1164, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 735 1.7 GHz, VID #D */
static const uint16_t pm90_n735d[] = {
	ID16(1700, 1276, BUS100),
	ID16(1400, 1212, BUS100),
	ID16(1200, 1148, BUS100),
	ID16(1000, 1100, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 740 1.73 GHz, 533 MHz FSB */
static const uint16_t pm90_n740[] = {
       ID16(1733, 1356, BUS133),
       ID16(1333, 1212, BUS133),
       ID16(1067, 1100, BUS133),
       ID16( 800,  988, BUS133),
};

/* Intel Pentium M processor 745 1.8 GHz, VID #A */
static const uint16_t pm90_n745a[] = {
	ID16(1800, 1340, BUS100),
	ID16(1600, 1292, BUS100),
	ID16(1400, 1228, BUS100),
	ID16(1200, 1164, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 745 1.8 GHz, VID #B */
static const uint16_t pm90_n745b[] = {
	ID16(1800, 1324, BUS100),
	ID16(1600, 1276, BUS100),
	ID16(1400, 1212, BUS100),
	ID16(1200, 1164, BUS100),
	ID16(1000, 1116, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 745 1.8 GHz, VID #C */
static const uint16_t pm90_n745c[] = {
	ID16(1800, 1308, BUS100),
	ID16(1600, 1260, BUS100),
	ID16(1400, 1212, BUS100),
	ID16(1200, 1148, BUS100),
	ID16(1000, 1100, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 745 1.8 GHz, VID #D */
static const uint16_t pm90_n745d[] = {
	ID16(1800, 1276, BUS100),
	ID16(1600, 1228, BUS100),
	ID16(1400, 1180, BUS100),
	ID16(1200, 1132, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 750 1.86 GHz, 533 MHz FSB */
/* values extracted from \_PR\NPSS (via _PSS) SDST ACPI table */
static const uint16_t pm90_n750[] = {
	ID16(1867, 1308, BUS133),
	ID16(1600, 1228, BUS133),
	ID16(1333, 1148, BUS133),
	ID16(1067, 1068, BUS133),
	ID16( 800,  988, BUS133),
};

/* Intel Pentium M processor 755 2.0 GHz, VID #A */
static const uint16_t pm90_n755a[] = {
	ID16(2000, 1340, BUS100),
	ID16(1800, 1292, BUS100),
	ID16(1600, 1244, BUS100),
	ID16(1400, 1196, BUS100),
	ID16(1200, 1148, BUS100),
	ID16(1000, 1100, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 755 2.0 GHz, VID #B */
static const uint16_t pm90_n755b[] = {
	ID16(2000, 1324, BUS100),
	ID16(1800, 1276, BUS100),
	ID16(1600, 1228, BUS100),
	ID16(1400, 1180, BUS100),
	ID16(1200, 1132, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 755 2.0 GHz, VID #C */
static const uint16_t pm90_n755c[] = {
	ID16(2000, 1308, BUS100),
	ID16(1800, 1276, BUS100),
	ID16(1600, 1228, BUS100),
	ID16(1400, 1180, BUS100),
	ID16(1200, 1132, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 755 2.0 GHz, VID #D */
static const uint16_t pm90_n755d[] = {
	ID16(2000, 1276, BUS100),
	ID16(1800, 1244, BUS100),
	ID16(1600, 1196, BUS100),
	ID16(1400, 1164, BUS100),
	ID16(1200, 1116, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 760 2.0 GHz, 533 MHz FSB */
static const uint16_t pm90_n760[] = {
	ID16(2000, 1356, BUS133),
	ID16(1600, 1244, BUS133),
	ID16(1333, 1164, BUS133),
	ID16(1067, 1084, BUS133),
	ID16( 800,  988, BUS133),
};

/* Intel Pentium M processor 765 2.1 GHz, VID #A */
static const uint16_t pm90_n765a[] = {
	ID16(2100, 1340, BUS100),
	ID16(1800, 1276, BUS100),
	ID16(1600, 1228, BUS100),
	ID16(1400, 1180, BUS100),
	ID16(1200, 1132, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 765 2.1 GHz, VID #B */
static const uint16_t pm90_n765b[] = {
	ID16(2100, 1324, BUS100),
	ID16(1800, 1260, BUS100),
	ID16(1600, 1212, BUS100),
	ID16(1400, 1180, BUS100),
	ID16(1200, 1132, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 765 2.1 GHz, VID #C */
static const uint16_t pm90_n765c[] = {
	ID16(2100, 1308, BUS100),
	ID16(1800, 1244, BUS100),
	ID16(1600, 1212, BUS100),
	ID16(1400, 1164, BUS100),
	ID16(1200, 1116, BUS100),
	ID16(1000, 1084, BUS100),
	ID16( 800, 1036, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 765 2.1 GHz, VID #E */
static const uint16_t pm90_n765e[] = {
	ID16(2100, 1356, BUS100),
	ID16(1800, 1292, BUS100),
	ID16(1600, 1244, BUS100),
	ID16(1400, 1196, BUS100),
	ID16(1200, 1148, BUS100),
	ID16(1000, 1100, BUS100),
	ID16( 800, 1052, BUS100),
	ID16( 600,  988, BUS100),
};

/* Intel Pentium M processor 770 2.13 GHz */
static const uint16_t pm90_n770[] = {
	ID16(2133, 1356, BUS133),
	ID16(1867, 1292, BUS133),
	ID16(1600, 1212, BUS133),
	ID16(1333, 1148, BUS133),
	ID16(1067, 1068, BUS133),
	ID16( 800,  988, BUS133),
};

/* Intel Pentium M processor 780 2.26 GHz */ 
static const uint16_t pm90_n780[] = {
	ID16(2267, 1388, BUS133),
	ID16(1867, 1292, BUS133),
	ID16(1600, 1212, BUS133),
	ID16(1333, 1148, BUS133),
	ID16(1067, 1068, BUS133), 
	ID16( 800,  988, BUS133),
}; 

struct fqlist {
	int vendor;
	unsigned bus_clk;
	unsigned n;
	const uint16_t *table;
};

#define ENTRY(ven, bus_clk, tab) \
	{ CPUVENDOR_##ven, bus_clk == BUS133 ? 1 : 0, __arraycount(tab), tab }

#define BUS_CLK(fqp) ((fqp)->bus_clk ? BUS133 : BUS100)

static const struct fqlist est_cpus[] = {
	ENTRY(INTEL, BUS100, pm130_900_ulv),
	ENTRY(INTEL, BUS100, pm130_1000_ulv),
	ENTRY(INTEL, BUS100, pm130_1100_ulv),
	ENTRY(INTEL, BUS100, pm130_1100_lv),
	ENTRY(INTEL, BUS100, pm130_1200_lv),
	ENTRY(INTEL, BUS100, pm130_1300_lv),
	ENTRY(INTEL, BUS100, pm130_1300),
	ENTRY(INTEL, BUS100, pm130_1400),
	ENTRY(INTEL, BUS100, pm130_1500),
	ENTRY(INTEL, BUS100, pm130_1600),
	ENTRY(INTEL, BUS100, pm130_1700),
	ENTRY(INTEL, BUS100, pm90_n723),
	ENTRY(INTEL, BUS100, pm90_n733g),
	ENTRY(INTEL, BUS100, pm90_n733h),
	ENTRY(INTEL, BUS100, pm90_n733i),
	ENTRY(INTEL, BUS100, pm90_n733j),
	ENTRY(INTEL, BUS100, pm90_n733k),
	ENTRY(INTEL, BUS100, pm90_n733l),
	ENTRY(INTEL, BUS100, pm90_n753g),
	ENTRY(INTEL, BUS100, pm90_n753h),
	ENTRY(INTEL, BUS100, pm90_n753i),
	ENTRY(INTEL, BUS100, pm90_n753j),
	ENTRY(INTEL, BUS100, pm90_n753k),
	ENTRY(INTEL, BUS100, pm90_n753l),
	ENTRY(INTEL, BUS100, pm90_n773g),
	ENTRY(INTEL, BUS100, pm90_n773h),
	ENTRY(INTEL, BUS100, pm90_n773i),
	ENTRY(INTEL, BUS100, pm90_n773j),
	ENTRY(INTEL, BUS100, pm90_n773k),
	ENTRY(INTEL, BUS100, pm90_n773l),
	ENTRY(INTEL, BUS100, pm90_n738),
	ENTRY(INTEL, BUS100, pm90_n758),
	ENTRY(INTEL, BUS100, pm90_n778),

	ENTRY(INTEL, BUS133, pm90_n710),
	ENTRY(INTEL, BUS100, pm90_n715a),
	ENTRY(INTEL, BUS100, pm90_n715b),
	ENTRY(INTEL, BUS100, pm90_n715c),
	ENTRY(INTEL, BUS100, pm90_n715d),
	ENTRY(INTEL, BUS100, pm90_n725a),
	ENTRY(INTEL, BUS100, pm90_n725b),
	ENTRY(INTEL, BUS100, pm90_n725c),
	ENTRY(INTEL, BUS100, pm90_n725d),
	ENTRY(INTEL, BUS133, pm90_n730),
	ENTRY(INTEL, BUS100, pm90_n735a),
	ENTRY(INTEL, BUS100, pm90_n735b),
	ENTRY(INTEL, BUS100, pm90_n735c),
	ENTRY(INTEL, BUS100, pm90_n735d),
	ENTRY(INTEL, BUS133, pm90_n740),
	ENTRY(INTEL, BUS100, pm90_n745a),
	ENTRY(INTEL, BUS100, pm90_n745b),
	ENTRY(INTEL, BUS100, pm90_n745c),
	ENTRY(INTEL, BUS100, pm90_n745d),
	ENTRY(INTEL, BUS133, pm90_n750),
	ENTRY(INTEL, BUS100, pm90_n755a),
	ENTRY(INTEL, BUS100, pm90_n755b),
	ENTRY(INTEL, BUS100, pm90_n755c),
	ENTRY(INTEL, BUS100, pm90_n755d),
	ENTRY(INTEL, BUS133, pm90_n760),
	ENTRY(INTEL, BUS100, pm90_n765a),
	ENTRY(INTEL, BUS100, pm90_n765b),
	ENTRY(INTEL, BUS100, pm90_n765c),
	ENTRY(INTEL, BUS100, pm90_n765e),
	ENTRY(INTEL, BUS133, pm90_n770),
	ENTRY(INTEL, BUS133, pm90_n780)
};

#define MSR2FREQINC(msr)	(((int) (msr) >> 8) & 0xff)
#define MSR2VOLTINC(msr)	((int) (msr) & 0xff)

#define MSR2MHZ(msr, bus)	((MSR2FREQINC((msr)) * (bus) + 50) / 100)
#define MSR2MV(msr)		(MSR2VOLTINC(msr) * 16 + 700)

static const struct 	fqlist *est_fqlist; /* not NULL if functional */
static uint16_t         fake_table[3];      /* guessed est_cpu table */
static struct fqlist    fake_fqlist;
static int 		est_node_target, est_node_current;
static const char 	est_desc[] = "Enhanced SpeedStep";
static int		lvendor, bus_clock;

static int		est_sysctl_helper(SYSCTLFN_PROTO);
static int		est_init_once(void);
static void		est_init_main(int);

static int
est_sysctl_helper(SYSCTLFN_ARGS)
{
	struct msr_cpu_broadcast mcb;
	struct sysctlnode	node;
	int			fq, oldfq, error;

	if (est_fqlist == NULL)
		return EOPNOTSUPP;

	node = *rnode;
	node.sysctl_data = &fq;

	oldfq = 0;
	if (rnode->sysctl_num == est_node_target)
		fq = oldfq = MSR2MHZ(rdmsr(MSR_PERF_CTL), bus_clock);
	else if (rnode->sysctl_num == est_node_current)
		fq = MSR2MHZ(rdmsr(MSR_PERF_STATUS), bus_clock);
	else
		return EOPNOTSUPP;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* support writing to ...frequency.target */
	if (rnode->sysctl_num == est_node_target && fq != oldfq) {
		int		i;

		for (i = est_fqlist->n - 1; i > 0; i--)
			if (MSR2MHZ(est_fqlist->table[i], bus_clock) >= fq)
				break;
		fq = MSR2MHZ(est_fqlist->table[i], bus_clock);
		mcb.msr_read = true;
		mcb.msr_type = MSR_PERF_CTL;
		mcb.msr_mask = 0xffffULL;
		mcb.msr_value = est_fqlist->table[i];
		msr_cpu_broadcast(&mcb);
	}

	return 0;
}

static int
est_init_once(void)
{
	est_init_main(lvendor);
	return 0;
}

void
est_init(int vendor)
{
	int error;
	static ONCE_DECL(est_initialized);

	error = RUN_ONCE(&est_initialized, est_init_once);
	if (__predict_false(error != 0))
		return;
}

static void
est_init_main(int vendor)
{
#ifdef __i386__
	const struct fqlist	*fql;
#endif
	const struct sysctlnode	*node, *estnode, *freqnode;
	uint64_t		msr;
	uint16_t		cur, idhi, idlo;
	uint8_t			crhi, crlo, crcur;
	int			i, mv, rc;
	size_t			len, freq_len;
	char			*freq_names, *cpuname;
       
	cpuname	= curcpu()->ci_dev->dv_xname;

	if (CPUID2FAMILY(curcpu()->ci_signature) == 15)
		bus_clock = p4_get_bus_clock(curcpu());
	else if (CPUID2FAMILY(curcpu()->ci_signature) == 6)
		bus_clock = p3_get_bus_clock(curcpu());

	if (bus_clock == 0) {
		aprint_normal("%s: unknown system bus clock\n", __func__);
		return;
	}

	lvendor = vendor;

	msr = rdmsr(MSR_PERF_STATUS);
	idhi = (msr >> 32) & 0xffff;
	idlo = (msr >> 48) & 0xffff;
	cur = msr & 0xffff;
	crhi = (idhi  >> 8) & 0xff;
	crlo = (idlo  >> 8) & 0xff;
	crcur = (cur >> 8) & 0xff;

#ifdef __i386__
	if (idhi == 0 || idlo == 0 || cur == 0 ||
	    ((cur >> 8) & 0xff) < ((idlo >> 8) & 0xff) ||
	    ((cur >> 8) & 0xff) > ((idhi >> 8) & 0xff)) {
		aprint_debug("%s: strange msr value 0x%016llx\n", __func__, msr);
		return;
	}
#endif

#ifdef __amd64__
	if (crlo == 0 || crhi == crlo) {
		aprint_debug("%s: crlo == 0 || crhi == crlo\n", __func__);
		return;
	}

	if (crhi == 0 || crcur == 0 || crlo > crhi ||
	    crcur < crlo || crcur > crhi) {
		/*
		 * Do complain about other weirdness, because we first want to
		 * know about it, before we decide what to do with it
		 */
		aprint_normal("%s: EST: strange msr value 0x%" PRIu64 "\n",
		    cpuname, msr);
		return;
	}
#endif

	msr = rdmsr(MSR_PERF_STATUS);
	mv = MSR2MV(msr);
	aprint_normal("%s: %s (%d mV) ", cpuname, est_desc, mv);
	aprint_normal("%d MHz\n", MSR2MHZ(msr, bus_clock));

#ifdef __i386__
	/*
	 * Find an entry which matches (vendor, bus_clock, idhi, idlo)
	 */
	est_fqlist = NULL;
	for (i = 0; i < __arraycount(est_cpus); i++) {
		fql = &est_cpus[i];
		if (vendor == fql->vendor && bus_clock == BUS_CLK(fql) &&
		    idhi == fql->table[0] && idlo == fql->table[fql->n - 1]) {
			est_fqlist = fql;
			break;
		}
	}
#endif

	if (est_fqlist == NULL) {
                aprint_normal("%s: unknown Enhanced SpeedStep CPU.\n",
                    cpuname);

		/*
		 * Some CPUs report the same frequency in idhi and idlo,
		 * so do not run est on them.
		 */
		if (idhi == idlo)
			return;
                /*
                 * Generate a fake table with the power states we know.
                 */
                fake_table[0] = idhi;
                if (cur == idhi || cur == idlo) {
                        aprint_normal("%s: using only highest and lowest "
                            "power states.\n", cpuname);

                        fake_table[1] = idlo;
                        fake_fqlist.n = 2;
                } else {
                        aprint_normal("%s: using only highest, current "
                            "and lowest power states.\n", cpuname);

                        fake_table[1] = cur;
                        fake_table[2] = idlo;
                        fake_fqlist.n = 3;
                }
		fake_fqlist.vendor = vendor;
		fake_fqlist.table = fake_table;
		est_fqlist = &fake_fqlist;
	}

	/*
	 * OK, tell the user the available frequencies.
	 */
	freq_len = est_fqlist->n * (sizeof("9999 ")-1) + 1;
	freq_names = malloc(freq_len, M_SYSCTLDATA, M_WAITOK);
	freq_names[0] = '\0';
	len = 0;
	for (i = 0; i < est_fqlist->n; i++) {
		len += snprintf(freq_names + len, freq_len - len, "%d%s",
		    MSR2MHZ(est_fqlist->table[i], bus_clock),
		    i < est_fqlist->n - 1 ? " " : "");
	}
	aprint_normal("%s: %s frequencies available (MHz): %s\n",
	    cpuname, est_desc, freq_names);

	/*
	 * Setup the sysctl sub-tree machdep.est.*
	 */
	if ((rc = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(NULL, 0, &node, &estnode,
	    0, CTLTYPE_NODE, "est", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(NULL, 0, &estnode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(NULL, 0, &freqnode, &node,
	    EST_TARGET_CTLFLAG, CTLTYPE_INT, "target", NULL,
	    est_sysctl_helper, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	est_node_target = node->sysctl_num;

	if ((rc = sysctl_createv(NULL, 0, &freqnode, &node,
	    0, CTLTYPE_INT, "current", NULL,
	    est_sysctl_helper, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	est_node_current = node->sysctl_num;

	if ((rc = sysctl_createv(NULL, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, freq_names, freq_len, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return;

 err:
	free(freq_names, M_SYSCTLDATA);
	aprint_normal("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}
