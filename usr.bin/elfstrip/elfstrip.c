/* $NetBSD: elfstrip.c,v 1.3 1996/10/16 00:27:10 jonathan Exp $ */

/*
 * Copyright (c) 1995
 *	Ted Lemon (hereinafter referred to as the author)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* elf2elf.c */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/errno.h>
/*#include <machine/elf.h>*/
#include <sys/exec_elf.h>

#include <a.out.h>

struct sect {
  unsigned long vaddr;
  unsigned long len;
};

int phcmp ();

void usage __P((void));
int main __P((int argc, char *argv[]));


void *saveRead (int file, off_t offset, int len, char *name);
void placeWrite(int file, off_t offset, int len, char *name, void *data);
extern int errno;

int xflag = 0;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	int ch;

	while ((ch = getopt(argc, argv, "dx")) != EOF)
		switch(ch) {
                case 'x':
                        xflag = 1;
                        /*FALLTHROUGH*/
		case 'd':
#ifdef notyet
			sfcn = s_stab;
#endif
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Check args... */
	if (argc < 1) {
		usage();
	}
	while (argc > 0) {
		elf32_strip(argv [0]);
		 argc--; argv++;
	}
}

int
elf32_strip(filename)
     const char *filename;
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	int i;
	int infile;
	unsigned long lastp;
	Elf32_Phdr **indt;
	void *segment;

  if ((infile = open (filename, O_RDWR)) < 0)
    {
      fprintf (stderr, "Can't open %s for read/write: %s\n",
	       filename, strerror (errno));
      exit (1);
    }

  /* Read the header, which is at the beginning of the file... */

  /*
  ** printf("elf header from file 0 size %x\n", sizeof ex);
  */

  i = read (infile, &ex, sizeof ex);
  if (i != sizeof ex)
    {
      fprintf (stderr, "ex: %s: %s.\n",
	       filename, i ? strerror (errno) : "End of file reached");
      exit (1);
    }

  if (strncmp(ex.e_ident, Elf32_e_ident, sizeof(ex.e_ident)) != 0) {
      fprintf (stderr, "strip: %s not  ELF format\n", filename);
      return(0);
  }

  /* Devalidate section headers */
  ex.e_shoff = 0;
  ex.e_shnum = 0;
  ex.e_shentsize = 0;
  ex.e_shstrndx = 0;
  placeWrite (infile, 0, sizeof ex, "ex", &ex);


  /* Read the program headers... */
  /*
  ** printf("program headers from file %x size %x\n",
  **        ex.phoff, ex.phcount * sizeof (Elf32_Phdr));
  */
  lastp = ex.e_phoff + ex.e_phnum * sizeof (Elf32_Phdr);

  ph = (Elf32_Phdr *)saveRead (infile, ex.e_phoff,
				ex.e_phnum * sizeof(Elf32_Phdr), "ph");
  indt = (Elf32_Phdr **)malloc(ex.e_phnum * sizeof(Elf32_Phdr **));
  for (i=0; i<ex.e_phnum; i++) {
	indt[i] = ph+i;
  }

  qsort (indt, ex.e_phnum, sizeof (Elf32_Phdr **), phcmp);

  for (i = 0; i < ex.e_phnum; i++)
    {
      Elf32_Phdr *php = indt[i];

      /* Section types we can ignore... */
      if (php->p_type == Elf_pt_null || php->p_type == Elf_pt_note ||
	  php->p_type == Elf_pt_phdr || php->p_type == Elf_pt_mips_reginfo)
      {
	printf("segment type %ld ignored file %lx size %lx\n",
		php->p_type, php->p_offset, php->p_filesz);
	continue;
      }
      /* Section types we can't handle... */
      else if (php->p_type != Elf_pt_load)
        {
	  fprintf (stderr, "Program header %d type %ld can't be converted.\n",
			php-ph, php->p_type);
	  exit (1);
	}

	/*
	** printf("segment %d load file %x size %x end %x\n",
	**	php-ph,
	**	php->p_offset, php->p_filesz, php->p_offset + php->p_filesz);
	*/
	if ((php->p_offset - lastp) > 0x2000) {
		int pg;

		pg = ((php->p_offset - lastp) / 0x2000);
		pg *= 0x2000;

  		segment = saveRead (infile, php->p_offset,
					php->p_filesz, "segment");
		php->p_offset -= pg;
  		placeWrite (infile, php->p_offset,
  					php->p_filesz, "segment", segment);
		/*
		** printf("Move segment %d shift %x to %x\n", 
		**	php-ph, pg, php->p_offset);
		*/
	}
	lastp = php->p_offset + php->p_filesz;
     }
  placeWrite (infile, ex.e_phoff, ex.e_phnum * sizeof (Elf32_Phdr), "ph", ph);
  ftruncate(infile, lastp);

  /* Looks like we won... */
  exit (0);
}

int
phcmp (h1, h2)
     Elf32_Phdr **h1, **h2;
{
  if ((*h1) -> p_offset > (*h2) -> p_offset)
    return 1;
  else if ((*h1) -> p_offset < (*h2) -> p_offset)
    return -1;
  else
    return 0;
}

void *
saveRead (int file, off_t offset, int len, char *name)
{
  void *tmp;
  int count;
  off_t off;
  if ((off = lseek (file, offset, SEEK_SET)) < 0)
    {
      fprintf (stderr, "%s: fseek: %s\n", name, strerror (errno));
      exit (1);
    }
  if (!(tmp = malloc (len)))
    {
      fprintf (stderr, "%s: Can't allocate %d bytes.\n", name, len);
      exit (1);
    }
  count = read (file, tmp, len);
  if (count != len)
    {
      fprintf (stderr, "%s: read: %s.\n",
	       name, count ? strerror (errno) : "End of file reached");
      exit (1);
    }
  return tmp;
}

void
placeWrite (int file, off_t offset, int len, char *name, void *data)
{
  int count;
  off_t off;
  if ((off = lseek (file, offset, SEEK_SET)) < 0)
    {
      fprintf (stderr, "%s: lseek: %s\n", name, strerror (errno));
      exit (1);
    }
  count = write (file, data, len);
  if (count != len)
    {
      fprintf (stderr, "%s: write: %s.\n", name, strerror (errno));
      exit (1);
    }
}


void
usage()
{
	(void)fprintf(stderr, "usage: strip [-dx] file ...\n");
	exit(1);
}
