/*	$NetBSD: exec_ecoff.h,v 1.1 2002/03/23 17:13:46 bouyer Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass
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
 *      This product includes software developed by Adam Glass.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef	_SYS_EXEC_ECOFF_H_
#define	_SYS_EXEC_ECOFF_H_

#include <machine/ecoff_machdep.h>

struct ecoff_filehdr {
	ECOFF_USHORT f_magic;	/* magic number */
	ECOFF_USHORT f_nscns;	/* # of sections */
	ECOFF_UINT   f_timdat;	/* time and date stamp */
	ECOFF_ULONG  f_symptr;	/* file offset of symbol table */
	ECOFF_UINT   f_nsyms;	/* # of symbol table entries */
	ECOFF_USHORT f_opthdr;	/* sizeof the optional header */
	ECOFF_USHORT f_flags;	/* flags??? */
};

struct ecoff_aouthdr {
	ECOFF_USHORT magic;
	ECOFF_USHORT vstamp;
	ECOFF_PAD
	ECOFF_ULONG  tsize;
	ECOFF_ULONG  dsize;
	ECOFF_ULONG  bsize;
	ECOFF_ULONG  entry;
	ECOFF_ULONG  text_start;
	ECOFF_ULONG  data_start;
	ECOFF_ULONG  bss_start;
	ECOFF_MACHDEP;
};

struct ecoff_scnhdr {		/* needed for size info */
	char	s_name[8];	/* name */
	ECOFF_ULONG  s_paddr;	/* physical addr? for ROMing?*/
	ECOFF_ULONG  s_vaddr;	/* virtual addr? */
	ECOFF_ULONG  s_size;	/* size */
	ECOFF_ULONG  s_scnptr;	/* file offset of raw data */
	ECOFF_ULONG  s_relptr;	/* file offset of reloc data */
	ECOFF_ULONG  s_lnnoptr;	/* file offset of line data */
	ECOFF_USHORT s_nreloc;	/* # of relocation entries */
	ECOFF_USHORT s_nlnno;	/* # of line entries */
	ECOFF_UINT   s_flags;	/* flags */
};

struct ecoff_exechdr {
	struct ecoff_filehdr f;
	struct ecoff_aouthdr a;
};

#define ECOFF_HDR_SIZE (sizeof(struct ecoff_exechdr))

#define ECOFF_OMAGIC 0407
#define ECOFF_NMAGIC 0410
#define ECOFF_ZMAGIC 0413

#define ECOFF_ROUND(value, by) \
        (((value) + (by) - 1) & ~((by) - 1))

#define ECOFF_BLOCK_ALIGN(ep, value) \
        ((ep)->a.magic == ECOFF_ZMAGIC ? ECOFF_ROUND((value), ECOFF_LDPGSZ) : \
	 (value))

#define ECOFF_TXTOFF(ep) \
        ((ep)->a.magic == ECOFF_ZMAGIC ? 0 : \
	 ECOFF_ROUND(ECOFF_HDR_SIZE + (ep)->f.f_nscns * \
		     sizeof(struct ecoff_scnhdr), ECOFF_SEGMENT_ALIGNMENT(ep)))

#define ECOFF_DATOFF(ep) \
        (ECOFF_BLOCK_ALIGN((ep), ECOFF_TXTOFF(ep) + (ep)->a.tsize))

#define ECOFF_SEGMENT_ALIGN(ep, value) \
        (ECOFF_ROUND((value), ((ep)->a.magic == ECOFF_ZMAGIC ? ECOFF_LDPGSZ : \
         ECOFF_SEGMENT_ALIGNMENT(ep))))

#ifdef _KERNEL
int	exec_ecoff_makecmds __P((struct proc *, struct exec_package *));
int	exec_ecoff_setup_stack __P((struct proc *, struct exec_package *));
int	cpu_exec_ecoff_probe __P((struct proc *, struct exec_package *));
void	cpu_exec_ecoff_setregs __P((struct proc *, struct exec_package *,
	    u_long));

int	exec_ecoff_prep_omagic __P((struct proc *, struct exec_package *,
	    struct ecoff_exechdr *, struct vnode *));
int	exec_ecoff_prep_nmagic __P((struct proc *, struct exec_package *,
	    struct ecoff_exechdr *, struct vnode *));
int	exec_ecoff_prep_zmagic __P((struct proc *, struct exec_package *,
	    struct ecoff_exechdr *, struct vnode *));

#endif /* _KERNEL */
#endif /* !_SYS_EXEC_ECOFF_H_ */
