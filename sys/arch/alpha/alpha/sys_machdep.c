/* $NetBSD: sys_machdep.c,v 1.16.32.1 2007/12/26 19:41:54 ad Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sys_machdep.c,v 1.16.32.1 2007/12/26 19:41:54 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/fpu.h>
#include <machine/sysarch.h>

#include <dev/pci/pcivar.h>

u_int	alpha_bus_window_count[ALPHA_BUS_TYPE_MAX + 1];

int	(*alpha_bus_get_window)(int, int,
	    struct alpha_bus_space_translation *);

struct alpha_pci_chipset *alpha_pci_chipset;

int
sys_sysarch(struct lwp *l, const struct sys_sysarch_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(void *) parms;
	} */
	int error = 0;

	switch(SCARG(uap, op)) {
	case ALPHA_FPGETMASK:
		*retval = FP_C_TO_NETBSD_MASK(l->l_md.md_flags);
		break;
	case ALPHA_FPGETSTICKY:
		*retval = FP_C_TO_NETBSD_FLAG(l->l_md.md_flags);
		break;
	case ALPHA_FPSETMASK:
	case ALPHA_FPSETSTICKY:
	    {
		fp_except m;
		u_int64_t md_flags;
		struct alpha_fp_except_args args;

		error = copyin(SCARG(uap, parms), &args, sizeof args);
		if (error)
			return error;
		m = args.mask;
		md_flags = l->l_md.md_flags;
		switch (SCARG(uap, op)) {
		case ALPHA_FPSETMASK:
			*retval = FP_C_TO_NETBSD_MASK(md_flags);
			md_flags = SET_FP_C_MASK(md_flags, m);
			break;
		case ALPHA_FPSETSTICKY:
			*retval = FP_C_TO_NETBSD_FLAG(md_flags);
			md_flags = SET_FP_C_FLAG(md_flags, m);
			break;
		}
		alpha_write_fp_c(l, md_flags);
		break;
	    }
	case ALPHA_GET_FP_C:
	    {
		struct alpha_fp_c_args args;

		args.fp_c = alpha_read_fp_c(l);
		error = copyout(&args, SCARG(uap, parms), sizeof args);
		break;
	    }
	case ALPHA_SET_FP_C:
	    {
		struct alpha_fp_c_args args;

		error = copyin(SCARG(uap, parms), &args, sizeof args);
		if (error)
			return (error);
		if ((args.fp_c >> 63) != 0)
			args.fp_c |= IEEE_INHERIT;
		alpha_write_fp_c(l, args.fp_c);
		break;
	    }
	case ALPHA_BUS_GET_WINDOW_COUNT:
	    {
		struct alpha_bus_get_window_count_args args;

		error = copyin(SCARG(uap, parms), &args, sizeof(args));
		if (error)
			return (error);

		if (args.type > ALPHA_BUS_TYPE_MAX)
			return (EINVAL);

		if (alpha_bus_window_count[args.type] == 0)
			return (EOPNOTSUPP);

		args.count = alpha_bus_window_count[args.type];
		error = copyout(&args, SCARG(uap, parms), sizeof(args));
		break;
	    }

	case ALPHA_BUS_GET_WINDOW:
	    {
		struct alpha_bus_space_translation abst;
		struct alpha_bus_get_window_args args;

		error = copyin(SCARG(uap, parms), &args, sizeof(args));
		if (error)
			return (error);

		if (args.type > ALPHA_BUS_TYPE_MAX)
			return (EINVAL);

		if (alpha_bus_window_count[args.type] == 0)
			return (EOPNOTSUPP);

		if (args.window >= alpha_bus_window_count[args.type])
			return (EINVAL);

		error = (*alpha_bus_get_window)(args.type, args.window,
		    &abst);
		if (error)
			return (error);

		error = copyout(&abst, args.translation, sizeof(abst));
		break;
	    }

	case ALPHA_PCI_CONF_READWRITE:
	    {
		struct alpha_pci_conf_readwrite_args args;
		pcitag_t tag;

		error = copyin(SCARG(uap, parms), &args, sizeof(args));
		if (error)
			return (error);

		if (alpha_pci_chipset == NULL)
			return (EOPNOTSUPP);

		if (args.bus > 0xff)		/* XXX MAGIC NUMBER */
			return (EINVAL);
		if (args.device > pci_bus_maxdevs(alpha_pci_chipset, args.bus))
			return (EINVAL);
		if (args.function > 7)		/* XXX MAGIC NUMBER */
			return (EINVAL);
		if (args.reg > 0xff)		/* XXX MAGIC NUMBER */
			return (EINVAL);

		tag = pci_make_tag(alpha_pci_chipset, args.bus, args.device,
		    args.function);

		if (args.write)
			pci_conf_write(alpha_pci_chipset, tag, args.reg,
			    args.val);
		else {
			args.val = pci_conf_read(alpha_pci_chipset, tag,
			    args.reg);
			error = copyout(&args, SCARG(uap, parms), sizeof(args));
		}
		break;
	    }

	default:
		error = EINVAL;
		break;
	}

	return (error);
}
