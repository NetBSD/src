/*	$NetBSD: gscbusvar.h,v 1.7 2009/05/08 09:33:58 skrll Exp $	*/

/*	$OpenBSD: gscbusvar.h,v 1.3 1999/08/16 02:48:39 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <hp700/hp700/intr.h>

struct gsc_attach_args {
	struct confargs ga_ca;
#define	ga_name		ga_ca.ca_name
#define	ga_dp		ga_ca.ca_dp
#define	ga_iot		ga_ca.ca_iot
#define	ga_mod		ga_ca.ca_mod
#define	ga_type		ga_ca.ca_type
#define	ga_hpa		ga_ca.ca_hpa
#define	ga_dmatag	ga_ca.ca_dmatag
#define	ga_irq		ga_ca.ca_irq
/*#define	ga_pdc_iodc_read	ga_ca.ca_pdc_iodc_read */

	/* The interrupt register for this GSC bus. */
	struct hp700_int_reg *ga_int_reg;

	/* This fixes a module's attach arguments. */
	void (*ga_fix_args)(void *, struct gsc_attach_args *);
	void *ga_fix_args_cookie;

	/* The SCSI target for the host adapter. */
	int	ga_scsi_target;

	/* The address for the Ethernet adapter. */
	u_int8_t ga_ether_address[6];
};

int gscprint(void *, const char *);
