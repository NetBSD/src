/* gmon_io.c - Input and output from/to gmon.out files.

   Copyright (C) 1999-2018 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "gprof.h"
#include "binary-io.h"
#include "search_list.h"
#include "source.h"
#include "symtab.h"
#include "cg_arcs.h"
#include "basic_blocks.h"
#include "corefile.h"
#include "call_graph.h"
#include "gmon_io.h"
#include "gmon_out.h"
#include "gmon.h"		/* Fetch header for old format.  */
#include "hertz.h"
#include "hist.h"
#include "libiberty.h"

enum gmon_ptr_size {
  ptr_32bit,
  ptr_64bit
};

enum gmon_ptr_signedness {
  ptr_signed,
  ptr_unsigned
};

static enum gmon_ptr_size gmon_get_ptr_size (void);
static enum gmon_ptr_signedness gmon_get_ptr_signedness (void);

#ifdef BFD_HOST_U_64_BIT
static int gmon_io_read_64 (FILE *, BFD_HOST_U_64_BIT *);
static int gmon_io_write_64 (FILE *, BFD_HOST_U_64_BIT);
#endif
static int gmon_read_raw_arc
  (FILE *, bfd_vma *, bfd_vma *, unsigned long *);
static int gmon_write_raw_arc
  (FILE *, bfd_vma, bfd_vma, unsigned long);

int gmon_input = 0;
int gmon_file_version = 0;	/* 0 == old (non-versioned) file format.  */

static enum gmon_ptr_size
gmon_get_ptr_size (void)
{
  int size;

  /* Pick best size for pointers.  Start with the ELF size, and if not
     elf go with the architecture's address size.  */
  size = bfd_get_arch_size (core_bfd);
  if (size == -1)
    size = bfd_arch_bits_per_address (core_bfd);

  switch (size)
    {
    case 32:
      return ptr_32bit;

    case 64:
      return ptr_64bit;

    default:
      fprintf (stderr, _("%s: address size has unexpected value of %u\n"),
	       whoami, size);
      done (1);
    }
}

static enum gmon_ptr_signedness
gmon_get_ptr_signedness (void)
{
  int sext;

  /* Figure out whether to sign extend.  If BFD doesn't know, assume no.  */
  sext = bfd_get_sign_extend_vma (core_bfd);
  if (sext == -1)
    return ptr_unsigned;
  return (sext ? ptr_signed : ptr_unsigned);
}

int
gmon_io_read_32 (FILE *ifp, unsigned int *valp)
{
  char buf[4];

  if (fread (buf, 1, 4, ifp) != 4)
    return 1;
  *valp = bfd_get_32 (core_bfd, buf);
  return 0;
}

#ifdef BFD_HOST_U_64_BIT
static int
gmon_io_read_64 (FILE *ifp, BFD_HOST_U_64_BIT *valp)
{
  char buf[8];

  if (fread (buf, 1, 8, ifp) != 8)
    return 1;
  *valp = bfd_get_64 (core_bfd, buf);
  return 0;
}
#endif

int
gmon_io_read_vma (FILE *ifp, bfd_vma *valp)
{
  unsigned int val32;
#ifdef BFD_HOST_U_64_BIT
  BFD_HOST_U_64_BIT val64;
#endif

  switch (gmon_get_ptr_size ())
    {
    case ptr_32bit:
      if (gmon_io_read_32 (ifp, &val32))
	return 1;
      if (gmon_get_ptr_signedness () == ptr_signed)
        *valp = (int) val32;
      else
        *valp = val32;
      break;

#ifdef BFD_HOST_U_64_BIT
    case ptr_64bit:
      if (gmon_io_read_64 (ifp, &val64))
	return 1;
#ifdef BFD_HOST_64_BIT
      if (gmon_get_ptr_signedness () == ptr_signed)
        *valp = (BFD_HOST_64_BIT) val64;
      else
#endif
        *valp = val64;
      break;
#endif
    }
  return 0;
}

int
gmon_io_read (FILE *ifp, char *buf, size_t n)
{
  if (fread (buf, 1, n, ifp) != n)
    return 1;
  return 0;
}

int
gmon_io_write_32 (FILE *ofp, unsigned int val)
{
  char buf[4];

  bfd_put_32 (core_bfd, (bfd_vma) val, buf);
  if (fwrite (buf, 1, 4, ofp) != 4)
    return 1;
  return 0;
}

