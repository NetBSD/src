/*	$NetBSD: edahdi.c,v 1.2 1999/06/01 14:27:39 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman, Waldi Ravens.
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
 *      This product includes software developed by
 *			Leo Weppelman and Waldi Ravens.
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

/*
 * This code implements a simple editor for partition id's on disks containing
 * AHDI partition info. 
 *
 * Credits for the code handling disklabels goes to Waldi Ravens.
 *
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/disklabel.h>

#include <machine/ahdilabel.h>

#include <fcntl.h>
#include <stdlib.h>
#include <curses.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

/*
 * Internal partition tables:
 */
typedef struct {
	char	id[4];
	u_int	start;
	u_int	end;
	u_int	rsec;
	u_int	rent;
	int	mod;
} part_t;

typedef struct {
	int	nparts;
	part_t	*parts;
} ptable_t;

/*
 * I think we can savely assume a fixed blocksize - AHDI won't support
 * something different...
 */
#define BLPM		((1024 * 1024) / DEV_BSIZE)

/*
 * #Partition entries shown on the screen at once
 */
#define MAX_PSHOWN	16	/* #partitions shown on screen	*/

/*
 * Tokens:
 */
#define	T_INVAL		0
#define	T_QUIT		1
#define	T_WRITE		2
#define	T_NEXT		3
#define	T_PREV		4
#define	T_NUMBER	5
#define	T_EOF		6

/*
 * Terminal capability strings (Ok, 1 to start with ;-) )
 */
char	*Clr_screen = "";

void	ahdi_cksum __P((void *));
u_int	ahdi_getparts __P((int, ptable_t *, u_int, u_int));
int	bsd_label __P((int, u_int));
int	dkcksum __P((struct disklabel *));
int	edit_parts __P((int, ptable_t *));
void   *disk_read __P((int, u_int, u_int));
void	disk_write __P((int, u_int, u_int, void  *));
char   *get_id __P((void));
void	get_termcap __P((void));
int	lex __P((int *));
void	outc __P((int));
int	show_parts __P((ptable_t *, int));
void	update_disk __P((ptable_t *, int, int));

int
main(argc, argv)
int	argc;
char	*argv[];
{
	int		fd;
	ptable_t	ptable;
	int		rv;
	struct stat	st;

	if (argc != 2) {
		char	*prog = strrchr(argv[0], '/');

		if (prog == NULL)
			prog = argv[0];
		else prog++;
		fprintf(stderr, "Usage: %s <raw_disk_device>", prog);
		exit(1);
	}
	if ((fd = open(argv[1], O_RDWR)) < 0)
		err(1, "Cannot open '%s'.", argv[1]);
	if (fstat(fd, &st) < 0)
		err(1, "Cannot stat '%s'.", argv[1]);
	if (!S_ISCHR(st.st_mode))
		errx(1, "'%s' must be a character special device.", argv[1]);

	if ((rv = bsd_label(fd, LABELSECTOR)) < 0)
		errx(1, "I/O error");
	if (rv == 0) {
		warnx("Disk has no ahdi partitions");
		return (2);
	}

	get_termcap();

	ptable.nparts = 0;
	ptable.parts  = NULL;

	if ((ahdi_getparts(fd, &ptable, AHDI_BBLOCK, AHDI_BBLOCK) != 0)
		|| (ptable.nparts == 0))
		exit (1);

	edit_parts(fd, &ptable);
	return (0);
}

int
edit_parts(fd, ptable)
	int		fd;
	ptable_t	*ptable;
{
	int	scr_base = 0;
	int	value;
	char	*error, *new_id;

	for (;;) {
		error = NULL;
		tputs(Clr_screen, 1, outc);
		show_parts(ptable, scr_base);

		printf("\n\n");
		printf("q    : quit - don't update the disk\n");
		printf("w    : write changes to disk\n");
		printf(">    : next screen of partitions\n");
		printf("<    : previous screen of partitions\n");
		printf("<nr> : modify id of partition <nr>\n");
		printf("\n\nCommand? ");
		fflush(stdout);

		switch (lex(&value)) {
		    case T_EOF:
			exit(0);

		    case T_INVAL:
			error = "Invalid command";
			break;
		    case T_QUIT :
			for (value = 0; value < ptable->nparts; value++) {
				if (ptable->parts[value].mod) {
					printf("\nThere are unwritten changes."
						" Quit anyway? [n] ");
					value = getchar();
					if ((value == 'y') || (value == 'Y'))
							exit (0);
					while (value != '\n')
						value = getchar();
					break;
				}
			}
			if (value == ptable->nparts)
				exit(0);
			break;
		    case T_WRITE:
			error = "No changes to write";
			for (value = 0; value < ptable->nparts; value++) {
				if (ptable->parts[value].mod) {
					update_disk(ptable, fd, value);
					error = "";
				}
			}
			break;
		    case T_NEXT :
			if ((scr_base + MAX_PSHOWN) < ptable->nparts)
				scr_base += MAX_PSHOWN;
			break;
		    case T_PREV :
			scr_base -= MAX_PSHOWN;
			if (scr_base < 0)
				scr_base = 0;
			break;
		    case T_NUMBER:
			if (value >= ptable->nparts) {
				error = "Not that many partitions";
				break;
			}
			if ((new_id = get_id()) == NULL) {
				error = "Invalid id";
				break;
			}
			strncpy(ptable->parts[value].id, new_id, 3);
			ptable->parts[value].mod = 1;
			scr_base = (value / MAX_PSHOWN) * MAX_PSHOWN;
			break;
		    default :
			error = "Internal error - unknown token";
			break;
		}
		if (error != NULL) {
			printf("\n\n%s", error);
			fflush(stdout);
			sleep(2);
		}
	}
}

