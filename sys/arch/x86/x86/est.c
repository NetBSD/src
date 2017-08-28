/*	$NetBSD: est.c,v 1.30.4.1 2017/08/28 17:51:56 skrll Exp $	*/
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
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: est.c,v 1.30.4.1 2017/08/28 17:51:56 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/cpu_msr.h>
#include <x86/est.h>
#include <x86/specialreg.h>

#define MSR2FREQINC(msr)	(((int) (msr) >> 8) & 0xff)
#define MSR2VOLTINC(msr)	((int) (msr) & 0xff)

#define MSR2MHZ(msr, bus)	((MSR2FREQINC((msr)) * (bus) + 50) / 100)
#define MSR2MV(msr)		(MSR2VOLTINC(msr) * 16 + 700)

/* Convert MHz and mV into IDs for passing to the MSR. */
#define ID16(MHz, mV, bus_clk) \
	((((MHz * 100 + 50) / bus_clk) << 8) | ((mV ? mV - 700 : 0) >> 4))

#define ENTRY(ven, bus_clk, tab) \
	{ CPUVENDOR_##ven, bus_clk == BUS133 ? 1 : 0, __arraycount(tab), tab }

#define BUS_CLK(fqp) ((fqp)->bus_clk ? BUS133 : BUS100)

struct fqlist {
	int		  vendor;
	unsigned	  bus_clk;
	unsigned	  n;
	const uint16_t	 *table;
};

#ifdef __i386__

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

/*
 * VIA C7-M 500 MHz FSB, 400 MHz FSB, and ULV variants.
 * Data from the "VIA C7-M Processor BIOS Writer's Guide (v2.17)" datasheet.
 */

/* 1.00GHz Centaur C7-M ULV */
static const uint16_t C7M_770_ULV[] = {
	ID16(1000,  844, BUS100),
	ID16( 800,  796, BUS100),
	ID16( 600,  796, BUS100),
	ID16( 400,  796, BUS100),
};

/* 1.00GHz Centaur C7-M ULV */
static const uint16_t C7M_779_ULV[] = {
	ID16(1000,  796, BUS100),
	ID16( 800,  796, BUS100),
	ID16( 600,  796, BUS100),
	ID16( 400,  796, BUS100),
};

/* 1.20GHz Centaur C7-M ULV */
static const uint16_t C7M_772_ULV[] = {
	ID16(1200,  844, BUS100),
	ID16(1000,  844, BUS100),
	ID16( 800,  828, BUS100),
	ID16( 600,  796, BUS100),
	ID16( 400,  796, BUS100),
};

/* 1.50GHz Centaur C7-M ULV */
static const uint16_t C7M_775_ULV[] = {
	ID16(1500,  956, BUS100),
	ID16(1400,  940, BUS100),
	ID16(1000,  860, BUS100),
	ID16( 800,  828, BUS100),
	ID16( 600,  796, BUS100),
	ID16( 400,  796, BUS100),
};

/* 1.20GHz Centaur C7-M 400 MHz FSB */
static const uint16_t C7M_771[] = {
	ID16(1200,  860, BUS100),
	ID16(1000,  860, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  844, BUS100),
	ID16( 400,  844, BUS100),
};

/* 1.50GHz Centaur C7-M 400 MHz FSB */
static const uint16_t C7M_754[] = {
	ID16(1500, 1004, BUS100),
	ID16(1400,  988, BUS100),
	ID16(1000,  940, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  844, BUS100),
	ID16( 400,  844, BUS100),
};

/* 1.60GHz Centaur C7-M 400 MHz FSB */
static const uint16_t C7M_764[] = {
	ID16(1600, 1084, BUS100),
	ID16(1400, 1052, BUS100),
	ID16(1000, 1004, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  844, BUS100),
	ID16( 400,  844, BUS100),
};

/* 1.80GHz Centaur C7-M 400 MHz FSB */
static const uint16_t C7M_784[] = {
	ID16(1800, 1148, BUS100),
	ID16(1600, 1100, BUS100),
	ID16(1400, 1052, BUS100),
	ID16(1000, 1004, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  844, BUS100),
	ID16( 400,  844, BUS100),
};

/* 2.00GHz Centaur C7-M 400 MHz FSB */
static const uint16_t C7M_794[] = {
	ID16(2000, 1148, BUS100),
	ID16(1800, 1132, BUS100),
	ID16(1600, 1100, BUS100),
	ID16(1400, 1052, BUS100),
	ID16(1000, 1004, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  844, BUS100),
	ID16( 400,  844, BUS100),
};

/* 1.60GHz Centaur C7-M 533 MHz FSB */
static const uint16_t C7M_765[] = {
	ID16(1600, 1084, BUS133),
	ID16(1467, 1052, BUS133),
	ID16(1200, 1004, BUS133),
	ID16( 800,  844, BUS133),
	ID16( 667,  844, BUS133),
	ID16( 533,  844, BUS133),
};

/* 2.00GHz Centaur C7-M 533 MHz FSB */
static const uint16_t C7M_785[] = {
	ID16(1867, 1148, BUS133),
	ID16(1600, 1100, BUS133),
	ID16(1467, 1052, BUS133),
	ID16(1200, 1004, BUS133),
	ID16( 800,  844, BUS133),
	ID16( 667,  844, BUS133),
	ID16( 533,  844, BUS133),
};

/* 2.00GHz Centaur C7-M 533 MHz FSB */
static const uint16_t C7M_795[] = {
	ID16(2000, 1148, BUS133),
	ID16(1867, 1132, BUS133),
	ID16(1600, 1100, BUS133),
	ID16(1467, 1052, BUS133),
	ID16(1200, 1004, BUS133),
	ID16( 800,  844, BUS133),
	ID16( 667,  844, BUS133),
	ID16( 533,  844, BUS133),
};

/* 1.00GHz VIA Eden 90nm 'Esther' */
static const uint16_t eden90_1000[] = {
	ID16(1000,  844, BUS100),
	ID16( 800,  844, BUS100),
	ID16( 600,  844, BUS100),
	ID16( 400,  844, BUS100),
};

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
	ENTRY(INTEL, BUS133, pm90_n780),
	ENTRY(IDT,   BUS100, C7M_770_ULV),
	ENTRY(IDT,   BUS100, C7M_779_ULV),
	ENTRY(IDT,   BUS100, C7M_772_ULV),
	ENTRY(IDT,   BUS100, C7M_771),
	ENTRY(IDT,   BUS100, C7M_775_ULV),
	ENTRY(IDT,   BUS100, C7M_754),
	ENTRY(IDT,   BUS100, C7M_764),
	ENTRY(IDT,   BUS133, C7M_765),
	ENTRY(IDT,   BUS100, C7M_784),
	ENTRY(IDT,   BUS133, C7M_785),
	ENTRY(IDT,   BUS100, C7M_794),
	ENTRY(IDT,   BUS133, C7M_795),
	ENTRY(IDT,   BUS100, eden90_1000)
};