#ifdef BFD_HOST_U_64_BIT
static int
gmon_io_write_64 (FILE *ofp, BFD_HOST_U_64_BIT val)
{
  char buf[8];

  bfd_put_64 (core_bfd, (bfd_vma) val, buf);
  if (fwrite (buf, 1, 8, ofp) != 8)
    return 1;
  return 0;
}
#endif

int
gmon_io_write_vma (FILE *ofp, bfd_vma val)
{

  switch (gmon_get_ptr_size ())
    {
    case ptr_32bit:
      if (gmon_io_write_32 (ofp, (unsigned int) val))
	return 1;
      break;

#ifdef BFD_HOST_U_64_BIT
    case ptr_64bit:
      if (gmon_io_write_64 (ofp, (BFD_HOST_U_64_BIT) val))
	return 1;
      break;
#endif
    }
  return 0;
}

int
gmon_io_write_8 (FILE *ofp, unsigned int val)
{
  char buf[1];

  bfd_put_8 (core_bfd, val, buf);
  if (fwrite (buf, 1, 1, ofp) != 1)
    return 1;
  return 0;
}

int
gmon_io_write (FILE *ofp, char *buf, size_t n)
{
  if (fwrite (buf, 1, n, ofp) != n)
    return 1;
  return 0;
}

static int
gmon_read_raw_arc (FILE *ifp, bfd_vma *fpc, bfd_vma *spc, unsigned long *cnt)
{
#ifdef BFD_HOST_U_64_BIT
  BFD_HOST_U_64_BIT cnt64;
#endif
  unsigned int cnt32;

  if (gmon_io_read_vma (ifp, fpc)
      || gmon_io_read_vma (ifp, spc))
    return 1;

  switch (gmon_get_ptr_size ())
    {
    case ptr_32bit:
      if (gmon_io_read_32 (ifp, &cnt32))
	return 1;
      *cnt = cnt32;
      break;

#ifdef BFD_HOST_U_64_BIT
    case ptr_64bit:
      if (gmon_io_read_64 (ifp, &cnt64))
	return 1;
      *cnt = cnt64;
      break;
#endif

    default:
      return 1;
    }
  return 0;
}

static int
gmon_write_raw_arc (FILE *ofp, bfd_vma fpc, bfd_vma spc, unsigned long cnt)
{

  if (gmon_io_write_vma (ofp, fpc)
      || gmon_io_write_vma (ofp, spc))
    return 1;

  switch (gmon_get_ptr_size ())
    {
    case ptr_32bit:
      if (gmon_io_write_32 (ofp, (unsigned int) cnt))
	return 1;
      break;

#ifdef BFD_HOST_U_64_BIT
    case ptr_64bit:
      if (gmon_io_write_64 (ofp, (BFD_HOST_U_64_BIT) cnt))
	return 1;
      break;
#endif
    }
  return 0;
}

