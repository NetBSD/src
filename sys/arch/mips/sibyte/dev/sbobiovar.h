/* $NetBSD: sbobiovar.h,v 1.1.10.2 2002/06/23 17:38:07 jdolecek Exp $ */

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
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
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

/* sbobio pseudo-offset (from base) of an on-board device*/
typedef u_int sbobio_offset;

/* type of an on-board device.  Matches table in sbobio.c */
enum sbobio_device_type {
	SBOBIO_DEVTYPE_SMBUS = 0,	/* SMBus interface */
	SBOBIO_DEVTYPE_DUART,		/* scn2681-ish DUART */
	SBOBIO_DEVTYPE_SYNCSERIAL,	/* synchronous serial port */
	SBOBIO_DEVTYPE_GBUS,		/* generic bus */
	SBOBIO_DEVTYPE_MAC,		/* Ethernet MAC */
};

/* autoconfiguration match information for zbbus children */
struct sbobio_attach_locs {
	sbobio_offset		sa_offset;
	u_int			sa_intr[2];
	enum sbobio_device_type	sa_type;
};

struct sbobio_attach_args {
	struct sbobio_attach_locs sa_locs;

	long			sa_base;
};