#endif	/* __i386__ */

static int	est_match(device_t, cfdata_t, void *);
static void	est_attach(device_t, device_t, void *);
static int	est_detach(device_t, int);
static int	est_bus_clock(struct cpu_info *);
static bool	est_tables(device_t);
static void	est_xcall(uint16_t);
static bool	est_sysctl(device_t);
static int	est_sysctl_helper(SYSCTLFN_PROTO);

struct est_softc {
	device_t	  sc_dev;
	struct cpu_info	 *sc_ci;
	struct sysctllog *sc_log;
	struct fqlist	 *sc_fqlist;
	struct fqlist	  sc_fake_fqlist;
	uint16_t	 *sc_fake_table;
	char		 *sc_freqs;
	size_t		  sc_freqs_len;
	int		  sc_bus_clock;
	int		  sc_node_target;
	int		  sc_node_current;
};

CFATTACH_DECL_NEW(est, sizeof(struct est_softc),
    est_match, est_attach, est_detach, NULL);

static int
est_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;

	if (strcmp(cfaa->name, "frequency") != 0)
		return 0;

	if (cpu_vendor != CPUVENDOR_IDT &&
	    cpu_vendor != CPUVENDOR_INTEL)
		return 0;

	if ((ci->ci_feat_val[1] & CPUID2_EST) == 0)
		return 0;

	return (est_bus_clock(ci) != 0) ? 5 : 0;
}