void
gmon_out_read (const char *filename)
{
  FILE *ifp;
  struct gmon_hdr ghdr;
  unsigned char tag;
  int nhist = 0, narcs = 0, nbbs = 0;

  /* Open gmon.out file.  */
  if (strcmp (filename, "-") == 0)
    {
      ifp = stdin;
      SET_BINARY (fileno (stdin));
    }
  else
    {
      ifp = fopen (filename, FOPEN_RB);

      if (!ifp)
	{
	  perror (filename);
	  done (1);
	}
    }

  if (fread (&ghdr, sizeof (struct gmon_hdr), 1, ifp) != 1)
    {
      fprintf (stderr, _("%s: file too short to be a gmon file\n"),
	       filename);
      done (1);
    }

  if ((file_format == FF_MAGIC)
      || (file_format == FF_AUTO && !strncmp (&ghdr.cookie[0], GMON_MAGIC, 4)))
    {
      if (file_format == FF_MAGIC && strncmp (&ghdr.cookie[0], GMON_MAGIC, 4))
	{
	  fprintf (stderr, _("%s: file `%s' has bad magic cookie\n"),
		   whoami, filename);
	  done (1);
	}

      /* Right magic, so it's probably really a new gmon.out file.  */
      gmon_file_version = bfd_get_32 (core_bfd, (bfd_byte *) ghdr.version);

      if (gmon_file_version != GMON_VERSION && gmon_file_version != 0)
	{
	  fprintf (stderr,
		   _("%s: file `%s' has unsupported version %d\n"),
		   whoami, filename, gmon_file_version);
	  done (1);
	}

      /* Read in all the records.  */
      while (fread (&tag, sizeof (tag), 1, ifp) == 1)
	{
	  switch (tag)
	    {
	    case GMON_TAG_TIME_HIST:
	      ++nhist;
	      gmon_input |= INPUT_HISTOGRAM;
	      hist_read_rec (ifp, filename);
	      break;

	    case GMON_TAG_CG_ARC:
	      ++narcs;
	      gmon_input |= INPUT_CALL_GRAPH;
	      cg_read_rec (ifp, filename);
	      break;

	    case GMON_TAG_BB_COUNT:
	      ++nbbs;
	      gmon_input |= INPUT_BB_COUNTS;
	      bb_read_rec (ifp, filename);
	      break;

	    default:
	      fprintf (stderr,
		       _("%s: %s: found bad tag %d (file corrupted?)\n"),
		       whoami, filename, tag);
	      done (1);
	    }
	}
    }
  else if (file_format == FF_AUTO
	   || file_format == FF_BSD
	   || file_format == FF_BSD44)
    {
      struct hdr
      {
	bfd_vma low_pc;
	bfd_vma high_pc;
	unsigned int ncnt;
      };
      unsigned int i;
      int samp_bytes, header_size = 0;
      unsigned long count;
      bfd_vma from_pc, self_pc;
      UNIT raw_bin_count;
      struct hdr tmp;
      unsigned int version;
      unsigned int hist_num_bins;

      /* Information from a gmon.out file is in two parts: an array of
	 sampling hits within pc ranges, and the arcs.  */
      gmon_input = INPUT_HISTOGRAM | INPUT_CALL_GRAPH;

      /* This fseek() ought to work even on stdin as long as it's
	 not an interactive device (heck, is there anybody who would
	 want to type in a gmon.out at the terminal?).  */
      if (fseek (ifp, 0, SEEK_SET) < 0)
	{
	  perror (filename);
	  done (1);
	}

      /* The beginning of the old BSD header and the 4.4BSD header
	 are the same: lowpc, highpc, ncnt  */
      if (gmon_io_read_vma (ifp, &tmp.low_pc)
          || gmon_io_read_vma (ifp, &tmp.high_pc)
          || gmon_io_read_32 (ifp, &tmp.ncnt))
	{
 bad_gmon_file:
          fprintf (stderr, _("%s: file too short to be a gmon file\n"),
		   filename);
	  done (1);
	}

      /* Check to see if this a 4.4BSD-style header.  */
      if (gmon_io_read_32 (ifp, &version))
	goto bad_gmon_file;

      if (version == GMONVERSION)
	{
	  unsigned int profrate;

	  /* 4.4BSD format header.  */
          if (gmon_io_read_32 (ifp, &profrate))
	    goto bad_gmon_file;

	  if (!histograms)
	    hz = profrate;
	  else if (hz != (int) profrate)
	    {
	      fprintf (stderr,
		       _("%s: profiling rate incompatible with first gmon file\n"),
		       filename);
	      done (1);
	    }

	  switch (gmon_get_ptr_size ())
	    {
	    case ptr_32bit:
	      header_size = GMON_HDRSIZE_BSD44_32;
	      break;

	    case ptr_64bit:
	      header_size = GMON_HDRSIZE_BSD44_64;
	      break;
	    }
	}
      else
	{
	  /* Old style BSD format.  */
	  if (file_format == FF_BSD44)
	    {
	      fprintf (stderr, _("%s: file `%s' has bad magic cookie\n"),
		       whoami, filename);
	      done (1);
	    }

	  switch (gmon_get_ptr_size ())
	    {
	    case ptr_32bit:
	      header_size = GMON_HDRSIZE_OLDBSD_32;
	      break;

	    case ptr_64bit:
	      header_size = GMON_HDRSIZE_OLDBSD_64;
	      break;
	    }
	}

      /* Position the file to after the header.  */
      if (fseek (ifp, header_size, SEEK_SET) < 0)
	{
	  perror (filename);
	  done (1);
	}

      samp_bytes = tmp.ncnt - header_size;
      hist_num_bins = samp_bytes / sizeof (UNIT);
      if (histograms && (tmp.low_pc != histograms->lowpc
			 || tmp.high_pc != histograms->highpc
			 || (hist_num_bins != histograms->num_bins)))
	{
	  fprintf (stderr, _("%s: incompatible with first gmon file\n"),
		   filename);
	  done (1);
	}

      if (!histograms)
	{
	  num_histograms = 1;
	  histograms = (struct histogram *) xmalloc (sizeof (struct histogram));
	  histograms->lowpc = tmp.low_pc;
	  histograms->highpc = tmp.high_pc;
	  histograms->num_bins = hist_num_bins;
	  hist_scale = (double)((tmp.high_pc - tmp.low_pc) / sizeof (UNIT))
	    / hist_num_bins;
	  histograms->sample = (int *) xmalloc (hist_num_bins * sizeof (int));
	  memset (histograms->sample, 0,
		  hist_num_bins * sizeof (int));
	}

      DBG (SAMPLEDEBUG,
	   printf ("[gmon_out_read] lowpc 0x%lx highpc 0x%lx ncnt %d\n",
		   (unsigned long) tmp.low_pc, (unsigned long) tmp.high_pc,
		   tmp.ncnt);
	   printf ("[gmon_out_read] samp_bytes %d hist_num_bins %d\n",
		   samp_bytes, hist_num_bins));

      /* Make sure that we have sensible values.  */
      if (samp_bytes < 0 || histograms->lowpc > histograms->highpc)
	{
	  fprintf (stderr,
	    _("%s: file '%s' does not appear to be in gmon.out format\n"),
	    whoami, filename);
	  done (1);
	}

      if (hist_num_bins)
	++nhist;

      for (i = 0; i < hist_num_bins; ++i)
	{
	  if (fread (raw_bin_count, sizeof (raw_bin_count), 1, ifp) != 1)
	    {
	      fprintf (stderr,
		       _("%s: unexpected EOF after reading %d/%d bins\n"),
		       whoami, --i, hist_num_bins);
	      done (1);
	    }

	  histograms->sample[i]
	    += bfd_get_16 (core_bfd, (bfd_byte *) raw_bin_count);
	}

      /* The rest of the file consists of a bunch of
	 <from,self,count> tuples.  */
      while (gmon_read_raw_arc (ifp, &from_pc, &self_pc, &count) == 0)
	{
	  ++narcs;

	  DBG (SAMPLEDEBUG,
	     printf ("[gmon_out_read] frompc 0x%lx selfpc 0x%lx count %lu\n",
		     (unsigned long) from_pc, (unsigned long) self_pc, count));

	  /* Add this arc.  */
	  cg_tally (from_pc, self_pc, count);
	}

      if (hz == HZ_WRONG)
	{
	  /* How many ticks per second?  If we can't tell, report
	     time in ticks.  */
	  hz = hertz ();

	  if (hz == HZ_WRONG)
	    {
	      hz = 1;
	      fprintf (stderr, _("time is in ticks, not seconds\n"));
	    }
	}
    }
  else
    {
      fprintf (stderr, _("%s: don't know how to deal with file format %d\n"),
	       whoami, file_format);
      done (1);
    }

  if (ifp != stdin)
    fclose (ifp);

  if (output_style & STYLE_GMON_INFO)
    {
      printf (_("File `%s' (version %d) contains:\n"),
	      filename, gmon_file_version);
      printf (nhist == 1 ?
	      _("\t%d histogram record\n") :
	      _("\t%d histogram records\n"), nhist);
      printf (narcs == 1 ?
	      _("\t%d call-graph record\n") :
	      _("\t%d call-graph records\n"), narcs);
      printf (nbbs == 1 ?
	      _("\t%d basic-block count record\n") :
	      _("\t%d basic-block count records\n"), nbbs);
      first_output = FALSE;
    }
}