int
show_parts(ptable, nr)
	ptable_t	*ptable;
	int		nr;
{
	int	i;
	part_t	*p;
	u_int	megs;

	if (nr >= ptable->nparts)
		return (0);	/* Nothing to show */
	printf("\n\n");
	printf("nr      root  desc   id     start       end    MBs\n");

	p = &ptable->parts[nr];
	i = nr;
	for(; (i < ptable->nparts) && ((i - nr) < MAX_PSHOWN); i++, p++) {
		megs = ((p->end - p->start + 1) + (BLPM >> 1)) / BLPM;
		printf("%2d%s %8u  %4u  %s  %8u  %8u  (%3u)\n", i,
			p->mod ? "*" : " ",
			p->rsec, p->rent, p->id, p->start, p->end, megs);
	}
	return (1);
}

int
lex(value)
	int	*value;
{
	char	c[1];
	int	rv, nch;

	rv = T_INVAL;

	*value = 0;
	for (;;) {
		if ((nch = read (0, c, 1)) != 1) {
			if (nch == 0)
				return (T_EOF);
			else return (rv);
		}
		switch (*c) {
			case 'q':
				rv = T_QUIT;
				goto out;
			case 'w':
				rv = T_WRITE;
				goto out;
			case '>':
				rv = T_NEXT;
				goto out;
			case '<':
				rv = T_PREV;
				goto out;
			default :
				if (isspace(*c)) {
					if (rv == T_INVAL)
						break;
					goto out;
				}
				else if (isdigit(*c)) {
					*value = (10 * *value) + *c - '0';
					rv = T_NUMBER;
				}
				goto out;
		}
	}
	/* NOTREACHED */
out:
	/*
	 * Flush rest of line before returning
	 */
	while (read (0, c, 1) == 1)
		if ((*c == '\n') || (*c == '\r'))
			break;
	return (rv);
}

char *
get_id()
{
	static char	buf[5];
	       int	n;
	printf("\nEnter new id: ");
	if (fgets(buf, sizeof(buf), stdin) == NULL)
		return (NULL);
	for (n = 0; n < 3; n++) {
		if (!isalpha(buf[n]))
			return (NULL);
		buf[n] = toupper(buf[n]);
	}
	buf[3] = '\0';
	return (buf);
}

int
bsd_label(fd, offset)
	int		fd;
	u_int		offset;
{
	u_char		*bblk;
	u_int		nsec;
	int		rv;

	nsec = (BBMINSIZE + (DEV_BSIZE - 1)) / DEV_BSIZE;
	bblk = disk_read(fd, offset, nsec);
	if (bblk) {
		u_int	*end, *p;
		
		end = (u_int *)&bblk[BBMINSIZE - sizeof(struct disklabel)];
		rv  = 1;
		for (p = (u_int *)bblk; p < end; ++p) {
			struct disklabel *dl = (struct disklabel *)&p[1];
			if (  (  (p[0] == NBDAMAGIC && offset == 0)
			      || (p[0] == AHDIMAGIC && offset != 0)
			      || (u_char *)dl - bblk == 7168
			      )
			   && dl->d_npartitions <= MAXPARTITIONS
			   && dl->d_magic2 == DISKMAGIC
			   && dl->d_magic  == DISKMAGIC
			   && dkcksum(dl)  == 0
			   )	{
				rv = 0;
				break;
			}
		}
		free(bblk);
	}
	else rv = -1;

	return(rv);
}

int
dkcksum(dl)
	struct disklabel *dl;
{
	u_short	*start, *end, sum = 0;

	start = (u_short *)dl;
	end   = (u_short *)&dl->d_partitions[dl->d_npartitions];
	while (start < end)
		sum ^= *start++;
	return(sum);
}

