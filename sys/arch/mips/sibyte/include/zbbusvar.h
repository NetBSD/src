/* $NetBSD: zbbusvar.h,v 1.2 2003/02/07 17:38:51 cgd Exp $ */

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

/* zbbus device number of a zbbus entity */
typedef u_int zbbus_busid;

/* type of a zbbus entity.  Matches table in sbmips/zbbus.c */
enum zbbus_entity_type {
	ZBBUS_ENTTYPE_CPU = 0,	/* a CPU core */
	ZBBUS_ENTTYPE_SCD,	/* SB1250 SCD */
	ZBBUS_ENTTYPE_BRZ,	/* SB1250 IO bridge 0 (PCI/LDT) */
	ZBBUS_ENTTYPE_OBIO,	/* SB1250 On-board I/O (IO bridge 1) */
};

/* autoconfiguration match information for zbbus children */
struct zbbus_attach_locs {
	zbbus_busid		za_busid;
	enum zbbus_entity_type	za_type;
};

struct zbbus_attach_args {
	struct zbbus_attach_locs za_locs;	/* locator match info */
};
