/*	$NetBSD: wrtvid.c,v 1.1.32.1 2000/12/08 09:28:45 bouyer Exp $	*/

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#define __DBINTERFACE_PRIVATE
#include <db.h>
#include <machine/disklabel.h>

#include "loadfile.h"

static void swabcfg(struct cpu_disklabel *);
static void swabvid(struct cpu_disklabel *);

int
main(int argc, char **argv)
{
	struct cpu_disklabel *pcpul;
	int tape_vid, tape_exe, fd;
	char *filename;
	char *filebuff;
	char fileext[256];
	u_long marks[MARK_MAX];
	size_t len;

	if (argc == 0)
		filename = "a.out";
	else
		filename = argv[1];

	marks[MARK_START] = 0;
	if ((fd = loadfile(filename, marks, COUNT_TEXT|COUNT_DATA)) == -1)
		return NULL;
	(void)close(fd);

	len = (((marks[MARK_END] - marks[MARK_START]) + 511) / 512) * 2;
	len *= 256;
	filebuff = malloc(len);

	marks[MARK_START] = (u_long)(filebuff - marks[MARK_START]);

	if ((fd = loadfile(filename, marks, LOAD_TEXT|LOAD_DATA)) == -1)
		return NULL;
	(void)close(fd);

	sprintf(fileext, "%c%cboot", filename[4], filename[5]);
	tape_vid = open(fileext, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	sprintf(fileext, "boot%c%c", filename[4], filename[5]);
	tape_exe = open(fileext, O_WRONLY|O_CREAT|O_TRUNC,0644);

	pcpul = (struct cpu_disklabel *)malloc(sizeof(struct cpu_disklabel));
	bzero(pcpul, sizeof(struct cpu_disklabel));

	strcpy(pcpul->vid_id, "NBSD");

	if (filename[5] == 't' ) {
		pcpul->vid_oss = 1;
	}else {
		pcpul->vid_oss = 2;
	}
	pcpul->vid_osl = len / 256;

	/* check this, it may not work in both endian. */
	{
		union {
			struct s {
				unsigned short s1;
				unsigned short s2;
			} s;
			unsigned long l;
		} a;
		a.l = marks[MARK_ENTRY];
		pcpul->vid_osa_u = a.s.s1;
		pcpul->vid_osa_l = a.s.s2;

	}
	pcpul->vid_cas = 1;
	pcpul->vid_cal = 1;
	/* do not want to write past end of structure, not null terminated */
	strncpy(pcpul->vid_mot, "MOTOROLA", 8);

	if (BYTE_ORDER != BIG_ENDIAN)
		swabvid(pcpul);

	pcpul->cfg_rec = 0x100;
	pcpul->cfg_psm = 0x200;

	if (BYTE_ORDER != BIG_ENDIAN)
		swabcfg(pcpul);

	write(tape_vid, pcpul, sizeof(struct cpu_disklabel));

	free(pcpul);

	write(tape_exe, filebuff, len);
	free(filebuff);

	close(tape_vid);
	close(tape_exe);
	return (0);
}

static void
swabvid(pcpul)
	struct cpu_disklabel *pcpul;
{
	M_32_SWAP(pcpul->vid_oss);
	M_16_SWAP(pcpul->vid_osl);
	/*
	M_16_SWAP(pcpul->vid_osa_u);
	M_16_SWAP(pcpul->vid_osa_l);
	*/
	M_32_SWAP(pcpul->vid_cas);
}

static void
swabcfg(pcpul)
	struct cpu_disklabel *pcpul;
{
	M_16_SWAP(pcpul->cfg_atm);
	M_16_SWAP(pcpul->cfg_prm);
	M_16_SWAP(pcpul->cfg_atm);
	M_16_SWAP(pcpul->cfg_rec);
	M_16_SWAP(pcpul->cfg_trk);
	M_16_SWAP(pcpul->cfg_psm);
	M_16_SWAP(pcpul->cfg_shd);
	M_16_SWAP(pcpul->cfg_pcom);
	M_16_SWAP(pcpul->cfg_rwcc);
	M_16_SWAP(pcpul->cfg_ecc);
	M_16_SWAP(pcpul->cfg_eatm);
	M_16_SWAP(pcpul->cfg_eprm);
	M_16_SWAP(pcpul->cfg_eatw);
	M_16_SWAP(pcpul->cfg_rsvc1);
	M_16_SWAP(pcpul->cfg_rsvc2);
}
