/* $NetBSD: nlist_coff.c,v 1.2.2.1 2000/06/22 18:01:02 minoura Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nlist_coff.c,v 1.2.2.1 2000/06/22 18:01:02 minoura Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <a.out.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#ifdef NLIST_COFF
#include <sys/exec_coff.h>

typedef struct nlist NLIST;
#define	_strx	n_un.n_strx
#define	_name	n_un.n_name

#define	badfmt(str)							\
	do {								\
		warnx("%s: %s: %s", kfile, str, strerror(EFTYPE));	\
		punt();							\
	} while (0)

#define	check(off, size)	((off < 0) || (off + size > mappedsize))
#define	BAD			do { rv = -1; goto out; } while (0)
#define	BADUNMAP		do { rv = -1; goto unmap; } while (0)

static const char *kfile;

int
create_knlist_coff(name, db)
	const char *name;
	DB *db;
{
	struct coff_filehdr *filehdrp;
	struct coff_aouthdr *aouthdrp;
	struct stat st;
	struct nlist nbuf;
	DBT key, data;
	char *mappedfile, *symname, *fsymname;
	size_t mappedsize, symnamesize, fsymnamesize;
	u_long symhdroff, extrstroff;
	u_long symhdrsize, i, nesyms;
	int fd, rv;
	struct external_syment *syment;
	u_long soff;
	u_long val;
	char snamebuf[16];

	rv = -1;

	/*
	 * Open and map the whole file.  If we can't open/stat it,
	 * something bad is going on so we punt.
	 */
	kfile = name;
	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		warn("%s", kfile);
		punt();
	}
	if (fstat(fd, &st) < 0) {
		warn("%s", kfile);
		punt();
	}
	if (st.st_size > SIZE_T_MAX)
		BAD;

	/*
	 * Map the file in its entirety.
	 */
	mappedsize = st.st_size;
	mappedfile = mmap(NULL, mappedsize, PROT_READ, MAP_FILE|MAP_PRIVATE,
			  fd, 0);
	if (mappedfile == (char *)-1)
		BAD;

	/*
	 * Make sure we can access the executable's header
	 * directly, and make sure the recognize the executable
	 * as an COFF binary.
	 */
	if (check(0, sizeof *filehdrp))
		BADUNMAP;
	filehdrp = (struct coff_filehdr *)&mappedfile[0];

	if (COFF_BADMAG(filehdrp))
		BADUNMAP;

	/*
	 * We've recognized it as an COFF binary.  From here
	 * on out, all errors are fatal.
	 */

	aouthdrp = (struct coff_aouthdr *)
		&mappedfile[sizeof(struct coff_filehdr)];

	/*
	 * Find the symbol list and string table.
	 */
	symhdroff = filehdrp->f_symptr;
	symhdrsize = filehdrp->f_nsyms;
	extrstroff = symhdroff + symhdrsize*COFF_ES_SYMENTSZ;

#ifdef DEGBU
	printf("sizeof syment = %d\n",sizeof(struct external_syment ));
	printf("symhdroff = 0x%lx,symhdrsize=%ld,stroff = 0x%lx",
	       symhdroff,symhdrsize, extrstroff);
