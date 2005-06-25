/*	$NetBSD: exec_macho.c,v 1.33 2005/06/25 02:22:57 christos Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exec_macho.c,v 1.33 2005/06/25 02:22:57 christos Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_macho.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <uvm/uvm.h>

#ifdef DEBUG_MACHO
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

static int exec_macho_load_segment(struct exec_package *, struct vnode *,
    u_long, struct exec_macho_segment_command *, int);
static int exec_macho_load_dylinker(struct proc *, struct exec_package *,
    struct exec_macho_dylinker_command *, u_long *, int);
static int exec_macho_load_dylib(struct proc *, struct exec_package *,
    struct exec_macho_dylib_command *, int);
static u_long exec_macho_load_thread(struct exec_macho_thread_command *);
static int exec_macho_load_file(struct proc *, struct exec_package *,
    const char *, u_long *, int, int, int);
static int exec_macho_load_vnode(struct proc *, struct exec_package *,
    struct vnode *, struct exec_macho_fat_header *, u_long *, int, int, int);

#ifdef DEBUG_MACHO
static void exec_macho_print_segment_command
    (struct exec_macho_segment_command *);
static void exec_macho_print_fat_header(struct exec_macho_fat_header *);
static void exec_macho_print_fat_arch(struct exec_macho_fat_arch *);
static void exec_macho_print_object_header(struct exec_macho_object_header *);
static void exec_macho_print_load_command(struct exec_macho_load_command *);
static void exec_macho_print_dylinker_command
    (struct exec_macho_dylinker_command *);
static void exec_macho_print_dylib_command(struct exec_macho_dylib_command *);
static void exec_macho_print_thread_command
    (struct exec_macho_thread_command *);

static void
exec_macho_print_segment_command(ls)
	struct exec_macho_segment_command *ls;
{
	printf("ls.cmd 0x%lx\n", ls->cmd);
	printf("ls.cmdsize 0x%ld\n", ls->cmdsize);
	printf("ls.segname %s\n", ls->segname);
	printf("ls.vmaddr 0x%lx\n", ls->vmaddr);
	printf("ls.vmsize %ld\n", ls->vmsize);
	printf("ls.fileoff 0x%lx\n", ls->fileoff);
	printf("ls.filesize %ld\n", ls->filesize);
	printf("ls.maxprot 0x%x\n", ls->maxprot);
	printf("ls.initprot 0x%x\n", ls->initprot);
	printf("ls.nsects %ld\n", ls->nsects);
	printf("ls.flags 0x%lx\n", ls->flags);
}

static void
exec_macho_print_fat_header(fat)
	struct exec_macho_fat_header *fat;
{
	printf("fat.magic 0x%x\n", be32toh(fat->magic));
	printf("fat.nfat_arch %d\n", be32toh(fat->nfat_arch));
}

static void
exec_macho_print_fat_arch(arch)
	struct exec_macho_fat_arch *arch;
{
	printf("arch.cputype %x\n", be32toh(arch->cputype));
	printf("arch.cpusubtype %d\n", be32toh(arch->cpusubtype));
	printf("arch.offset 0x%x\n", (int32_t)be32toh(arch->offset));
	printf("arch.size %d\n", (int32_t)be32toh(arch->size));
	printf("arch.align 0x%x\n", (int32_t)be32toh(arch->align));
}

static void
exec_macho_print_object_header(hdr)
	struct exec_macho_object_header *hdr;
{
	printf("hdr.magic 0x%lx\n", hdr->magic);
	printf("hdr.cputype %x\n", hdr->cputype);
	printf("hdr.cpusubtype %d\n", hdr->cpusubtype);
	printf("hdr.filetype 0x%lx\n", hdr->filetype);
	printf("hdr.ncmds %ld\n", hdr->ncmds);
	printf("hdr.sizeofcmds %ld\n", hdr->sizeofcmds);
	printf("hdr.flags 0x%lx\n", hdr->flags);
}

static void
exec_macho_print_load_command(lc)
	struct exec_macho_load_command *lc;
{
	printf("lc.cmd %lx\n", lc->cmd);
	printf("lc.cmdsize %ld\n", lc->cmdsize);
}

static void
exec_macho_print_dylinker_command(dy)
	struct exec_macho_dylinker_command *dy;
{
	printf("dy.cmd %lx\n", dy->cmd);
	printf("dy.cmdsize %ld\n", dy->cmdsize);
	printf("dy.name.offset 0x%lx\n", dy->name.offset);
	printf("dy.name %s\n", ((char *)dy) + dy->name.offset);
}

static void
exec_macho_print_dylib_command(dy)
	struct exec_macho_dylib_command *dy;
{
	printf("dy.cmd %lx\n", dy->cmd);
	printf("dy.cmdsize %ld\n", dy->cmdsize);
	printf("dy.dylib.name.offset 0x%lx\n", dy->dylib.name.offset);
	printf("dy.dylib.name %s\n", ((char *)dy) + dy->dylib.name.offset);
	printf("dy.dylib.timestamp %ld\n", dy->dylib.timestamp);
	printf("dy.dylib.current_version %ld\n", dy->dylib.current_version);
	printf("dy.dylib.compatibility_version %ld\n",
	    dy->dylib.compatibility_version);
}

static void
exec_macho_print_thread_command(th)
	struct exec_macho_thread_command *th;
{
	printf("th.cmd %lx\n", th->cmd);
	printf("th.cmdsize %ld\n", th->cmdsize);
	printf("th.flavor %ld\n", th->flavor);
	printf("th.count %ld\n", th->count);
}
#endif /* DEBUG_MACHO */

