/* $NetBSD: sbscdvar.h,v 1.3 2009/08/12 12:56:29 simonb Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* sbscd pseudo-offset (from base) of an SCD sub-device */
typedef u_int sbscd_offset;

/* type of an on-board device.  Matches table in sbscd.c */
enum sbscd_device_type {
	SBSCD_DEVTYPE_ICU = 0,		/* Interrupt controller/mapper */
	SBSCD_DEVTYPE_WDOG,		/* watchdog timer */
	SBSCD_DEVTYPE_TIMER,		/* general-purpose timer */
	SBSCD_DEVTYPE_JTAGCONS,		/* JTAG console (mem-mapped I/O) */
};

/* autoconfiguration match information for zbbus children */
struct sbscd_attach_locs {
	sbscd_offset		sa_addr;
	u_int			sa_intr[2];
	enum sbscd_device_type	sa_type;
};

/* XXX can probably just get away without separate sbscd_attach_locs ? */
struct sbscd_attach_args {
	struct sbscd_attach_locs sa_locs;
};
