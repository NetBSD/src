/*********************************************************************
 *      NetBSD(SH3) boot loader 
 * 
 *                1998.11.10
 *                     By T.Horiuchi (Brains, Corp.)
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/gmon.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <vm/vm_param.h>
#include <machine/cpu.h>
#include "osloader.h"

#define vm_page_size (1<<12)

char *netbsd = "/netbsd";
struct coff_filehdr FileHdr;
struct coff_aouthdr AoutHdr;

static int coff_find_section __P((FILE *, struct coff_filehdr *,
			     struct coff_scnhdr *, int));

void LoadAndReset __P((char *));

/*
 * coff_find_section - load specified section header
 *
 * TODO - optimize by reading all section headers in at once
 */

static int
coff_find_section( fd, fp, sh, s_type)
	FILE *fd;
	struct coff_filehdr *fp;
	struct coff_scnhdr *sh;
	int s_type;
{
	int i, pos, siz;
	
	pos = COFF_HDR_SIZE;
	for (i = 0; i < fp->f_nscns; i++, pos += sizeof(struct coff_scnhdr)) {
		siz = sizeof(struct coff_scnhdr);
		if( fread(sh, 1, siz, fd ) != siz ){
			perror("osloader");
			exit( 1 );
		}

		if (sh->s_flags == s_type)
			return 0;
	}
	return -1;
}

void
LoadAndReset( char *osimage )
{
	int mib[2];
	u_long val;
	int len;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_LOADANDRESET;
	val = (u_long)osimage;
	len = sizeof( val );

	sysctl(mib, 2, NULL, NULL, &val, len);
}

int 
main(int argc, char *argv[] )
{
	FILE *fp;
	int error;
	long dsize;
	struct coff_scnhdr sh;
	u_long ep_taddr;
	u_long ep_tsize;
	u_long toffset;
	u_long ep_daddr;
	u_long ep_dsize;
	u_long doffset;
	char *osimage;
	char *p;
	int i;
	u_long cksum;
	u_long size;

#if 0
	printf("osloader: start\n");
#endif

	fp = fopen( netbsd, "r" );
	if( fp == NULL ){
		perror("osloader");
		exit( 1 );
	}

	if( fread( &FileHdr, 1, sizeof( FileHdr ), fp ) != sizeof(FileHdr)){
		perror("osloader");
		exit( 1 );
	}

	if(fread( &AoutHdr, 1, sizeof( AoutHdr ), fp ) != sizeof(AoutHdr)){
		perror("osloader");
		exit( 1 );
	}

	/* set up command for text segment */
	error = coff_find_section(fp, &FileHdr, &sh, COFF_STYP_TEXT);
	if (error) {		
		printf("can't find text section: %d\n", error);
		exit( 1 );
	}

	ep_taddr = COFF_ALIGN(sh.s_vaddr);
	toffset = sh.s_scnptr - (sh.s_vaddr - ep_taddr);
	ep_tsize = sh.s_size + (sh.s_vaddr - ep_taddr);

	printf("VMCMD: addr %lx size 0x%lx offset 0x%lx\n", ep_taddr,
	       ep_tsize, toffset); 

	/* set up command for data segment */
	error = coff_find_section(fp, &FileHdr, &sh, COFF_STYP_DATA);
	if (error) {
		printf("can't find data section: %d\n", error);
		exit( 1 );
	}

	ep_daddr = COFF_ALIGN(sh.s_vaddr);
	doffset = sh.s_scnptr - (sh.s_vaddr - ep_daddr);
	dsize = sh.s_size + (sh.s_vaddr - ep_daddr);
	ep_dsize = round_page(dsize) + AoutHdr.a_bsize;

	printf("VMCMD: addr 0x%lx size 0x%lx offset 0x%lx\n", ep_daddr,
		 dsize, doffset); 

	osimage = malloc( ep_tsize+dsize+sizeof(u_long)*2 );
	if( osimage == NULL){
		printf("osloader:no memory\n");
		exit( 1 );
	}

	*(u_long *)osimage = ep_tsize+dsize;
	p = osimage + 2*sizeof( u_long );
	
	/* load text area */
	fseek( fp, toffset, SEEK_SET );
	if( fread(p, 1, ep_tsize, fp) != ep_tsize ){
		perror("osloader:");
		exit( 1 );
	}

	/* load data area */
	fseek( fp, doffset, SEEK_SET );
	if( fread(p+ep_daddr-ep_taddr, 1, dsize, fp) != dsize ){
		perror("osloader:");
		exit( 1 );
	}

	fclose( fp );

	cksum = 0;
	size = (ep_tsize + dsize) >> 2;

	for( i = 0; i < size; i++){
		cksum += *(u_long *)p;
		p += sizeof(u_long );
	}

	*(u_long *)(osimage+sizeof(u_long)) = cksum ;

#if 0
	printf("osimage = %p\n", osimage );
#endif

	LoadAndReset( osimage );

	return (0);	/* NOT REACHED */
}