static int
exec_macho_load_segment(epp, vp, foff, ls, type)
	struct exec_package *epp;
	struct vnode *vp;
	u_long foff;
	struct exec_macho_segment_command *ls;
	int type;
{
	int flags;
	struct exec_macho_emul_arg *emea;
	u_long addr = trunc_page(ls->vmaddr), size = round_page(ls->filesize);

	emea = (struct exec_macho_emul_arg *)epp->ep_emul_arg;

	flags = VMCMD_BASE;

#ifdef DEBUG_MACHO
	exec_macho_print_segment_command(ls);
#endif
	if (strcmp(ls->segname, "__PAGEZERO") == 0)
		return 0;

	if (strcmp(ls->segname, "__TEXT") != 0 &&
	    strcmp(ls->segname, "__DATA") != 0 &&
	    strcmp(ls->segname, "__LOCK") != 0 &&
	    strcmp(ls->segname, "__OBJC") != 0 &&
	    strcmp(ls->segname, "__CGSERVER") != 0 &&
	    strcmp(ls->segname, "__IMAGE") != 0 &&
	    strcmp(ls->segname, "__LINKEDIT") != 0) {
		DPRINTF(("Unknown exec_macho segment %s\n", ls->segname));
		return ENOEXEC;
	}
	if (type == MACHO_MOH_EXECUTE) {
		if (strcmp(ls->segname, "__TEXT") == 0) {
			epp->ep_taddr = addr;
			epp->ep_tsize = round_page(ls->vmsize);
			emea->macho_hdr =
			    (struct exec_macho_object_header *)addr;
		}
		if ((strcmp(ls->segname, "__DATA") == 0) ||
		    (strcmp(ls->segname, "__OBJC") == 0) ||
		    (strcmp(ls->segname, "__IMAGE") == 0) ||
		    (strcmp(ls->segname, "__CGSERVER") == 0)) {
			epp->ep_daddr = addr;
			epp->ep_dsize = round_page(ls->vmsize);
		}
	}

	/*
	 * Some libraries do not have a load base address. The Darwin
	 * kernel seems to skip them, and dyld will do the job.
	 */
	if (addr == 0)
		return ENOMEM;

	if (ls->filesize > 0) {
		NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_pagedvn, size,
		    addr, vp, (off_t)(ls->fileoff + foff),
		    ls->initprot, flags);
		DPRINTF(("map(0x%lx, 0x%lx, %o, fd@ 0x%lx)\n",
		    addr, size, ls->initprot,
		    ls->fileoff + foff));
	}

	if (ls->vmsize > size) {
		addr += size;
		size = round_page(ls->vmsize - size);
		NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, size,
		    addr, vp, (off_t)(ls->fileoff + foff),
		    ls->initprot, flags);
		DPRINTF(("mmap(0x%lx, 0x%lx, %o, zero)\n",
		    ls->vmaddr + ls->filesize, ls->vmsize - ls->filesize,
		    ls->initprot));
	}
	return 0;
}


