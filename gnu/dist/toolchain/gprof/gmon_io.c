/* gmon_io.c - Input and output from/to gmon.out files.

   Copyright 2000, 2001 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "cg_arcs.h"
#include "basic_blocks.h"
#include "bfd.h"
#include "corefile.h"
#include "call_graph.h"
#include "gmon_io.h"
#include "gmon_out.h"
#include "gmon.h"		/* Fetch header for old format.  */
#include "gprof.h"
#include "hertz.h"
#include "hist.h"
#include "libiberty.h"

int gmon_input = 0;
int gmon_file_version = 0;	/* 0 == old (non-versioned) file format.  */

int
DEFUN (gmon_io_read_32, (ifp, valp), FILE * ifp AND unsigned int *valp)
{
  char buf[4];

  if (fread (buf, 1, 4, ifp) != 4)
    return 1;
  *valp = bfd_get_32 (core_bfd, buf);
  return 0;
}

int
DEFUN (gmon_io_read_64, (ifp, valp), FILE * ifp AND BFD_HOST_U_64_BIT *valp)
{
  char buf[8];

  if (fread (buf, 1, 8, ifp) != 8)
    return 1;
  *valp = bfd_get_64 (core_bfd, buf);
  return 0;
}

int
DEFUN (gmon_io_read_vma, (ifp, valp), FILE * ifp AND bfd_vma *valp)
{
  char buf[8];
  bfd_vma val;
  unsigned int val32;
  BFD_HOST_U_64_BIT val64;

  switch (bfd_arch_bits_per_address (core_bfd))
    {
    case 32:
      if (gmon_io_read_32 (ifp, &val32))
	return 1;
      val = val32;
      break;

    case 64:
      if (gmon_io_read_64 (ifp, &val64))
	return 1;
      val = val64;
      break;

    default:
      fprintf (stderr, _("%s: bits per address has unexpected value of %u\n"),
	       whoami, bfd_arch_bits_per_address (core_bfd));
      done (1);
    }
  *valp = val;
  return 0;
}

int
DEFUN (gmon_io_read, (ifp, buf, n), FILE * ifp AND char *buf AND size_t n)
{
  if (fread (buf, 1, n, ifp) != n)
    return 1;
  return 0;
}

int
DEFUN (gmon_io_write_32, (ofp, val), FILE * ofp AND unsigned int val)
{
  char buf[4];

  bfd_put_32 (core_bfd, val, buf);
  if (fwrite (buf, 1, 4, ofp) != 4)
    return 1;
  return 0;
}

int
DEFUN (gmon_io_write_64, (ofp, val), FILE * ofp AND BFD_HOST_U_64_BIT val)
{
  char buf[8];

  bfd_put_64 (core_bfd, val, buf);
  if (fwrite (buf, 1, 8, ofp) != 8)
    return 1;
  return 0;
}

int
DEFUN (gmon_io_write_vma, (ofp, val), FILE * ofp AND bfd_vma val)
{
  char buf[8];

  switch (bfd_arch_bits_per_address (core_bfd))
    {
    case 32:
      if (gmon_io_write_32 (ofp, (unsigned int) val))
	return 1;
      break;

    case 64:
      if (gmon_io_write_64 (ofp, (BFD_HOST_U_64_BIT) val))
	return 1;
      break;

    default:
      fprintf (stderr, _("%s: bits per address has unexpected value of %u\n"),
	       whoami, bfd_arch_bits_per_address (core_bfd));
      done (1);
    }
  return 0;
}

int
DEFUN (gmon_io_write_8, (ofp, val), FILE * ofp AND unsigned char val)
{
  char buf[1];

  bfd_put_8 (core_bfd, val, buf);
  if (fwrite (buf, 1, 1, ofp) != 1)
    return 1;
  return 0;
}

int
DEFUN (gmon_io_write, (ofp, buf, n), FILE * ofp AND char *buf AND size_t n)
{
  if (fwrite (buf, 1, n, ofp) != n)
    return 1;
  return 0;
}

int
DEFUN (gmon_read_raw_arc, (ifp, fpc, spc, cnt), FILE * ifp AND bfd_vma *fpc AND bfd_vma *spc AND unsigned long *cnt)
{
  BFD_HOST_U_64_BIT cnt64;
  unsigned int cnt32;

  if (gmon_io_read_vma (ifp, fpc)
      || gmon_io_read_vma (ifp, spc))
    return 1;

  switch (bfd_arch_bits_per_address (core_bfd))
    {
    case 32:
      if (gmon_io_read_32 (ifp, &cnt32))
	return 1;
      *cnt = cnt32;
      break;

    case 64:
      if (gmon_io_read_64 (ifp, &cnt64))
	return 1;
      *cnt = cnt64;
      break;

    default:
      fprintf (stderr, _("%s: bits per address has unexpected value of %u\n"),
	       whoami, bfd_arch_bits_per_address (core_bfd));
      done (1);
    }
  return 0;
}

