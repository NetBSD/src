/* $NetBSD: elfstrip.c,v 1.2 1996/09/29 22:01:47 jonathan Exp $ */

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
#include <machine/elf.h>
#include <a.out.h>

struct sect {
  unsigned long vaddr;
  unsigned long len;
};

int phcmp ();

void *saveRead (int file, off_t offset, int len, char *name);
void placeWrite(int file, off_t offset, int len, char *name, void *data);
extern int errno;

int main (int argc, char **argv)
{
  struct ehdr ex;
  struct phdr *ph;
  int i;
  int infile;
  unsigned long lastp;
  struct phdr **indt;
  void *segment;

  /* Check args... */
  if (argc != 2)
    {
      fprintf (stderr, "usage: elfstrip <elf executable>\n");
      exit (1);
    }

  /* Try the input file... */
  if ((infile = open (argv [1], O_RDWR)) < 0)
    {
      fprintf (stderr, "Can't open %s for read/write: %s\n",
	       argv [1], strerror (errno));
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
	       argv [1], i ? strerror (errno) : "End of file reached");
      exit (1);
    }

  /* Devalidate section headers */
  ex.shoff = 0;
  ex.shcount = 0;
  ex.shsize = 0;
  ex.shstrndx = 0;
  placeWrite (infile, 0, sizeof ex, "ex", &ex);


  /* Read the program headers... */
  /*
  ** printf("program headers from file %x size %x\n",
  **        ex.phoff, ex.phcount * sizeof (struct phdr));
  */
  lastp = ex.phoff + ex.phcount * sizeof (struct phdr);

  ph = (struct phdr *)saveRead (infile, ex.phoff,
				ex.phcount * sizeof (struct phdr), "ph");
  indt = (struct phdr **)malloc(ex.phcount * sizeof(struct phdr **));
  for (i=0; i<ex.phcount; i++) {
	indt[i] = ph+i;
  }

  qsort (indt, ex.phcount, sizeof (struct phdr **), phcmp);

  for (i = 0; i < ex.phcount; i++)
    {
      struct phdr *php = indt[i];

      /* Section types we can ignore... */
      if (php->type == PT_NULL || php->type == PT_NOTE ||
	  php->type == PT_PHDR || php->type == PT_MIPS_REGINFO)
      {
	printf("segment type %ld ignored file %lx size %lx\n",
		php->type, php->offset, php->filesz);
	continue;
      }
      /* Section types we can't handle... */
      else if (php->type != PT_LOAD)
        {
	  fprintf (stderr, "Program header %d type %ld can't be converted.\n",
			php-ph, php->type);
	  exit (1);
	}

	/*
	** printf("segment %d load file %x size %x end %x\n",
	**	php-ph,
	**	php->offset, php->filesz, php->offset + php->filesz);
	*/
	if ((php->offset - lastp) > 0x2000) {
		int pg;

		pg = ((php->offset - lastp) / 0x2000);
		pg *= 0x2000;

  		segment = saveRead (infile, php->offset,
					php->filesz, "segment");
		php->offset -= pg;
  		placeWrite (infile, php->offset,
  					php->filesz, "segment", segment);
		/*
		** printf("Move segment %d shift %x to %x\n", 
		**	php-ph, pg, php->offset);
		*/
	}
	lastp = php->offset + php->filesz;
     }
  placeWrite (infile, ex.phoff, ex.phcount * sizeof (struct phdr), "ph", ph);
  ftruncate(infile, lastp);

  /* Looks like we won... */
  exit (0);
}

int
phcmp (h1, h2)
     struct phdr **h1, **h2;
{
  if ((*h1) -> offset > (*h2) -> offset)
    return 1;
  else if ((*h1) -> offset < (*h2) -> offset)
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
