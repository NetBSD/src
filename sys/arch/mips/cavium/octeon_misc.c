/*	$NetBSD: octeon_misc.c,v 1.2 2022/02/06 20:20:19 andvar Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
 * Copyright (c) 2009, 2010 Miodrag Vallat.
 * Copyright (c) 2019 Visa Hankala.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 2003-2004 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_misc.c,v 1.2 2022/02/06 20:20:19 andvar Exp $");

#include <sys/param.h>

#include <mips/cavium/octeonreg.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>

int	octeon_core_ver;

/*
 * Return the name of the CPU model we are running on.
 * Side effect: sets the octeon_core_ver variable.
 */
const char *
octeon_cpu_model(mips_prid_t cpu_id)
{
	const uint64_t fuse = octeon_xkphys_read_8(CIU_FUSE);
	const int numcores = popcount64(fuse);
	const int clock_mhz = curcpu()->ci_cpu_freq / 1000000;
	const char *family;
	const char *coremodel;
	bool tested;
	static char buf[sizeof("CNnnXX-NNNN (unverified)")] = {};

	if (buf[0] != 0)
		return buf;		/* we've been here before */

	/* 
	 * Don't print "pass X.Y", but if needed:
	 *	passhi = ((cpu_id >> 3) & 7) + '0';
	 *	passlo = (cpu_id & 7) + '0';
	 * Note some chips use different representation for the pass number.
	 */

#ifdef OCTEON_DEBUG
	printf("cpuid = 0x%x\n", cpu_id);
#endif

	switch (numcores) {
	case  1: coremodel = "10"; break;
	case  2: coremodel = "20"; break;
	case  3: coremodel = "25"; break;
	case  4: coremodel = "30"; break;
	case  5: coremodel = "32"; break;
	case  6: coremodel = "34"; break;
	case  7: coremodel = "38"; break;
	case  8: coremodel = "40"; break;
	case  9: coremodel = "42"; break;
	case 10: coremodel = "45"; break;
	case 11: coremodel = "48"; break;
	case 12: coremodel = "50"; break;
	case 13: coremodel = "52"; break;
	case 14: coremodel = "55"; break;
	case 15: coremodel = "58"; break;
	case 16: coremodel = "60"; break;
	case 24: coremodel = "70"; break;
	case 32: coremodel = "80"; break;
	case 40: coremodel = "85"; break;
	case 44: coremodel = "88"; break;
	case 48: coremodel = "90"; break;
	default:
		coremodel = "XX"; break;
	}

	/*
         * Assume all CPU families haven't been tested unless explicitly
         * noted.  Sources of extra information for determining actual
         * CPU models include chip documentation and U-Boot source code.
	 */
	tested = false;

	switch (MIPS_PRID_IMPL(cpu_id)) {
	/* the order of these cases is the numeric value of MIPS_CNnnXX */
	case MIPS_CN38XX:
		family = "38";	/* XXX may also be family "36" or "37" */
		octeon_core_ver = OCTEON_1;
		break;
	case MIPS_CN31XX:
		family = "31";	/* XXX may also be model "3020" */
		octeon_core_ver = OCTEON_1;
		break;
	case MIPS_CN30XX:
		family = "30";	/* XXX half cache model is "3005" */
		octeon_core_ver = OCTEON_1;
		break;
	case MIPS_CN58XX:
		family = "58";
		octeon_core_ver = OCTEON_PLUS;
		break;
	case MIPS_CN56XX:
		family = "56";	/* XXX may also be family "54", "55" or "57" */
		octeon_core_ver = OCTEON_PLUS;
		break;
	case MIPS_CN50XX:
		family = "50";
		octeon_core_ver = OCTEON_PLUS;
		tested = true;
		break;
	case MIPS_CN52XX:
		family = "52";	/* XXX may also be family "51" */
		octeon_core_ver = OCTEON_PLUS;
		break;
	case MIPS_CN63XX:
		family = "63";	/* XXX may also be family "62" */
		octeon_core_ver = OCTEON_2;
		break;
	case MIPS_CN68XX:
		family = "68";
		octeon_core_ver = OCTEON_2;
		break;
	case MIPS_CN66XX:
		family = "66";
		octeon_core_ver = OCTEON_2;
		break;
	case MIPS_CN61XX:
		family = "61";	/* XXX may also be family "60" */
		octeon_core_ver = OCTEON_2;
		break;
	case MIPS_CN78XX:
		family = "78";	/* XXX may also be family "76" or "77" */
		octeon_core_ver = OCTEON_3;
		break;
	case MIPS_CN70XX:
		family = "70";
		if (octeon_xkphys_read_8(MIO_FUS_PDF) & MIO_FUS_PDF_IS_71XX)
			family = "71";
		octeon_core_ver = OCTEON_3;
		tested = true;
		break;
	case MIPS_CN73XX:
		family = "73";	/* XXX may also be family "72" */
		octeon_core_ver = OCTEON_3;
		tested = true;
		break;
	default:
		panic("IMPL 0x%02x not implemented", MIPS_PRID_IMPL(cpu_id));
	}

	snprintf(buf, sizeof(buf), "CN%s%s-%d%s", family, coremodel,
	    clock_mhz, tested ? "" : " (unverified)");

	if (!tested)
		printf(">>> model %s\n", buf);

	return buf;
}

/*
 * Return the coprocessor clock speed (IO clock speed).
 *
 * Octeon I and Octeon Plus use the CPU core clock speed.
 * Octeon II and III use a configurable multiplier against
 * the PLL reference clock speed (50MHz).
 */
int
octeon_ioclock_speed(void)
{
	u_int64_t mio_rst_boot, rst_boot;

	switch (octeon_core_ver) {
	case OCTEON_1:
	case OCTEON_PLUS:
		return curcpu()->ci_cpu_freq;
	case OCTEON_2:
		mio_rst_boot = octeon_xkphys_read_8(MIO_RST_BOOT);
		return OCTEON_PLL_REF_CLK *
		    __SHIFTOUT(mio_rst_boot, MIO_RST_BOOT_PNR_MUL);
	case OCTEON_3:
		rst_boot = octeon_xkphys_read_8(RST_BOOT);
		return OCTEON_PLL_REF_CLK *
		    __SHIFTOUT(rst_boot, RST_BOOT_PNR_MUL);
	default:
		panic("%s: unknown Octeon core type %d", __func__,
		    octeon_core_ver);
	}
}

/*
 * Initiate chip soft-reset.
 */
void
octeon_soft_reset(void)
{

	/* XXX should invalidate caches, tlb, watchdog? */
	switch (octeon_core_ver) {
	case OCTEON_1:
	case OCTEON_PLUS:
	case OCTEON_2:
		octeon_xkphys_write_8(CIU_SOFT_BIST, CIU_SOFT_BIST_SOFT_BIST);
		octeon_xkphys_write_8(CIU_SOFT_RST, CIU_SOFT_RST_SOFT_RST);
	case OCTEON_3:
		octeon_xkphys_write_8(RST_SOFT_RST, CIU_SOFT_RST_SOFT_RST);
	default:
		panic("%s: unknown Octeon core type %d", __func__,
		    octeon_core_ver);
	}
}