#ifdef NOTDEF

/*
 * exec_coff_makecmds(): Check if it's an coff-format executable.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in coff format.  Check 'standard' magic numbers for
 * this architecture.  If that fails, return failure.
 *
 * This function is  responsible for creating a set of vmcmds which can be
 * used to build the process's vm space and inserting them into the exec
 * package.
 */

int
exec_coff_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error;
	struct coff_filehdr *fp = epp->ep_hdr;
	struct coff_aouthdr *ap;

	if (epp->ep_hdrvalid < COFF_HDR_SIZE)
		return ENOEXEC;

	if (COFF_BADMAG(fp))
		return ENOEXEC;
	
	ap = epp->ep_hdr + sizeof(struct coff_filehdr);
	switch (ap->a_magic) {
	case COFF_OMAGIC:
		error = exec_coff_prep_omagic(p, epp, fp, ap);
		break;
	case COFF_NMAGIC:
		error = exec_coff_prep_nmagic(p, epp, fp, ap);
		break;
	case COFF_ZMAGIC:
		error = exec_coff_prep_zmagic(p, epp, fp, ap);
		break;
	default:
		return ENOEXEC;
	}

#ifdef	TODO
	if (error == 0)
		error = cpu_exec_coff_hook(p, epp );
#endif

	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * exec_coff_setup_stack(): Set up the stack segment for a coff
 * executable.
 *
 * Note that the ep_ssize parameter must be set to be the current stack
 * limit; this is adjusted in the body of execve() to yield the
 * appropriate stack segment usage once the argument length is
 * calculated.
 *
 * This function returns an int for uniformity with other (future) formats'
 * stack setup functions.  They might have errors to return.
 */

int
exec_coff_setup_stack(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	/* DPRINTF(("enter exec_coff_setup_stack\n")); */

	epp->ep_maxsaddr = USRSTACK - MAXSSIZ;
	epp->ep_minsaddr = USRSTACK;
	epp->ep_ssize = p->p_rlimit[RLIMIT_STACK].rlim_cur;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 *
	 * note that in memory, things assumed to be: 0 ....... ep_maxsaddr
	 * <stack> ep_minsaddr
	 */
	/* DPRINTF(("VMCMD: addr %x size %d\n", epp->ep_maxsaddr,
		 (epp->ep_minsaddr - epp->ep_ssize) - epp->ep_maxsaddr)); */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
		  ((epp->ep_minsaddr - epp->ep_ssize) - epp->ep_maxsaddr),
		  epp->ep_maxsaddr, NULLVP, 0, VM_PROT_NONE);
	/* DPRINTF(("VMCMD: addr %x size %d\n",
		 epp->ep_minsaddr - epp->ep_ssize,
		 epp->ep_ssize)); */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, epp->ep_ssize,
		  (epp->ep_minsaddr - epp->ep_ssize), NULLVP, 0,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return 0;
}