static void
est_attach(device_t parent, device_t self, void *aux)
{
	struct est_softc *sc = device_private(self);
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;

	sc->sc_ci = ci;
	sc->sc_dev = self;
	sc->sc_log = NULL;
	sc->sc_freqs = NULL;
	sc->sc_fqlist = NULL;
	sc->sc_fake_table = NULL;
	sc->sc_bus_clock = est_bus_clock(ci);

	KASSERT(sc->sc_bus_clock != 0);

	aprint_naive("\n");
	aprint_normal(": Enhanced SpeedStep\n");

	(void)pmf_device_register(self, NULL, NULL);

	if (est_tables(self) != false)
		est_sysctl(self);
}

static int
est_detach(device_t self, int flags)
{
	struct est_softc *sc = device_private(self);
	uint16_t n = sc->sc_fake_fqlist.n;

	if (sc->sc_log != NULL)
		sysctl_teardown(&sc->sc_log);

	if (sc->sc_freqs != NULL)
		kmem_free(sc->sc_freqs, sc->sc_freqs_len);

	if (sc->sc_fake_table != NULL)
		kmem_free(sc->sc_fake_table, n * sizeof(*sc->sc_fake_table));

	pmf_device_deregister(self);

	return 0;
}

static int
est_bus_clock(struct cpu_info *ci)
{
	uint32_t family, model;
	int bus_clock = 0;

	family = CPUID_TO_BASEFAMILY(ci->ci_signature);
	model  = CPUID_TO_MODEL(ci->ci_signature);

	switch (family) {

	case 0x0f:
		bus_clock = p4_get_bus_clock(ci);
		break;

	case 0x06:

		if (cpu_vendor != CPUVENDOR_IDT)
			bus_clock = p3_get_bus_clock(ci);
		else {
			switch (model) {

			case 0x0a: /* C7 Esther */
			case 0x0d: /* C7 Esther */
				bus_clock = viac7_get_bus_clock(ci);
				break;

			default:
				bus_clock = via_get_bus_clock(ci);
				break;
			}
		}
	}

	return bus_clock;
}