int
DEFUN (gmon_write_raw_arc, (ofp, fpc, spc, cnt), FILE * ofp AND bfd_vma fpc AND bfd_vma spc AND unsigned long cnt)
{

  if (gmon_io_write_vma (ofp, fpc)
      || gmon_io_write_vma (ofp, spc))
    return 1;

  switch (bfd_arch_bits_per_address (core_bfd))
    {
    case 32:
      if (gmon_io_write_32 (ofp, (unsigned int) cnt))
	return 1;
      break;

    case 64:
      if (gmon_io_write_64 (ofp, (BFD_HOST_U_64_BIT) cnt))
	return 1;
      break;

    default:
      fprintf (stderr, _("%s: bits per address has unexpected value of %u\n"),
	       whoami, bfd_arch_bits_per_address (core_bfd));
      done (1);
    }
  return 0;
}

void
DEFUN (gmon_out_read, (filename), const char *filename)
{
  FILE *ifp;
  struct gmon_hdr ghdr;
  unsigned char tag;
  int nhist = 0, narcs = 0, nbbs = 0;

  /* Open gmon.out file.  */
  if (strcmp (filename, "-") == 0)
    {
      ifp = stdin;
#ifdef SET_BINARY
      SET_BINARY (fileno (stdin));
#endif
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
	int ncnt;
      };
      int i, samp_bytes, header_size;
      unsigned long count;
      bfd_vma from_pc, self_pc;
      static struct hdr h;
      UNIT raw_bin_count;
      struct hdr tmp;
      int version;

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
	  int profrate;

	  /* 4.4BSD format header.  */
          if (gmon_io_read_32 (ifp, &profrate))
	    goto bad_gmon_file;

	  if (!s_highpc)
	    hz = profrate;
	  else if (hz != profrate)
	    {
	      fprintf (stderr,
		       _("%s: profiling rate incompatible with first gmon file\n"),
		       filename);
	      done (1);
	    }

	  switch (bfd_arch_bits_per_address (core_bfd))
	    {
	    case 32:
	      header_size = GMON_HDRSIZE_BSD44_32;
	      break;

	    case 64:
	      header_size = GMON_HDRSIZE_BSD44_64;
	      break;

	    default:
              fprintf (stderr,
                       _("%s: bits per address has unexpected value of %u\n"),
	               whoami, bfd_arch_bits_per_address (core_bfd));
              done (1);
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

	  switch (bfd_arch_bits_per_address (core_bfd))
	    {
	    case 32:
	      header_size = GMON_HDRSIZE_OLDBSD_32;
	      break;

	    case 64:
	      header_size = GMON_HDRSIZE_OLDBSD_64;
	      break;

	    default:
              fprintf (stderr,
                       _("%s: bits per address has unexpected value of %u\n"),
	               whoami, bfd_arch_bits_per_address (core_bfd));
              done (1);
	    }
	}

      /* Position the file to after the header.  */
      if (fseek (ifp, header_size, SEEK_SET) < 0)
	{
	  perror (filename);
	  done (1);
	}

      if (s_highpc && (tmp.low_pc != h.low_pc
		       || tmp.high_pc != h.high_pc || tmp.ncnt != h.ncnt))
	{
	  fprintf (stderr, _("%s: incompatible with first gmon file\n"),
		   filename);
	  done (1);
	}

      h = tmp;
      s_lowpc = (bfd_vma) h.low_pc;
      s_highpc = (bfd_vma) h.high_pc;
      lowpc = (bfd_vma) h.low_pc / sizeof (UNIT);
      highpc = (bfd_vma) h.high_pc / sizeof (UNIT);
      samp_bytes = h.ncnt - header_size;
      hist_num_bins = samp_bytes / sizeof (UNIT);

      DBG (SAMPLEDEBUG,
	   printf ("[gmon_out_read] lowpc 0x%lx highpc 0x%lx ncnt %d\n",
		   (unsigned long) h.low_pc, (unsigned long) h.high_pc,
		   h.ncnt);
	   printf ("[gmon_out_read]   s_lowpc 0x%lx   s_highpc 0x%lx\n",
		   (unsigned long) s_lowpc, (unsigned long) s_highpc);
	   printf ("[gmon_out_read]     lowpc 0x%lx     highpc 0x%lx\n",
		   (unsigned long) lowpc, (unsigned long) highpc);
	   printf ("[gmon_out_read] samp_bytes %d hist_num_bins %d\n",
		   samp_bytes, hist_num_bins));

      /* Make sure that we have sensible values.  */
      if (samp_bytes < 0 || lowpc > highpc)
	{
	  fprintf (stderr,
	    _("%s: file '%s' does not appear to be in gmon.out format\n"),
	    whoami, filename);
	  done (1);
	}

      if (hist_num_bins)
	++nhist;

      if (!hist_sample)
	{
	  hist_sample =
	    (int *) xmalloc (hist_num_bins * sizeof (hist_sample[0]));

	  memset (hist_sample, 0, hist_num_bins * sizeof (hist_sample[0]));
	}

      for (i = 0; i < hist_num_bins; ++i)
	{
	  if (fread (raw_bin_count, sizeof (raw_bin_count), 1, ifp) != 1)
	    {
	      fprintf (stderr,
		       _("%s: unexpected EOF after reading %d/%d bins\n"),
		       whoami, --i, hist_num_bins);
	      done (1);
	    }

	  hist_sample[i] += bfd_get_16 (core_bfd, (bfd_byte *) raw_bin_count);
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

      fclose (ifp);

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

  if (output_style & STYLE_GMON_INFO)
    {
      printf (_("File `%s' (version %d) contains:\n"),
	      filename, gmon_file_version);
      printf (_("\t%d histogram record%s\n"),
	      nhist, nhist == 1 ? "" : "s");
      printf (_("\t%d call-graph record%s\n"),
	      narcs, narcs == 1 ? "" : "s");
      printf (_("\t%d basic-block count record%s\n"),
	      nbbs, nbbs == 1 ? "" : "s");
      first_output = FALSE;
    }
}


void
DEFUN (gmon_out_write, (filename), const char *filename)
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
      bfd_put_32 (core_bfd, GMON_VERSION, (bfd_byte *) ghdr.version);

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
      int i, hdrsize, padsize;
      char pad[3*4];
      Arc *arc;
      Sym *sym;

      memset (pad, 0, sizeof (pad));

      /* Decide how large the header will be.  Use the 4.4BSD format
         header if explicitly specified, or if the profiling rate is
         non-standard.  Otherwise, use the old BSD format.  */
      if (file_format == FF_BSD44
	  || hz != hertz())
	{
	  padsize = 3*4;
	  switch (bfd_arch_bits_per_address (core_bfd))
	    {
	    case 32:
	      hdrsize = GMON_HDRSIZE_BSD44_32;
	      break;

	    case 64:
	      hdrsize = GMON_HDRSIZE_BSD44_64;
	      break;

	    default:
              fprintf (stderr,
                       _("%s: bits per address has unexpected value of %u\n"),
	               whoami, bfd_arch_bits_per_address (core_bfd));
              done (1);
	    }
	}
      else
	{
	  padsize = 0;
	  switch (bfd_arch_bits_per_address (core_bfd))
	    {
	    case 32:
	      hdrsize = GMON_HDRSIZE_OLDBSD_32;
	      break;

	    case 64:
	      hdrsize = GMON_HDRSIZE_OLDBSD_64;
	      /* FIXME: Checking host compiler defines here means that we can't
		 use a cross gprof alpha OSF.  */ 
#if defined(__alpha__) && defined (__osf__)
	      padsize = 4;
#endif
	      break;

	    default:
              fprintf (stderr,
                       _("%s: bits per address has unexpected value of %u\n"),
	               whoami, bfd_arch_bits_per_address (core_bfd));
              done (1);
	    }
	}

      /* Write the parts of the headers that are common to both the
	 old BSD and 4.4BSD formats.  */
      if (gmon_io_write_vma (ofp, s_lowpc)
          || gmon_io_write_vma (ofp, s_highpc)
          || gmon_io_write_32 (ofp, hist_num_bins * sizeof (UNIT) + hdrsize))
	{
	  perror (filename);
	  done (1);
	}

      /* Write out the 4.4BSD header bits, if that's what we're using.  */
      if (file_format == FF_BSD44
	  || hz != hertz())
	{
          if (gmon_io_write_32 (ofp, GMONVERSION)
	      || gmon_io_write_32 (ofp, hz))
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
      for (i = 0; i < hist_num_bins; ++i)
	{
	  bfd_put_16 (core_bfd, hist_sample[i], (bfd_byte *) & raw_bin_count[0]);
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