/*
 * exec_coff_prep_omagic(): Prepare a COFF OMAGIC binary's exec package
 */

int
exec_coff_prep_omagic(p, epp, fp, ap)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	epp->ep_taddr = COFF_SEGMENT_ALIGN(ap, ap->a_tstart);
	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = COFF_SEGMENT_ALIGN(ap, ap->a_dstart);
	epp->ep_dsize = ap->a_dsize;
	epp->ep_entry = ap->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
		  ap->a_tsize + ap->a_dsize, epp->ep_taddr, epp->ep_vp,
		  COFF_TXTOFF(fp, ap),
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
#ifdef	sh3
	if (ap->a_bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
			  COFF_ROUND(ap->a_dstart + ap->a_dsize, COFF_LDPGSZ),
			  NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#else
	if (ap->a_bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
			  COFF_SEGMENT_ALIGN(ap, ap->a_dstart + ap->a_dsize),
			  NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#endif
	
	return exec_coff_setup_stack(p, epp);
}

/*
 * exec_coff_prep_nmagic(): Prepare a 'native' NMAGIC COFF binary's exec
 *                          package.
 */

int
exec_coff_prep_nmagic(p, epp, fp, ap)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	epp->ep_taddr = COFF_SEGMENT_ALIGN(ap, ap->a_tstart);
	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = COFF_ROUND(ap->a_dstart, COFF_LDPGSZ);
	epp->ep_dsize = ap->a_dsize;
	epp->ep_entry = ap->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_tsize,
		  epp->ep_taddr, epp->ep_vp, COFF_TXTOFF(fp, ap),
		  VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_dsize,
		  epp->ep_daddr, epp->ep_vp, COFF_DATOFF(fp, ap),
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	if (ap->a_bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
			  COFF_SEGMENT_ALIGN(ap, ap->a_dstart + ap->a_dsize),
			  NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_coff_setup_stack(p, epp);
}

/*
 * coff_find_section - load specified section header
 *
 * TODO - optimize by reading all section headers in at once
 */

static int
coff_find_section(p, vp, fp, sh, s_type)
	struct proc *p;
	struct vnode *vp;
	struct coff_filehdr *fp;
	struct coff_scnhdr *sh;
	int s_type;
{
	int i, pos, resid, siz, error;
	
	pos = COFF_HDR_SIZE;
	for (i = 0; i < fp->f_nscns; i++, pos += sizeof(struct coff_scnhdr)) {
		siz = sizeof(struct coff_scnhdr);
		error = vn_rdwr(UIO_READ, vp, (caddr_t) sh,
		    siz, pos, UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
		    &resid, p);
		if (error) {
			DPRINTF(("section hdr %d read error %d\n", i, error));
			return error;
		}
		siz -= resid;
		if (siz != sizeof(struct coff_scnhdr)) {
			DPRINTF(("incomplete read: hdr %d ask=%d, rem=%d got %d\n",
				 s_type, sizeof(struct coff_scnhdr),
				 resid, siz));
			return ENOEXEC;
		}
		/* DPRINTF(("found section: %x\n", sh->s_flags)); */
		if (sh->s_flags == s_type)
			return 0;
	}
	return ENOEXEC;
}

/*
 * exec_coff_prep_zmagic(): Prepare a COFF ZMAGIC binary's exec package
 *
 * First, set the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
exec_coff_prep_zmagic(p, epp, fp, ap)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	int error;
	u_long offset;
	long dsize;
#ifndef	sh3
	long  baddr, bsize;
#endif
	struct coff_scnhdr sh;
	
	/* DPRINTF(("enter exec_coff_prep_zmagic\n")); */

	/* set up command for text segment */
	error = coff_find_section(p, epp->ep_vp, fp, &sh, COFF_STYP_TEXT);
	if (error) {		
		DPRINTF(("can't find text section: %d\n", error));
		return error;
	}
	/* DPRINTF(("COFF text addr %x size %d offset %d\n", sh.s_vaddr,
		 sh.s_size, sh.s_scnptr)); */
	epp->ep_taddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - epp->ep_taddr);
	epp->ep_tsize = sh.s_size + (sh.s_vaddr - epp->ep_taddr);

