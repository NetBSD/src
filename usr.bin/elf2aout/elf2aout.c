/* $NetBSD: elf2aout.c,v 1.2 1996/09/29 22:01:44 jonathan Exp $ */

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

/* elf2aout.c

   This program converts an elf executable to a NetBSD a.out executable.
   The minimal symbol table is copied, but the debugging symbols and
   other informational sections are not. */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <machine/elf.h>
#include <stdio.h>
#include <a.out.h>
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
  int i;
  struct sect text, data, bss;
  struct exec aex;
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

  /* Find space for a table matching ELF section indices to a.out symbol
     types. */
  symTypeTable = (int *)malloc (ex.shcount * sizeof (int));
  if (!symTypeTable)
    {
      fprintf (stderr, "symTypeTable: can't allocate.\n");
      exit (1);
    }
  memset (symTypeTable, 0, ex.shcount * sizeof (int));

  /* Look for the symbol table and string table...
     Also map section indices to symbol types for a.out */
  for (i = 0; i < ex.shcount; i++)
    {
      char *name = shstrtab + sh [i].name;
      if (!strcmp (name, ".symtab"))
	symtabix = i;
      else if (!strcmp (name, ".strtab"))
	strtabix = i;
      else if (!strcmp (name, ".text") || !strcmp (name, ".rodata"))
	symTypeTable [i] = N_TEXT;
      else if (!strcmp (name, ".data") || !strcmp (name, ".sdata") ||
	       !strcmp (name, ".lit4") || !strcmp (name, ".lit8"))
	symTypeTable [i] = N_DATA;
      else if (!strcmp (name, ".bss") || !strcmp (name, ".sbss"))
	symTypeTable [i] = N_BSS;
    }

  /* Figure out if we can cram the program header into an a.out header...
     Basically, we can't handle anything but loadable segments, but we
     can ignore some kinds of segments.   We can't handle holes in the
     address space, and we handle start addresses other than 0x1000 by
     hoping that the loader will know where to load - a.out doesn't have
     an explicit load address.   Segments may be out of order, so we
     sort them first. */
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
  aex.a_midmag = htonl ((symflag << 26) | (MID_PMAX << 16) | OMAGIC);
  aex.a_text = text.len;
  aex.a_data = data.len;
  aex.a_bss = bss.len;
  aex.a_entry = ex.entry;
  aex.a_syms = (sizeof (struct nlist) *
		(symtabix != -1
		 ? sh [symtabix].size / sizeof (struct sym) : 0));
  aex.a_trsize = 0;
  aex.a_drsize = 0;

  /* Make the output file... */
  if ((outfile = open (argv [2], O_WRONLY | O_CREAT, 0777)) < 0)
    {
      fprintf (stderr, "Unable to create %s: %s\n", argv [2], strerror (errno));
      exit (1);
    }
  /* Write the header... */
  i = write (outfile, &aex, sizeof aex);
  if (i != sizeof aex)
    {
      perror ("aex: write");
      exit (1);
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
	  copy (outfile, infile, ph [i].offset, ph [i].filesz);
	  cur_vma = ph [i].vaddr + ph [i].filesz;
	}
    }

  /* Copy and translate the symbol table... */
  translate_syms (outfile, infile, sh [symtabix].offset, sh [symtabix].size,
		  sh [strtabix].offset, sh [strtabix].size);

  /* Looks like we won... */
  exit (0);
}

/* translate_syms (out, in, offset, size)

   Read the ELF symbol table from in at offset; translate it into a.out
   nlist format and write it to out. */

translate_syms (out, in, symoff, symsize, stroff, strsize)
     int out, in;
     off_t symoff, symsize;
     off_t stroff, strsize;
{
# define SYMS_PER_PASS	64
  struct sym inbuf [64];
  struct nlist outbuf [64];
  int i, remaining, cur;
  char *oldstrings;
  char *newstrings, *nsp;
  int newstringsize;

  /* Zero the unused fields in the output buffer.. */
  memset (outbuf, 0, sizeof outbuf);

  /* Find number of symbols to process... */
  remaining = symsize / sizeof (struct sym);

  /* Suck in the old string table... */
  oldstrings = saveRead (in, stroff, strsize, "string table");

  /* Allocate space for the new one.   XXX We make the wild assumption that
     no two symbol table entries will point at the same place in the
     string table - if that assumption is bad, this could easily blow up. */
  newstringsize = strsize + remaining;
  newstrings = (char *)malloc (newstringsize);
  if (!newstrings)
    {
      fprintf (stderr, "No memory for new string table!\n");
      exit (1);
    }
  /* Initialize the table pointer... */
  nsp = newstrings;

  /* Go the the start of the ELF symbol table... */
  if (lseek (in, symoff, SEEK_SET) < 0)
    {
      perror ("translate_syms: lseek");
      exit (1);
    }

  /* Translate and copy symbols... */
  while (remaining)
    {
      cur = remaining;
      if (cur > SYMS_PER_PASS)
	cur = SYMS_PER_PASS;
      remaining -= cur;
      if ((i = read (in, inbuf, cur * sizeof (struct sym)))
	  != cur * sizeof (struct sym))
	{
	  if (i < 0)
	    perror ("translate_syms");
	  else
	    fprintf (stderr, "translate_syms: premature end of file.\n");
	  exit (1);
	}

      /* Do the translation... */
      for (i = 0; i < cur; i++)
	{
	  /* Copy the symbol into the new table, but prepend an underscore. */
	  *nsp = '_';
	  strcpy (nsp + 1, oldstrings + inbuf [i].name);
	  outbuf [i].n_un.n_strx = nsp - newstrings + 4;
	  nsp += strlen (nsp) + 1;

	  /* Convert ELF symbol type/section/etc info into a.out type info. */
	  if (inbuf [i].type == STT_FILE)
	    outbuf [i].n_type = N_FN;
	  else if (inbuf [i].shndx == SHN_UNDEF)
	    outbuf [i].n_type = N_UNDF;
	  else if (inbuf [i].shndx == SHN_ABS)
	    outbuf [i].n_type = N_ABS;
	  else if (inbuf [i].shndx == SHN_COMMON ||
		 inbuf [i].shndx == SHN_MIPS_ACOMMON)
	    outbuf [i].n_type = N_COMM;
	  else
	    outbuf [i].n_type = symTypeTable [inbuf [i].shndx];
	  if (inbuf [i].binding == STB_GLOBAL)
	    outbuf [i].n_type |= N_EXT;
	  /* Symbol values in executables should be compatible. */
	  outbuf [i].n_value = inbuf [i].value;
	}
      /* Write out the symbols... */
      if ((i = write (out, outbuf, cur * sizeof (struct nlist)))
	  != cur * sizeof (struct nlist))
	{
	  fprintf (stderr, "translate_syms: write: %s\n", strerror (errno));
	  exit (1);
	}
    }
  /* Write out the string table length... */
  if (write (out, &newstringsize, sizeof newstringsize)
      != sizeof newstringsize)
    {
      fprintf (stderr,
	       "translate_syms: newstringsize: %s\n", strerror (errno));
      exit (1);
    }
  /* Write out the string table... */
  if (write (out, newstrings, newstringsize) != newstringsize)
    {
      fprintf (stderr, "translate_syms: newstrings: %s\n", strerror (errno));
      exit (1);
    }
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