void
ahdi_cksum(buf)
void	*buf;
{
	unsigned short	*p = (unsigned short *)buf;
	unsigned short	csum = 0;
	int		i;

	p[255] = 0;
	for(i = 0; i < 256; i++)
		csum += *p++;
	*--p = (0x1234 - csum) & 0xffff;
}


u_int
ahdi_getparts(fd, ptable, rsec, esec)
	int			fd;
	ptable_t		*ptable;
	u_int			rsec,
				esec;
{
	struct ahdi_part	*part, *end;
	struct ahdi_root	*root;
	u_int			rv;

	root = disk_read(fd, rsec, 1);
	if (!root) {
		rv = rsec + (rsec == 0);
		goto done;
	}

	if (rsec == AHDI_BBLOCK)
		end = &root->ar_parts[AHDI_MAXRPD];
	else end = &root->ar_parts[AHDI_MAXARPD];
	for (part = root->ar_parts; part < end; ++part) {
		u_int	id = *((u_int32_t *)&part->ap_flg);
		if (!(id & 0x01000000))
			continue;
		if ((id &= 0x00ffffff) == AHDI_PID_XGM) {
			u_int	offs = part->ap_st + esec;
			rv = ahdi_getparts(fd, ptable, offs,
					esec == AHDI_BBLOCK ? offs : esec);
			if (rv)
				goto done;
		} else {
			part_t	*p;
			u_int	i = ++ptable->nparts;
			ptable->parts = realloc(ptable->parts,
						i * sizeof *ptable->parts);
			if (ptable->parts == NULL) {
				fprintf(stderr, "Allocation error\n");
				rv = 1;
				goto done;
			}
			p = &ptable->parts[--i];
			*((u_int32_t *)&p->id) = id << 8;
			p->start = part->ap_st + rsec;
			p->end   = p->start + part->ap_size - 1;
			p->rsec  = rsec;
			p->rent  = part - root->ar_parts;
			p->mod   = 0;
		}
	}
	rv = 0;
done:
	free(root);
	return(rv);
}

void *
disk_read(fd, start, count)
	int	fd;
	u_int	start,
		count;
{
	char	*buffer;
	off_t	offset;
	size_t	size;

	size   = count * DEV_BSIZE;
	offset = start * DEV_BSIZE;
	if ((buffer = malloc(size)) == NULL)
		errx(1, "No memory");

	if (lseek(fd, offset, SEEK_SET) != offset) {
		free(buffer);
		err(1, "Seek error");
	}
	if (read(fd, buffer, size) != size) {
		free(buffer);
		err(1, "Read error");
		exit(1);
	}
	return(buffer);
}

void
update_disk(ptable, fd, pno)
	ptable_t	*ptable;
	int		fd, pno;
{
	struct ahdi_root	*root;
	struct ahdi_part	*apart;
	part_t			*lpart;
	u_int			rsec;
	int			i;

	rsec = ptable->parts[pno].rsec;
	root = disk_read(fd, rsec, 1);

	/*
	 * Look for additional mods on the same sector
	 */
	for (i = 0; i < ptable->nparts; i++) {
		lpart = &ptable->parts[i];
		if (lpart->mod && (lpart->rsec == rsec)) {
			apart = &root->ar_parts[lpart->rent];

			/* Paranoia.... */
			if ((lpart->end - lpart->start + 1) != apart->ap_size)
				errx(1, "Updating wrong partition!");
			apart->ap_id[0] = lpart->id[0];
			apart->ap_id[1] = lpart->id[1];
			apart->ap_id[2] = lpart->id[2];
			lpart->mod = 0;
		}
	}
	if (rsec == 0)
		ahdi_cksum(root);
	disk_write(fd, rsec, 1, root);
	free(root);
}

void
disk_write(fd, start, count, buf)
	int	fd;
	u_int	start,
		count;
	void	*buf;
{
	off_t	offset;
	size_t	size;

	size   = count * DEV_BSIZE;
	offset = start * DEV_BSIZE;

	if (lseek(fd, offset, SEEK_SET) != offset)
		err(1, "Seek error");
	if (write(fd, buf, size) != size)
		err(1, "Write error");
}

void
get_termcap()
{
	char	*term, tbuf[1024], buf[1024], *p;

	if ((term = getenv("TERM")) == NULL)
		warnx("No TERM environment variable!");
	else {
		if (tgetent(tbuf, term) != 1)
			errx(1, "Tgetent failure.");
		p = buf;
		if (tgetstr("cl", &p)) {
			if ((Clr_screen = malloc(strlen(buf) + 1)) == NULL)
				errx(1, "Malloc failure.");
			strcpy(Clr_screen, buf);
		}
	}
}

void
outc(c)
int	c;
{
	(void)putchar(c);
}