#ifdef notyet
	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((ap->a_tsize != 0 || ap->a_dsize != 0) &&
	    epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;
#endif
	
	/* DPRINTF(("VMCMD: addr %x size %d offset %d\n", epp->ep_taddr,
		 epp->ep_tsize, offset)); */
#ifdef notyet
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, epp->ep_tsize,
		  epp->ep_taddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_EXECUTE);
#else
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_tsize,
		  epp->ep_taddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_EXECUTE);
#endif

	/* set up command for data segment */
	error = coff_find_section(p, epp->ep_vp, fp, &sh, COFF_STYP_DATA);
	if (error) {
		DPRINTF(("can't find data section: %d\n", error));
		return error;
	}
	/* DPRINTF(("COFF data addr %x size %d offset %d\n", sh.s_vaddr,
		 sh.s_size, sh.s_scnptr)); */
	epp->ep_daddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - epp->ep_daddr);
	dsize = sh.s_size + (sh.s_vaddr - epp->ep_daddr);
#ifdef sh3
	epp->ep_dsize = round_page(dsize) + ap->a_bsize;
#else
	epp->ep_dsize = dsize + ap->a_bsize;
#endif

	/* DPRINTF(("VMCMD: addr %x size %d offset %d\n", epp->ep_daddr,
		 dsize, offset)); */
#ifdef notyet
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, dsize,
		  epp->ep_daddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#else
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
		  dsize, epp->ep_daddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#endif

#ifdef	sh3
	if (ap->a_bsize > 0){
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
			  COFF_ROUND(ap->a_dstart + ap->a_dsize, COFF_LDPGSZ),
			  NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
		
	}
#else
	/* set up command for bss segment */
	baddr = round_page(epp->ep_daddr + dsize);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0) {
		/* DPRINTF(("VMCMD: addr %x size %d offset %d\n",
			 baddr, bsize, 0)); */
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
			  bsize, baddr, NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}
#endif

#ifdef	TODO
	/* load any shared libraries */
	error = coff_find_section(p, epp->ep_vp, fp, &sh, COFF_STYP_SHLIB);
	if (!error) {
		int resid;
		struct coff_slhdr *slhdr;
		char buf[128], *bufp;	/* FIXME */
		int len = sh.s_size, path_index, entry_len;
		
		/* DPRINTF(("COFF shlib size %d offset %d\n",
			 sh.s_size, sh.s_scnptr)); */

		error = vn_rdwr(UIO_READ, epp->ep_vp, (caddr_t) buf,
				len, sh.s_scnptr,
				UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
				&resid, p);
		if (error) {
			DPRINTF(("shlib section read error %d\n", error));
			return ENOEXEC;
		}
		bufp = buf;
		while (len) {
			slhdr = (struct coff_slhdr *)bufp;
			path_index = slhdr->path_index * sizeof(long);
			entry_len = slhdr->entry_len * sizeof(long);

			/* DPRINTF(("path_index: %d entry_len: %d name: %s\n",
				 path_index, entry_len, slhdr->sl_name)); */

			error = coff_load_shlib(p, slhdr->sl_name, epp);
			if (error)
				return ENOEXEC;
			bufp += entry_len;
			len -= entry_len;
		}
	}
#endif
		
	/* set up entry point */
	epp->ep_entry = ap->a_entry;

#if 0
	DPRINTF(("text addr: %x size: %d data addr: %x size: %d entry: %x\n",
		 epp->ep_taddr, epp->ep_tsize,
		 epp->ep_daddr, epp->ep_dsize,
		 epp->ep_entry));
#endif
	
	return exec_coff_setup_stack(p, epp);
}


#endif /* NOTDEF */
