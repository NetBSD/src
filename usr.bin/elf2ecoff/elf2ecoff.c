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

/* elf2ecoff.c

   This program converts an elf executable to an ECOFF executable.
   No symbol table is retained.   This is useful primarily in building
   net-bootable kernels for machines (e.g., DECstation and Alpha) which
   only support the ECOFF object file format. */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <machine/elf.h>
#include <stdio.h>
#include <sys/exec_ecoff.h>
#include <sys/errno.h>
#include <string.h>
#include <limits.h>

struct sect {
  unsigned long vaddr;
  unsigned long len;
};

int phcmp ();
char *saveRead (int file, off_t offset, off_t len, char *name);
int copy (int, int, off_t, off_t);
int translate_syms (int, int, off_t, off_t, off_t, off_t);
extern int errno;
int *symTypeTable;

main (int argc, char **argv, char **envp)
{
  struct ehdr ex;
  struct phdr *ph;
  struct shdr *sh;
  struct sym *symtab;
  char *shstrtab;
  int strtabix, symtabix;
  int i, pad;
  struct sect text, data, bss;
  struct ecoff_filehdr efh;
  struct ecoff_aouthdr eah;
  struct ecoff_scnhdr esecs [3];
  int infile, outfile;
  unsigned long cur_vma = ULONG_MAX;
  int symflag = 0;

  text.len = data.len = bss.len = 0;
  text.vaddr = data.vaddr = bss.vaddr = 0;

  /* Check args... */
  if (argc < 3 || argc > 4)
    {
    usage:
      fprintf (stderr,
	       "usage: elf2aout <elf executable> <a.out executable> [-s]\n");
      exit (1);
    }
  if (argc == 4)
    {
      if (strcmp (argv [3], "-s"))
	goto usage;
      symflag = 1;
    }

  /* Try the input file... */
  if ((infile = open (argv [1], O_RDONLY)) < 0)
    {
      fprintf (stderr, "Can't open %s for read: %s\n",
	       argv [1], strerror (errno));
      exit (1);
    }

  /* Read the header, which is at the beginning of the file... */
  i = read (infile, &ex, sizeof ex);
  if (i != sizeof ex)
    {
      fprintf (stderr, "ex: %s: %s.\n",
	       argv [1], i ? strerror (errno) : "End of file reached");
      exit (1);
    }

  /* Read the program headers... */
  ph = (struct phdr *)saveRead (infile, ex.phoff,
				ex.phcount * sizeof (struct phdr), "ph");
  /* Read the section headers... */
  sh = (struct shdr *)saveRead (infile, ex.shoff,
				ex.shcount * sizeof (struct shdr), "sh");
  /* Read in the section string table. */
  shstrtab = saveRead (infile, sh [ex.shstrndx].offset,
		       sh [ex.shstrndx].size, "shstrtab");

  /* Figure out if we can cram the program header into an ECOFF
     header...  Basically, we can't handle anything but loadable
     segments, but we can ignore some kinds of segments.  We can't
     handle holes in the address space.  Segments may be out of order,
     so we sort them first. */

  qsort (ph, ex.phcount, sizeof (struct phdr), phcmp);

  for (i = 0; i < ex.phcount; i++)
    {
      /* Section types we can ignore... */
      if (ph [i].type == PT_NULL || ph [i].type == PT_NOTE ||
	  ph [i].type == PT_PHDR || ph [i].type == PT_MIPS_REGINFO)
	continue;
      /* Section types we can't handle... */
      else if (ph [i].type != PT_LOAD)
        {
	  fprintf (stderr, "Program header %d type %d can't be converted.\n");
	  exit (1);
	}
      /* Writable (data) segment? */
      if (ph [i].flags & PF_W)
	{
	  struct sect ndata, nbss;

	  ndata.vaddr = ph [i].vaddr;
	  ndata.len = ph [i].filesz;
	  nbss.vaddr = ph [i].vaddr + ph [i].filesz;
	  nbss.len = ph [i].memsz - ph [i].filesz;

	  combine (&data, &ndata, 0);
	  combine (&bss, &nbss, 1);
	}
      else
	{
	  struct sect ntxt;

	  ntxt.vaddr = ph [i].vaddr;
	  ntxt.len = ph [i].filesz;

	  combine (&text, &ntxt);
	}
      /* Remember the lowest segment start address. */
      if (ph [i].vaddr < cur_vma)
	cur_vma = ph [i].vaddr;
    }

  /* Sections must be in order to be converted... */
  if (text.vaddr > data.vaddr || data.vaddr > bss.vaddr ||
      text.vaddr + text.len > data.vaddr || data.vaddr + data.len > bss.vaddr)
    {
      fprintf (stderr, "Sections ordering prevents a.out conversion.\n");
      exit (1);
    }

  /* If there's a data section but no text section, then the loader
     combined everything into one section.   That needs to be the
     text section, so just make the data section zero length following
     text. */
  if (data.len && !text.len)
    {
      text = data;
      data.vaddr = text.vaddr + text.len;
      data.len = 0;
    }

  /* If there is a gap between text and data, we'll fill it when we copy
     the data, so update the length of the text segment as represented in
     a.out to reflect that, since a.out doesn't allow gaps in the program
     address space. */
  if (text.vaddr + text.len < data.vaddr)
    text.len = data.vaddr - text.vaddr;

  /* We now have enough information to cons up an a.out header... */
  eah.ea_magic = ECOFF_OMAGIC;
  eah.ea_vstamp = 200;
  eah.ea_tsize = text.len;
  eah.ea_dsize = data.len;
  eah.ea_bsize = bss.len;
  eah.ea_entry = ex.entry;
  eah.ea_text_start = text.vaddr;
  eah.ea_data_start = data.vaddr;
  eah.ea_bss_start = bss.vaddr;
  eah.ea_gprmask = 0xf3fffffe;
  bzero (&eah.ea_cprmask, sizeof eah.ea_cprmask);
  eah.ea_gp_value = 0; /* unused. */

  efh.ef_magic = ECOFF_MAGIC_MIPSEL;
  efh.ef_nsecs = 3;
  efh.ef_timestamp = 0;	/* bogus */
  efh.ef_symptr = 0;
  efh.ef_syms = 0;
  efh.ef_opthdr = sizeof eah;
  efh.ef_flags = 0x100f; /* Stripped, not sharable. */

  strcpy (esecs [0].es_name, ".text");
  strcpy (esecs [1].es_name, ".data");
  strcpy (esecs [2].es_name, ".bss");
  esecs [0].es_physaddr = esecs [0].es_virtaddr = eah.ea_text_start;
  esecs [1].es_physaddr = esecs [1].es_virtaddr = eah.ea_data_start;
  esecs [2].es_physaddr = esecs [2].es_virtaddr = eah.ea_bss_start;
  esecs [0].es_size = eah.ea_tsize;
  esecs [1].es_size = eah.ea_dsize;
  esecs [2].es_size = eah.ea_bsize;
  esecs [0].es_raw_offset = ECOFF_TXTOFF (&efh, &eah);
  esecs [1].es_raw_offset = ECOFF_DATOFF (&efh, &eah);
  esecs [2].es_raw_offset = esecs [1].es_raw_offset +
	  ECOFF_ROUND (esecs [1].es_size, ECOFF_SEGMENT_ALIGNMENT (&eah));
  esecs [0].es_reloc_offset = esecs [1].es_reloc_offset
	  = esecs [2].es_reloc_offset = 0;
  esecs [0].es_line_offset = esecs [1].es_line_offset
	  = esecs [2].es_line_offset = 0;
  esecs [0].es_nreloc = esecs [1].es_nreloc = esecs [2].es_nreloc = 0;
  esecs [0].es_nline = esecs [1].es_nline = esecs [2].es_nline = 0;
  esecs [0].es_flags = 0x20;
  esecs [1].es_flags = 0x40;
  esecs [2].es_flags = 0x82;

  /* Make the output file... */
  if ((outfile = open (argv [2], O_WRONLY | O_CREAT, 0777)) < 0)
    {
      fprintf (stderr, "Unable to create %s: %s\n", argv [2], strerror (errno));
      exit (1);
    }

  /* Write the headers... */
  i = write (outfile, &efh, sizeof efh);
  if (i != sizeof efh)
    {
      perror ("efh: write");
      exit (1);

  for (i = 0; i < 6; i++)
    {
      printf ("Section %d: %s phys %x  size %x  file offset %x\n",
	      i, esecs [i].es_name, esecs [i].es_physaddr,
	      esecs [i].es_size, esecs [i].es_raw_offset);
    }
    }
  fprintf (stderr, "wrote %d byte file header.\n", i);

  i = write (outfile, &eah, sizeof eah);
  if (i != sizeof eah)
    {
      perror ("eah: write");
      exit (1);
    }
  fprintf (stderr, "wrote %d byte a.out header.\n", i);

  i = write (outfile, &esecs, sizeof esecs);
  if (i != sizeof esecs)
    {
      perror ("esecs: write");
      exit (1);
    }
  fprintf (stderr, "wrote %d bytes of section headers.\n", i);

  if (pad = ((sizeof efh + sizeof eah + sizeof esecs) & 15))
    {
      pad = 16 - pad;
      i = write (outfile, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0", pad);
      if (i < 0)
	{
	  perror ("ipad: write");
	  exit (1);
	}
      fprintf (stderr, "wrote %d byte pad.\n", i);
    }

  /* Copy the loadable sections.   Zero-fill any gaps less than 64k;
     complain about any zero-filling, and die if we're asked to zero-fill
     more than 64k. */
  for (i = 0; i < ex.phcount; i++)
    {
      /* Unprocessable sections were handled above, so just verify that
	 the section can be loaded before copying. */
      if (ph [i].type == PT_LOAD && ph [i].filesz)
	{
	  if (cur_vma != ph [i].vaddr)
	    {
	      unsigned long gap = ph [i].vaddr - cur_vma;
	      char obuf [1024];
	      if (gap > 65536)
		{
		  fprintf (stderr, "Intersegment gap (%d bytes) too large.\n",
			   gap);
		  exit (1);
		}
	      fprintf (stderr, "Warning: %d byte intersegment gap.\n", gap);
	      memset (obuf, 0, sizeof obuf);
	      while (gap)
		{
		  int count = write (outfile, obuf, (gap > sizeof obuf
						     ? sizeof obuf : gap));
		  if (count < 0)
		    {
		      fprintf (stderr, "Error writing gap: %s\n",
			       strerror (errno));
		      exit (1);
		    }
		  gap -= count;
		}
	    }
fprintf (stderr, "writing %d bytes...\n", ph [i].filesz);
	  copy (outfile, infile, ph [i].offset, ph [i].filesz);
	  cur_vma = ph [i].vaddr + ph [i].filesz;
	}
    }

  /* Looks like we won... */
  exit (0);
}

copy (out, in, offset, size)
     int out, in;
     off_t offset, size;
{
  char ibuf [4096];
  int remaining, cur, count;

  /* Go the the start of the ELF symbol table... */
  if (lseek (in, offset, SEEK_SET) < 0)
    {
      perror ("copy: lseek");
      exit (1);
    }

  remaining = size;
  while (remaining)
    {
      cur = remaining;
      if (cur > sizeof ibuf)
	cur = sizeof ibuf;
      remaining -= cur;
      if ((count = read (in, ibuf, cur)) != cur)
	{
	  fprintf (stderr, "copy: read: %s\n",
		   count ? strerror (errno) : "premature end of file");
	  exit (1);
	}
      if ((count = write (out, ibuf, cur)) != cur)
	{
	  perror ("copy: write");
	  exit (1);
	}
    }
}

/* Combine two segments, which must be contiguous.   If pad is true, it's
   okay for there to be padding between. */
combine (base, new, pad)
     struct sect *base, *new;
     int pad;
{
  if (!base -> len)
    *base = *new;
  else if (new -> len)
    {
      if (base -> vaddr + base -> len != new -> vaddr)
	{
	  if (pad)
	    base -> len = new -> vaddr - base -> vaddr;
	  else
	    {
	      fprintf (stderr,
		       "Non-contiguous data can't be converted.\n");
	      exit (1);
	    }
	}
      base -> len += new -> len;
    }
}

phcmp (h1, h2)
     struct phdr *h1, *h2;
{
  if (h1 -> vaddr > h2 -> vaddr)
    return 1;
  else if (h1 -> vaddr < h2 -> vaddr)
    return -1;
  else
    return 0;
}

char *saveRead (int file, off_t offset, off_t len, char *name)
{
  char *tmp;
  int count;
  off_t off;
  if ((off = lseek (file, offset, SEEK_SET)) < 0)
    {
      fprintf (stderr, "%s: fseek: %s\n", name, strerror (errno));
      exit (1);
    }
  if (!(tmp = (char *)malloc (len)))
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