#endif

	if (symhdrsize == 0)
		badfmt("stripped");

	/*
	 * Set up the data item, pointing to a nlist structure.
	 * which we fill in for each symbol.
	 */
	data.data = (u_char *)&nbuf;
	data.size = sizeof(nbuf);

	/*
	 * Create a buffer (to be expanded later, if necessary)
	 * to hold symbol names after we've added underscores
	 * to them.
	 */
	symnamesize = 1024;
	if ((symname = malloc(symnamesize)) == NULL) {
		warn("malloc");
		punt();
	}

	nesyms = filehdrp->f_nsyms;

	/*
	 * Read each symbol and enter it into the database.
	 */
	for (i = 0; i < nesyms; i++) {

		/*
		 * Find symbol name, copy it (with added underscore) to
		 * temporary buffer, and prepare the database key for
		 * insertion.
		 */
		syment = (struct external_syment *)&mappedfile[symhdroff + 
				       COFF_ES_SYMENTSZ*i];

		if(syment->e_sclass[0] != 2){
			continue;
		}

		if(syment->e.e.e_zeroes[0]){
			if( syment->e.e_name[COFF_ES_SYMNMLEN-1] ){
				memcpy( snamebuf, syment->e.e_name, 
				       COFF_ES_SYMNMLEN);
				snamebuf[COFF_ES_SYMNMLEN] = '\0';
				fsymname = snamebuf;
			}
			else{
				fsymname = syment->e.e_name ;
			}
			fsymnamesize = strlen(fsymname) + 1;
			
#ifdef DEBUG
			printf("%s\n",fsymname ); 
#endif
		}
		else{
			memcpy(&soff, syment->e.e.e_offset, sizeof(long));
			fsymname = &mappedfile[extrstroff+soff];
			fsymnamesize = strlen(fsymname) + 1;

#ifdef DEBUG
			printf("*%s\n",fsymname ); 
#endif
		}
		
		while (symnamesize < fsymnamesize + 1) {
			symnamesize *= 2;
			if ((symname = realloc(symname, symnamesize)) == NULL){
				warn("malloc");
				punt();
			}
		}
#if 0
		strcpy(symname, "_"); 
		strcat(symname, fsymname);
#else
		strcpy(symname, fsymname);
#endif

		key.data = symname;
		key.size = strlen((char *)key.data);

		/*
		 * Convert the symbol information into an nlist structure,
		 * as best we can.
		 */
		memcpy(&val, syment->e_value, sizeof( long ));
		nbuf.n_value = val;
		nbuf.n_type = N_EXT;				/* XXX */
		nbuf.n_desc = 0;				/* XXX */
		nbuf.n_other = 0;				/* XXX */

		/*
		 * Enter the symbol into the database.
		 */
		if (db->put(db, &key, &data, 0)) {
			warn("record enter");
			punt();
		}
		/*
		 * If it's the kernel version string, we've gotta keep
		 * some extra data around.  Under a seperate key,
		 * we enter the first line (i.e. up to the first newline,
		 * with the newline replaced by a NUL to terminate the
		 * entered string) of the version string.
		 */
		if (strcmp((char *)key.data, VRS_SYM) == 0) {
			unsigned long vma;
			char *tmpcp;
			struct coff_scnhdr *sh;
			int i;

			key.data = (u_char *)VRS_KEY;
			key.size = sizeof(VRS_KEY) - 1;

			/* Find the version string, relative to start */
			vma = nbuf.n_value;

#ifdef DEBUG
			printf("vma = %lx,tstart = %lx, dstart=%lx\n",
			       vma, aouthdrp->a_tstart,
			       aouthdrp->a_dstart);
			printf("tsize = %lx, dsize=%lx\n",
			       aouthdrp->a_tsize,
			       aouthdrp->a_dsize);
#endif
			if (aouthdrp->a_tstart <= vma &&
		            vma < (aouthdrp->a_tstart + aouthdrp->a_tsize)){
				for(i=0;i<filehdrp->f_nscns;i++){
					sh = (struct coff_scnhdr *)
						&mappedfile[COFF_HDR_SIZE+
							    i*sizeof(struct
								     coff_scnhdr)];
					if( sh->s_flags == COFF_STYP_TEXT ){
						break;
					}
				}
				vma = vma - sh->s_vaddr + sh->s_scnptr;
			}
			else if (aouthdrp->a_dstart <= vma &&
			    vma < (aouthdrp->a_dstart + aouthdrp->a_dsize)){
				for(i=0;i<filehdrp->f_nscns;i++){
					sh = (struct coff_scnhdr *)
						&mappedfile[COFF_HDR_SIZE+
							    i*sizeof(struct
								     coff_scnhdr)];
					if( sh->s_flags == COFF_STYP_DATA ){
						break;
					}
				}
				vma = vma - sh->s_vaddr + sh->s_scnptr;
			}
			else {
				warn("version string neither text nor data");
				punt();
			}
			data.data = strdup(&mappedfile[vma]);

#ifdef DEBUG
			printf("vma = %lx,version = %s\n",
			       vma, (char *)data.data);
#endif

			/* assumes newline terminates version. */
			if ((tmpcp = strchr(data.data, '\n')) != NULL)
				*tmpcp = '\0';
			data.size = strlen((char *)data.data);

			if (db->put(db, &key, &data, 0)) {
				warn("record enter");
				punt();
			}

			/* free pointer created by strdup(). */
			free(data.data);

			/* Restore to original values */
			data.data = (u_char *)&nbuf;
			data.size = sizeof(nbuf);
		}
	} /* End of for() */

	rv = 0;

unmap:
	munmap(mappedfile, mappedsize);
out:
	return (rv);
}

#endif /* NLIST_COFF */