static int
exec_macho_load_dylinker(p, epp, dy, entry, depth)
	struct proc *p;
	struct exec_package *epp;
	struct exec_macho_dylinker_command *dy;
	u_long *entry;
	int depth;
{
	struct exec_macho_emul_arg *emea;
	const char *name = ((const char *)dy) + dy->name.offset;
	char path[MAXPATHLEN];
	int error;
#ifdef DEBUG_MACHO
	exec_macho_print_dylinker_command(dy);
#endif
	emea = (struct exec_macho_emul_arg *)epp->ep_emul_arg;

	(void)snprintf(path, sizeof(path), "%s%s", emea->path, name);
	DPRINTF(("loading linker %s\n", path));
	if ((error = exec_macho_load_file(p, epp, path, entry,
	    MACHO_MOH_DYLINKER, 1, depth)) != 0)
		return error;
	return 0;
}

static int
exec_macho_load_dylib(p, epp, dy, depth)
	struct proc *p;
	struct exec_package *epp;
	struct exec_macho_dylib_command *dy;
	int depth;
{
	struct exec_macho_emul_arg *emea;
	const char *name = ((const char *)dy) + dy->dylib.name.offset;
	char path[MAXPATHLEN];
	int error;
	u_long entry;
#ifdef DEBUG_MACHO
	exec_macho_print_dylib_command(dy);
#endif
	emea = (struct exec_macho_emul_arg *)epp->ep_emul_arg;
	(void)snprintf(path, sizeof(path), "%s%s", emea->path, name);
	DPRINTF(("loading library %s\n", path));
	if ((error = exec_macho_load_file(p, epp, path, &entry,
	    MACHO_MOH_DYLIB, 0, depth)) != 0)
		return error;
	return 0;
}

static u_long
exec_macho_load_thread(th)
	struct exec_macho_thread_command *th;
{
#ifdef DEBUG_MACHO
	exec_macho_print_thread_command(th);
#endif
	return exec_macho_thread_entry(th);
}

/*
 * exec_macho_load_file(): Load a macho-binary. This is used
 * for the dynamic linker and library recursive loading.
 */
static int
exec_macho_load_file(p, epp, path, entry, type, recursive, depth)
	struct proc *p;
	struct exec_package *epp;
	const char *path;
	u_long *entry;
	int type;
	int recursive;
	int depth;
{
	int error;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	struct exec_macho_fat_header fat;

	/*
	 * Check for excessive rercursive loading
	 */
	if (depth++ > 6)
		return E2BIG;

	/*
	 * 1. open file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return error;
	vp = nd.ni_vp;

	/*
	 * Similarly, if it's not marked as executable, or it's not a regular
	 * file, we don't allow it to be used.
	 */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto badunlock;
	}

	error = vn_marktext(vp);
	if (error)
		return (error);

	if ((error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p)) != 0)
		goto badunlock;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, &attr, p->p_ucred, p)) != 0)
		goto badunlock;

#ifdef notyet /* XXX cgd 960926 */
	XXX cgd 960926: (maybe) VOP_OPEN it (and VOP_CLOSE in copyargs?)
#endif
	VOP_UNLOCK(vp, 0);

	if ((error = exec_read_from(p, vp, 0, &fat, sizeof(fat))) != 0)
		goto bad;

	if ((error = exec_macho_load_vnode(p, epp, vp, &fat,
	    entry, type, recursive, depth)) != 0)
		goto bad;

	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0);

bad:
#ifdef notyet /* XXX cgd 960926 */
	(maybe) VOP_CLOSE it
#endif
	vrele(vp);
	return error;
}

