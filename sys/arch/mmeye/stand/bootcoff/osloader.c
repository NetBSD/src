/*-
 * Copyright (C) 1999 T.Horiuchi(Brains, Corp.) and SAITOH Masanobu.
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
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/exec_coff.h>
#include <sys/param.h>
#include <sys/gmon.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <uvm/uvm_param.h>
#include <machine/cpu.h>

#define vm_page_size (NBPG)

char *netbsd = "/netbsd";
struct coff_filehdr FileHdr;
struct coff_aouthdr AoutHdr;

static int coff_find_section __P((FILE *, struct coff_filehdr *,
			     struct coff_scnhdr *, int));

void	LoadAndReset __P((char *));
int	main __P((int, char **));

static int
coff_find_section(fd, fp, sh, s_type)
	FILE *fd;
	struct coff_filehdr *fp;
	struct coff_scnhdr *sh;
	int s_type;
{
	int i, pos, siz;
	
	pos = COFF_HDR_SIZE;
	for (i = 0; i < fp->f_nscns; i++, pos += sizeof(struct coff_scnhdr)) {
		siz = sizeof(struct coff_scnhdr);
		if (fread(sh, 1, siz, fd) != siz) {
			perror("osloader");
			exit(1);
		}

		if (sh->s_flags == s_type)
			return (0);
	}
	return (-1);
}

void
LoadAndReset(osimage)
	char *osimage;
{
	int mib[2];
	u_long val;
	int len;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_LOADANDRESET;
	val = (u_long)osimage;
	len = sizeof(val);

	sysctl(mib, 2, NULL, NULL, &val, len);
}

int 
main(argc, argv)
	int argc;
	char *argv[];
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

	fp = fopen(netbsd, "r");
	if (fp == NULL) {
		perror("osloader");
		exit(1);
	}

	if (fread(&FileHdr, 1, sizeof(FileHdr), fp) != sizeof(FileHdr)) {
		perror("osloader");
		exit(1);
	}

	if (fread(&AoutHdr, 1, sizeof(AoutHdr), fp) != sizeof(AoutHdr)) {
		perror("osloader");
		exit(1);
	}

	/* set up command for text segment */
	error = coff_find_section(fp, &FileHdr, &sh, COFF_STYP_TEXT);
	if (error) {		
		printf("can't find text section: %d\n", error);
		exit(1);
	}

	ep_taddr = COFF_ALIGN(sh.s_vaddr);
	toffset = sh.s_scnptr - (sh.s_vaddr - ep_taddr);
	ep_tsize = sh.s_size + (sh.s_vaddr - ep_taddr);

	printf("addr %lx size 0x%lx offset 0x%lx\n", ep_taddr,
	       ep_tsize, toffset); 

	/* set up command for data segment */
	error = coff_find_section(fp, &FileHdr, &sh, COFF_STYP_DATA);
	if (error) {
		printf("can't find data section: %d\n", error);
		exit(1);
	}

	ep_daddr = COFF_ALIGN(sh.s_vaddr);
	doffset = sh.s_scnptr - (sh.s_vaddr - ep_daddr);
	dsize = sh.s_size + (sh.s_vaddr - ep_daddr);
	ep_dsize = round_page(dsize) + AoutHdr.a_bsize;

	printf("addr 0x%lx size 0x%lx offset 0x%lx\n", ep_daddr,
		 dsize, doffset); 

	osimage = malloc(ep_tsize+dsize+sizeof(u_long) * 2);
	if (osimage == NULL) {
		printf("osloader:no memory\n");
		exit(1);
	}

	*(u_long *)osimage = ep_tsize+dsize;
	p = osimage + 2 * sizeof(u_long);
	
	/* load text area */
	fseek(fp, toffset, SEEK_SET);
	if (fread(p, 1, ep_tsize, fp) != ep_tsize) {
		perror("osloader:");
		exit(1);
	}

	/* load data area */
	fseek(fp, doffset, SEEK_SET);
	if (fread(p + ep_daddr - ep_taddr, 1, dsize, fp) != dsize) {
		perror("osloader:");
		exit(1);
	}

	fclose(fp);

	cksum = 0;
	size = (ep_tsize + dsize) >> 2;

	for(i = 0; i < size; i++) {
		cksum += *(u_long *)p;
		p += sizeof(u_long);
	}

	*(u_long *)(osimage+sizeof(u_long)) = cksum;

	LoadAndReset(osimage);

	return (0);	/* NOTREACHED */
}