void
gmon_out_write (const char *filename)
{
  FILE *ofp;
  struct gmon_hdr ghdr;

  ofp = fopen (filename, FOPEN_WB);
  if (!ofp)
    {
      perror (filename);
      done (1);
    }

  if (file_format == FF_AUTO || file_format == FF_MAGIC)
    {
      /* Write gmon header.  */

      memcpy (&ghdr.cookie[0], GMON_MAGIC, 4);
      bfd_put_32 (core_bfd, (bfd_vma) GMON_VERSION, (bfd_byte *) ghdr.version);

      if (fwrite (&ghdr, sizeof (ghdr), 1, ofp) != 1)
	{
	  perror (filename);
	  done (1);
	}

      /* Write execution time histogram if we have one.  */
      if (gmon_input & INPUT_HISTOGRAM)
	hist_write_hist (ofp, filename);

      /* Write call graph arcs if we have any.  */
      if (gmon_input & INPUT_CALL_GRAPH)
	cg_write_arcs (ofp, filename);

      /* Write basic-block info if we have it.  */
      if (gmon_input & INPUT_BB_COUNTS)
	bb_write_blocks (ofp, filename);
    }
  else if (file_format == FF_BSD || file_format == FF_BSD44)
    {
      UNIT raw_bin_count;
      unsigned int i, hdrsize;
      unsigned padsize;
      char pad[3*4];
      Arc *arc;
      Sym *sym;

      memset (pad, 0, sizeof (pad));

      hdrsize = 0;
      /* Decide how large the header will be.  Use the 4.4BSD format
         header if explicitly specified, or if the profiling rate is
         non-standard.  Otherwise, use the old BSD format.  */
      if (file_format == FF_BSD44
	  || hz != hertz())
	{
	  padsize = 3*4;
	  switch (gmon_get_ptr_size ())
	    {
	    case ptr_32bit:
	      hdrsize = GMON_HDRSIZE_BSD44_32;
	      break;

	    case ptr_64bit:
	      hdrsize = GMON_HDRSIZE_BSD44_64;
	      break;
	    }
	}
      else
	{
	  padsize = 0;
	  switch (gmon_get_ptr_size ())
	    {
	    case ptr_32bit:
	      hdrsize = GMON_HDRSIZE_OLDBSD_32;
	      break;

	    case ptr_64bit:
	      hdrsize = GMON_HDRSIZE_OLDBSD_64;
	      /* FIXME: Checking host compiler defines here means that we can't
		 use a cross gprof alpha OSF.  */
#if defined(__alpha__) && defined (__osf__)
	      padsize = 4;
#endif
	      break;
	    }
	}

      /* Write the parts of the headers that are common to both the
	 old BSD and 4.4BSD formats.  */
      if (gmon_io_write_vma (ofp, histograms->lowpc)
          || gmon_io_write_vma (ofp, histograms->highpc)
          || gmon_io_write_32 (ofp, histograms->num_bins
			       * sizeof (UNIT) + hdrsize))
	{
	  perror (filename);
	  done (1);
	}

      /* Write out the 4.4BSD header bits, if that's what we're using.  */
      if (file_format == FF_BSD44
	  || hz != hertz())
	{
          if (gmon_io_write_32 (ofp, GMONVERSION)
	      || gmon_io_write_32 (ofp, (unsigned int) hz))
	    {
	      perror (filename);
	      done (1);
	    }
	}

      /* Now write out any necessary padding after the meaningful
	 header bits.  */
      if (padsize != 0
          && fwrite (pad, 1, padsize, ofp) != padsize)
        {
          perror (filename);
	  done (1);
	}

      /* Dump the samples.  */
      for (i = 0; i < histograms->num_bins; ++i)
	{
	  bfd_put_16 (core_bfd, (bfd_vma) histograms->sample[i],
		      (bfd_byte *) &raw_bin_count[0]);
	  if (fwrite (&raw_bin_count[0], sizeof (raw_bin_count), 1, ofp) != 1)
	    {
	      perror (filename);
	      done (1);
	    }
	}

      /* Dump the normalized raw arc information.  */
      for (sym = symtab.base; sym < symtab.limit; ++sym)
	{
	  for (arc = sym->cg.children; arc; arc = arc->next_child)
	    {
	      if (gmon_write_raw_arc (ofp, arc->parent->addr,
				      arc->child->addr, arc->count))
		{
		  perror (filename);
		  done (1);
		}
	      DBG (SAMPLEDEBUG,
		   printf ("[dumpsum] frompc 0x%lx selfpc 0x%lx count %lu\n",
			   (unsigned long) arc->parent->addr,
			   (unsigned long) arc->child->addr, arc->count));
	    }
	}

      fclose (ofp);
    }
  else
    {
      fprintf (stderr, _("%s: don't know how to deal with file format %d\n"),
	       whoami, file_format);
      done (1);
    }
}