static bool
est_tables(device_t self)
{
	struct est_softc *sc = device_private(self);

#ifdef __i386__
	const struct fqlist *fql;
#endif
	uint64_t msr;
	uint16_t cur, idhi, idlo;
	size_t len;
	int i, mv;

	msr = rdmsr(MSR_PERF_STATUS);

	idhi = (msr >> 32) & 0xffff;
	idlo = (msr >> 48) & 0xffff;
	cur = msr & 0xffff;

#ifdef __i386__
	if (idhi == 0 || idlo == 0 || cur == 0 ||
	    ((cur >> 8) & 0xff) < ((idlo >> 8) & 0xff) ||
	    ((cur >> 8) & 0xff) > ((idhi >> 8) & 0xff)) {
		aprint_debug_dev(self, "strange msr value 0x%"PRIx64"\n", msr);
		return false;
	}
#endif

#ifdef __amd64__
	uint8_t crhi = (idhi >> 8) & 0xff;
	uint8_t crlo = (idlo >> 8) & 0xff;
	uint8_t crcur = (cur >> 8) & 0xff;
	if (crlo == 0 || crhi == 0 || crcur == 0 || crhi == crlo ||
	    crlo > crhi || crcur < crlo || crcur > crhi) {
		/*
		 * Do complain about other weirdness, because we first want to
		 * know about it, before we decide what to do with it
		 */
		aprint_debug_dev(self, "strange msr value 0x%"PRIu64"\n", msr);
		aprint_debug_dev(self, " crhi=%u, crlo=%u, crcur=%u\n",
		    crhi, crlo, crcur);
		return false;
	}
#endif

	msr = rdmsr(MSR_PERF_STATUS);
	mv = MSR2MV(msr);

#ifdef __i386__
	/*
	 * Find an entry which matches (vendor, bus_clock, idhi, idlo).
	 */
	sc->sc_fqlist = NULL;

	for (i = 0; i < __arraycount(est_cpus); i++) {

		fql = &est_cpus[i];

		if (cpu_vendor == fql->vendor &&
		    sc->sc_bus_clock == BUS_CLK(fql) &&
		    idhi == fql->table[0] && idlo == fql->table[fql->n - 1]) {
			sc->sc_fqlist = __UNCONST(fql);
			break;
		}
	}
#endif

	if (sc->sc_fqlist == NULL) {
		int j, tablesize, freq, volt;
		int minfreq, minvolt, maxfreq, maxvolt, freqinc, voltinc;

		/*
		 * Some CPUs report the same frequency in idhi and idlo,
		 * so do not run est on them.
		 */
		if (idhi == idlo) {
			aprint_debug_dev(self, "idhi == idlo\n");
			return false;
		}

#ifdef EST_DEBUG
		aprint_normal_dev(self, "bus_clock = %d\n", sc->sc_bus_clock);
		aprint_normal_dev(self, "idlo = 0x%x\n", idlo);
		aprint_normal_dev(self, "lo  %4d mV, %4d MHz\n",
		    MSR2MV(idlo), MSR2MHZ(idlo, sc->sc_bus_clock));
		aprint_normal_dev(self, "raw %4d   , %4d    \n",
		    (idlo & 0xff), ((idlo >> 8) & 0xff));
		aprint_normal_dev(self, "idhi = 0x%x\n", idhi);
		aprint_normal_dev(self, "hi  %4d mV, %4d MHz\n",
		    MSR2MV(idhi), MSR2MHZ(idhi, sc->sc_bus_clock));
		aprint_normal_dev(self, "raw %4d   , %4d    \n",
		    (idhi & 0xff), ((idhi >> 8) & 0xff));
		aprint_normal_dev(self, "cur  = 0x%x\n", cur);
#endif

                /*
                 * Generate a fake table with the power states we know,
		 * interpolating the voltages and frequencies between the
		 * high and low values.  The (milli)voltages are always
		 * rounded up when computing the table.
                 */
		minfreq = MSR2FREQINC(idlo);
		maxfreq = MSR2FREQINC(idhi);
		minvolt = MSR2VOLTINC(idlo);
		maxvolt = MSR2VOLTINC(idhi);
		freqinc = maxfreq - minfreq;
		voltinc = maxvolt - minvolt;

		/* Avoid diving by zero. */
		if (freqinc == 0)
			return false;

		if (freqinc < voltinc || voltinc == 0) {
			tablesize = maxfreq - minfreq + 1;
			if (voltinc != 0)
				voltinc = voltinc * 100 / freqinc - 1;
			freqinc = 100;
		} else {
			tablesize = maxvolt - minvolt + 1;
			freqinc = freqinc * 100 / voltinc - 1;
			voltinc = 100;
		}

		sc->sc_fake_table = kmem_alloc(tablesize *
		    sizeof(uint16_t), KM_SLEEP);
		sc->sc_fake_fqlist.n = tablesize;

		/* The frequency/voltage table is highest frequency first */
		freq = maxfreq * 100;
		volt = maxvolt * 100;

		for (j = 0; j < tablesize; j++) {
			sc->sc_fake_table[j] = (((freq + 99) / 100) << 8) +
			    (volt + 99) / 100;
#ifdef EST_DEBUG
			aprint_normal_dev(self, "fake entry %d: %4d mV, "
			    "%4d MHz, MSR*100 mV = %4d freq = %4d\n",
			    j, MSR2MV(sc->sc_fake_table[j]),
			    MSR2MHZ(sc->sc_fake_table[j], sc->sc_bus_clock),
			    volt, freq);
#endif /* EST_DEBUG */
			freq -= freqinc;
			volt -= voltinc;
		}

		sc->sc_fake_fqlist.table = sc->sc_fake_table;
		sc->sc_fake_fqlist.vendor = cpu_vendor;
		sc->sc_fqlist = &sc->sc_fake_fqlist;
	}

	sc->sc_freqs_len = sc->sc_fqlist->n * (sizeof("9999 ") - 1) + 1;
	sc->sc_freqs = kmem_zalloc(sc->sc_freqs_len, KM_SLEEP);
	for (i = len = 0; i < sc->sc_fqlist->n; i++) {
		if (len >= sc->sc_freqs_len)
			break;
		len += snprintf(sc->sc_freqs + len, sc->sc_freqs_len - len,
		    "%d%s", MSR2MHZ(sc->sc_fqlist->table[i], sc->sc_bus_clock),
		    i < sc->sc_fqlist->n - 1 ? " " : "");
	}

	aprint_debug_dev(self, "%d mV, %d (MHz): %s\n", mv,
	    MSR2MHZ(msr, sc->sc_bus_clock), sc->sc_freqs);

	return true;
}