/*
 * exec_macho_load_vnode(): Map a file from the given vnode.
 * The fat signature is checked,
 * and it will return the address of the entry point in entry.
 * The type determines what we are loading, a dynamic linker,
 * a dynamic library, or a binary. We use that to guess at
 * the entry point.
 */
static int
exec_macho_load_vnode(p, epp, vp, fat, entry, type, recursive, depth)
	struct proc *p;
	struct exec_package *epp;
	struct vnode *vp;
	struct exec_macho_fat_header *fat;
	u_long *entry;
	int type;
{
	u_long aoffs, offs;
	struct exec_macho_fat_arch arch;
	struct exec_macho_object_header hdr;
	struct exec_macho_load_command lc;
	struct exec_macho_emul_arg *emea;
	int error = ENOEXEC, i;
	size_t size;
	void *bf = &lc;
	u_int32_t *sc = NULL;

#ifdef DEBUG_MACHO
	exec_macho_print_fat_header(fat);
#endif

	switch (fat->magic) {
	case MACHO_FAT_MAGIC:
		for (i = 0; i < be32toh(fat->nfat_arch); i++, arch) {
			if ((error = exec_read_from(p, vp, sizeof(*fat) +
			    sizeof(arch) * i, &arch, sizeof(arch))) != 0)
				goto bad;
#ifdef DEBUG_MACHO
			exec_macho_print_fat_arch(&arch);
#endif
			for (sc = exec_macho_supported_cpu; *sc; sc++)
				if (*sc == be32toh(arch.cputype))
					break;

			if (sc != NULL)
				break;
		}
		if (sc == NULL || *sc == 0) {
			DPRINTF(("CPU %d not supported by this binary",
				be32toh(arch.cputype)));
			goto bad;
		}
		break;

	case MACHO_MOH_MAGIC:
		/*
		 * This is not a FAT Mach-O binary, the file starts
		 * with the object header.
		 */
		arch.offset = 0;
		break;

	default:
		DPRINTF(("bad exec_macho magic %x\n", fat->magic));
		goto bad;
		break;
	}

	if ((error = exec_read_from(p, vp, be32toh(arch.offset), &hdr,
	    sizeof(hdr))) != 0)
		goto bad;

	if (hdr.magic != MACHO_MOH_MAGIC) {
		DPRINTF(("bad exec_macho header magic %lx\n", hdr.magic));
		goto bad;
	}

#ifdef DEBUG_MACHO
	exec_macho_print_object_header(&hdr);
#endif
	switch (hdr.filetype) {
	case MACHO_MOH_PRELOAD:
	case MACHO_MOH_EXECUTE:
	case MACHO_MOH_DYLINKER:
	case MACHO_MOH_DYLIB:
	case MACHO_MOH_BUNDLE:
		break;
	default:
		DPRINTF(("Unsupported exec_macho filetype 0x%lx\n",
		    hdr.filetype));
		goto bad;
	}


	aoffs = be32toh(arch.offset);
	offs = aoffs + sizeof(hdr);
	size = sizeof(lc);
	for (i = 0; i < hdr.ncmds; i++) {
		if ((error = exec_read_from(p, vp, offs, &lc, sizeof(lc))) != 0)
			goto bad;

#ifdef DEBUG_MACHO
		exec_macho_print_load_command(&lc);
#endif
		if (size < lc.cmdsize) {
			if (lc.cmdsize > 4096) {
				DPRINTF(("Bad command size %ld\n", lc.cmdsize));
				goto bad;
			}
			if (bf != &lc)
				free(bf, M_TEMP);
			bf = malloc(size = lc.cmdsize, M_TEMP, M_WAITOK);
		}

		if ((error = exec_read_from(p, vp, offs, bf, lc.cmdsize)) != 0)
			goto bad;

		switch (lc.cmd) {
		case MACHO_LC_SEGMENT:
			error = exec_macho_load_segment(epp, vp, aoffs,
			    (struct exec_macho_segment_command *)bf, type);

			switch(error) {
			case ENOMEM:	/* Just skip, dyld will load it */
				DPRINTF(("load segment failed, skipping\n"));
				i = hdr.ncmds;
				break;
			case 0:		/* No error, carry on loading file */
				break;
			default:	/* Abort file load */
				DPRINTF(("load segment failed, aborting\n"));
				goto bad;
				break;
			}
			break;
		case MACHO_LC_LOAD_DYLINKER:
			if ((error = exec_macho_load_dylinker(p, epp,
			    (struct exec_macho_dylinker_command *)bf,
			    entry, depth)) != 0) {
				DPRINTF(("load linker failed\n"));
				goto bad;
			}
			emea = (struct exec_macho_emul_arg *)epp->ep_emul_arg;
			emea->dynamic = 1;
			break;
		case MACHO_LC_LOAD_DYLIB:
			/*
			 * We should only load libraries required by the
			 * binary we want to load, not libraries required
			 * by theses libraries.
			 */
			if (recursive == 0)
				break;
			if ((error = exec_macho_load_dylib(p, epp,
			    (struct exec_macho_dylib_command *)bf,
			    depth)) != 0) {
				DPRINTF(("load dylib failed\n"));
				goto bad;
			}
			break;

		case MACHO_LC_THREAD:
		case MACHO_LC_UNIXTHREAD:
			if (type == MACHO_MOH_DYLINKER || *entry == 0) {
				*entry = exec_macho_load_thread(
				    (struct exec_macho_thread_command *)bf);
			} else {
				(void)exec_macho_load_thread(
				    (struct exec_macho_thread_command *)bf);
			}
			break;

		case MACHO_LC_ID_DYLINKER:
		case MACHO_LC_ID_DYLIB:
		case MACHO_LC_SYMTAB:
		case MACHO_LC_DYSYMTAB:
			break;
		default:
			DPRINTF(("Unhandled exec_macho command 0x%lx\n",
			    lc.cmd));
			break;
		}
		offs += lc.cmdsize;
	}
	error = 0;
bad:
	if (bf != &lc)
		free(bf, M_TEMP);
	return error;
}

/*
 * exec_macho_makecmds(): Prepare an Mach-O binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */
int
exec_macho_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec_macho_fat_header *fat = epp->ep_hdr;
	struct exec_macho_emul_arg *emea;
	int error;

	if (epp->ep_hdrvalid < sizeof(*fat))
		return ENOEXEC;

	/*
	 * Check mount point.  Though we're not trying to exec this binary,
	 * we will be executing code from it, so if the mount point
	 * disallows execution or set-id-ness, we punt or kill the set-id.
	 */
	if (epp->ep_vp->v_mount->mnt_flag & MNT_NOEXEC)
		return EACCES;

	if (epp->ep_vp->v_mount->mnt_flag & MNT_NOSUID)
		epp->ep_vap->va_mode &= ~(S_ISUID | S_ISGID);

	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	emea = malloc(sizeof(struct exec_macho_emul_arg), M_EXEC, M_WAITOK);
	epp->ep_emul_arg = (void *)emea;
	emea->dynamic = 0;

	if (!epp->ep_esch->u.mach_probe_func)
		emea->path = "/";
	else {
	    if ((error = (*epp->ep_esch->u.mach_probe_func)(&emea->path)) != 0)
		    goto bad2;
	}

	/*
	 * Make sure the underlying functions will not get
	 * a random value here. 0 means that no entry point
	 * has been found yet.
	 */
	epp->ep_entry = 0;

	if ((error = exec_macho_load_vnode(p, epp, epp->ep_vp, fat,
	    &epp->ep_entry, MACHO_MOH_EXECUTE, 1, 0)) != 0)
		goto bad;

	/*
	 * stash a copy of the program name in epp->ep_emul_arg because
	 * we will need it later.
	 */
	if ((error = copyinstr(epp->ep_name, emea->filename,
	    MAXPATHLEN, NULL)) != 0) {
		DPRINTF(("Copyinstr %p failed\n", epp->ep_name));
		goto bad;
	}

	return (*epp->ep_esch->es_setup_stack)(p, epp);
bad:
	kill_vmcmds(&epp->ep_vmcmds);
bad2:
	free(emea, M_EXEC);
	return error;
}