static void
est_xcall(uint16_t val)
{
	struct msr_rw_info msr;
	uint64_t xc;

	msr.msr_read = true;
	msr.msr_type = MSR_PERF_CTL;
	msr.msr_mask = 0xffffULL;
	msr.msr_value = val;

	xc = xc_broadcast(0, x86_msr_xcall, &msr, NULL);
	xc_wait(xc);
}

static bool
est_sysctl(device_t self)
{
	struct est_softc *sc = device_private(self);
	const struct sysctlnode	*node, *estnode, *freqnode;
	int rv;

	/*
	 * Setup the sysctl sub-tree machdep.est.*
	 */
	rv = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &node, &estnode,
	    0, CTLTYPE_NODE, "est", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &estnode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    est_sysctl_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	sc->sc_node_target = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_INT, "current", NULL,
	    est_sysctl_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	sc->sc_node_current = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, sc->sc_freqs, sc->sc_freqs_len,
	    CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	return true;

fail:
	aprint_error_dev(self, "failed to initialize sysctl (err %d)\n", rv);

	sysctl_teardown(&sc->sc_log);
	sc->sc_log = NULL;

	return false;
}

static int
est_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct est_softc *sc;
	int fq, err, i, oldfq;

	fq = oldfq = 0;

	node = *rnode;
	sc = node.sysctl_data;

	if (sc->sc_fqlist == NULL)
		return EOPNOTSUPP;

	node.sysctl_data = &fq;

	if (rnode->sysctl_num == sc->sc_node_target) {

		fq = oldfq = MSR2MHZ(rdmsr(MSR_PERF_CTL), sc->sc_bus_clock);

	} else if (rnode->sysctl_num == sc->sc_node_current) {

		fq = MSR2MHZ(rdmsr(MSR_PERF_STATUS), sc->sc_bus_clock);

	} else
		return EOPNOTSUPP;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err != 0 || newp == NULL)
		return err;

	if (fq == oldfq || rnode->sysctl_num != sc->sc_node_target)
		return 0;

	for (i = sc->sc_fqlist->n - 1; i > 0; i--) {

		if (MSR2MHZ(sc->sc_fqlist->table[i], sc->sc_bus_clock) >= fq)
			break;
	}

	est_xcall(sc->sc_fqlist->table[i]);
	return 0;
}

MODULE(MODULE_CLASS_DRIVER, est, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
est_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_est,
		    cfattach_ioconf_est, cfdata_ioconf_est);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_est,
		    cfattach_ioconf_est, cfdata_ioconf_est);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
