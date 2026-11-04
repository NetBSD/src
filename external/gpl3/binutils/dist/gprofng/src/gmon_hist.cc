/* hist.c  -  Histogram related operations.

   Copyright (C) 1999-2026 Free Software Foundation, Inc.

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

#include "config.h"
#include "util.h"
#include "bfd.h"
#include "gp-gmon.h"

#include "symtab.h"
#include "gmon_io.h"
#include "gmon_out.h"
#include "hist.h"

extern int print_name_only (Sym * self);
extern void print_name (Sym * self);

#include <math.h>
#define _(String) (String)

typedef unsigned char UNIT[2];	/* unit of profiling */
#define UNITS_TO_CODE (offset_to_code / sizeof(UNIT))

static long hz = 0;

/* Given a range of addresses for a symbol, find a histogram record
   that intersects with this range, and clips the range to that
   histogram record, modifying *P_LOWPC and *P_HIGHPC.

   If no intersection is found, *P_LOWPC and *P_HIGHPC will be set to
   one unspecified value.  If more that one intersection is found,
   an error is emitted.  */
static void hist_clip_symbol_address (bfd_vma *p_lowpc,
				      bfd_vma *p_highpc, const char *whoami);


/* Declarations of automatically generated functions to output blurbs.  */
extern void flat_blurb (FILE * fp);

static histogram *find_histogram (bfd_vma lowpc, bfd_vma highpc);
static histogram *find_histogram_for_pc (bfd_vma pc);

static histogram * histograms;
static unsigned num_histograms;

/* Scale factor converting samples to pc values:
   each sample covers HIST_SCALE bytes.  */
static double hist_scale;
static char hist_dimension[16] = "seconds";
static char hist_dimension_abbrev = 's';

static double total_time;	/* Total time for all routines.  */

/* Table of SI prefixes for powers of 10 (used to automatically
   scale some of the values in the flat profile).  */
const struct
  {
    char prefix;
    double scale;
  }
SItab[] =
{
  { 'T', 1e-12 },				/* tera */
  { 'G', 1e-09 },				/* giga */
  { 'M', 1e-06 },				/* mega */
  { 'K', 1e-03 },				/* kilo */
  { ' ', 1e-00 },
  { 'm', 1e+03 },				/* milli */
  { 'u', 1e+06 },				/* micro */
  { 'n', 1e+09 },				/* nano */
  { 'p', 1e+12 },				/* pico */
  { 'f', 1e+15 },				/* femto */
  { 'a', 1e+18 }				/* ato */
};

/* Reads just the header part of histogram record into
   *RECORD from IFP.  FILENAME is the name of IFP and
   is provided for formatting error messages only.

   If FIRST is non-zero, sets global variables HZ, HIST_DIMENSION,
   HIST_DIMENSION_ABBREV, HIST_SCALE.  If FIRST is zero, checks
   that the new histogram is compatible with already-set values
   of those variables and emits an error if that's not so.  */
static void
read_histogram_header (histogram *record,
		       FILE *ifp, const char *filename,
		       int first, const char *whoami)
{
  unsigned int profrate;
  char n_hist_dimension[15];
  char n_hist_dimension_abbrev;
  double n_hist_scale;

  if (gmon_io_read_vma (ifp, &record->lowpc, whoami)
      || gmon_io_read_vma (ifp, &record->highpc, whoami)
      || gmon_io_read_32 (ifp, &record->num_bins)
      || gmon_io_read_32 (ifp, &profrate)
      || gmon_io_read (ifp, n_hist_dimension, 15)
      || gmon_io_read (ifp, &n_hist_dimension_abbrev, 1))
    {
      fprintf (stderr, "%s: %s: unexpected end of file\n",
	       whoami, filename);

      done (1);
    }

  n_hist_scale = (double)(record->highpc - record->lowpc) / sizeof (UNIT)
    / record->num_bins;

  if (first)
    {
      /* We don't try to verify profrate is the same for all histogram
	 records.  If we have two histogram records for the same
	 address range and profiling samples is done as often
	 as possible as opposed on timer, then the actual profrate will
	 be slightly different.  Most of the time the difference does not
	 matter and insisting that profiling rate is exactly the same
	 will only create inconvenient.  */
      hz = profrate;
      memcpy (hist_dimension, n_hist_dimension, 15);
      hist_dimension_abbrev = n_hist_dimension_abbrev;
      hist_scale = n_hist_scale;
    }
  else
    {
      if (strncmp (n_hist_dimension, hist_dimension, 15) != 0)
	{
	  fprintf (stderr,
		   "%s: dimension unit changed between histogram records\n"
		     "%s: from '%s'\n"
		     "%s: to '%s'\n",
		   whoami, whoami, hist_dimension, whoami, n_hist_dimension);
	  done (1);
	}

      if (n_hist_dimension_abbrev != hist_dimension_abbrev)
	{
	  fprintf (stderr,
		   _("%s: dimension abbreviation changed between histogram records\n"
		     "%s: from '%c'\n"
		     "%s: to '%c'\n"),
		   whoami, whoami, hist_dimension_abbrev, whoami, n_hist_dimension_abbrev);
	  done (1);
	}

      /* The only reason we require the same scale for histograms is that
	 there's code (notably printing code), that prints units,
	 and it would be very confusing to have one unit mean different
	 things for different functions.  */
      if (fabs (hist_scale - n_hist_scale) > 0.000001)
	{
	  fprintf (stderr,
		   _("%s: different scales in histogram records: %f != %f\n"),
		   whoami, hist_scale, n_hist_scale);
	  done (1);
	}
    }
}

/* Read the histogram from file IFP.  FILENAME is the name of IFP and
   is provided for formatting error messages only.  */

void
hist_read_rec (FILE * ifp, const char *filename, const char *whoami)
{
  bfd_vma lowpc, highpc;
  histogram n_record;
  histogram *record, *existing_record;
  unsigned i;

  /* 1. Read the header and see if there's existing record for the
     same address range and that there are no overlapping records.  */
  read_histogram_header (&n_record, ifp, filename, num_histograms == 0, whoami);

  existing_record = find_histogram (n_record.lowpc, n_record.highpc);
  if (existing_record)
    {
      record = existing_record;
    }
  else
    {
      /* If this record overlaps, but does not completely match an existing
	 record, it's an error.  */
      lowpc = n_record.lowpc;
      highpc = n_record.highpc;
      hist_clip_symbol_address (&lowpc, &highpc, whoami);
      if (lowpc != highpc)
	{
	  fprintf (stderr,
		   _("%s: overlapping histogram records\n"),
		   whoami);
	  done (1);
	}

      /* This is new record.  Add it to global array and allocate space for
	 the samples.  */
      histograms = (struct histogram *)
          xrealloc (histograms, sizeof (histogram) * (num_histograms + 1));
      memcpy (histograms + num_histograms,
	      &n_record, sizeof (histogram));
      record = &histograms[num_histograms];
      ++num_histograms;

      record->sample = (int *) xmalloc (record->num_bins
					* sizeof (record->sample[0]));
      memset (record->sample, 0, record->num_bins * sizeof (record->sample[0]));
    }

  /* 2. We have either a new record (with zeroed histogram data), or an existing
     record with some data in the histogram already.  Read new data into the
     record, adding hit counts.  */

  DBG (SAMPLEDEBUG,
       printf ("[hist_read_rec] n_lowpc 0x%lx n_highpc 0x%lx ncnt %u\n",
	       (unsigned long) record->lowpc, (unsigned long) record->highpc,
               record->num_bins));

  for (i = 0; i < record->num_bins; ++i)
    {
      UNIT count;
      if (fread (&count[0], sizeof (count), 1, ifp) != 1)
	{
	  fprintf (stderr,
		  _("%s: %s: unexpected EOF after reading %u of %u samples\n"),
		   whoami, filename, i, record->num_bins);
	  done (1);
	}
      record->sample[i] += bfd_get_16 (core_bfd, (bfd_byte *) & count[0]);
      DBG (SAMPLEDEBUG,
	   printf ("[hist_read_rec] 0x%lx: %u\n",
		   (unsigned long) (record->lowpc
                                    + i * (record->highpc - record->lowpc)
                                    / record->num_bins),
		   record->sample[i]));
    }
}

/* Calculate scaled entry point addresses (to save time in
   hist_assign_samples), and, on architectures that have procedure
   entry masks at the start of a function, possibly push the scaled
   entry points over the procedure entry mask, if it turns out that
   the entry point is in one bin and the code for a routine is in the
   next bin.  */

static void
scale_and_align_entries (const char *whoami)
{
  Sym *sym;
  bfd_vma bin_of_entry;
  bfd_vma bin_of_code;
  Sym_Table *symtab = get_symtab (whoami);

  for (sym = symtab->base; sym < symtab->limit; sym++)
    {
      histogram *r = find_histogram_for_pc (sym->addr);

      sym->hist.scaled_addr = sym->addr / sizeof (UNIT);

      if (r)
	{
	  bin_of_entry = (sym->hist.scaled_addr - r->lowpc) / hist_scale;
	  bin_of_code = ((sym->hist.scaled_addr + UNITS_TO_CODE - r->lowpc)
		     / hist_scale);
	  if (bin_of_entry < bin_of_code)
	    {
	      DBG (SAMPLEDEBUG,
		   printf ("[scale_and_align_entries] pushing 0x%lx to 0x%lx\n",
			   (unsigned long) sym->hist.scaled_addr,
			   (unsigned long) (sym->hist.scaled_addr
					    + UNITS_TO_CODE)));
	      sym->hist.scaled_addr += UNITS_TO_CODE;
	    }
	}
    }
}


/* Assign samples to the symbol to which they belong.

   Histogram bin I covers some address range [BIN_LOWPC,BIN_HIGH_PC)
   which may overlap one more symbol address ranges.  If a symbol
   overlaps with the bin's address range by O percent, then O percent
   of the bin's count is credited to that symbol.

   There are three cases as to where BIN_LOW_PC and BIN_HIGH_PC can be
   with respect to the symbol's address range [SYM_LOW_PC,
   SYM_HIGH_PC) as shown in the following diagram.  OVERLAP computes
   the distance (in UNITs) between the arrows, the fraction of the
   sample that is to be credited to the symbol which starts at
   SYM_LOW_PC.

	  sym_low_pc                                      sym_high_pc
	       |                                               |
	       v                                               v

	       +-----------------------------------------------+
	       |                                               |
	  |  ->|    |<-         ->|         |<-         ->|    |<-  |
	  |         |             |         |             |         |
	  +---------+             +---------+             +---------+

	  ^         ^             ^         ^             ^         ^
	  |         |             |         |             |         |
     bin_low_pc bin_high_pc  bin_low_pc bin_high_pc  bin_low_pc bin_high_pc

   For the VAX we assert that samples will never fall in the first two
   bytes of any routine, since that is the entry mask, thus we call
   scale_and_align_entries() to adjust the entry points if the entry
   mask falls in one bin but the code for the routine doesn't start
   until the next bin.  In conjunction with the alignment of routine
   addresses, this should allow us to have only one sample for every
   four bytes of text space and never have any overlap (the two end
   cases, above).  */

static void
hist_assign_samples_1 (histogram *r, const char *whoami)
{
  bfd_vma bin_low_pc, bin_high_pc;
  bfd_vma sym_low_pc, sym_high_pc;
  bfd_vma overlap;
  unsigned int bin_count;
  unsigned int i, j, k;
  double count_time, credit;
  Sym_Table *symtab = get_symtab (whoami);

  bfd_vma lowpc = r->lowpc / sizeof (UNIT);

  /* Iterate over all sample bins.  */
  for (i = 0, k = 1; i < r->num_bins; ++i)
    {
      bin_count = r->sample[i];
      if (! bin_count)
	continue;

      bin_low_pc = lowpc + (bfd_vma) (hist_scale * i);
      bin_high_pc = lowpc + (bfd_vma) (hist_scale * (i + 1));
      count_time = bin_count;

      DBG (SAMPLEDEBUG,
	   printf (
      "[assign_samples] bin_low_pc=0x%lx, bin_high_pc=0x%lx, bin_count=%u\n",
		    (unsigned long) (sizeof (UNIT) * bin_low_pc),
		    (unsigned long) (sizeof (UNIT) * bin_high_pc),
		    bin_count));
      total_time += count_time;

      /* Credit all symbols that are covered by bin I.

         PR gprof/13325: Make sure that K does not get decremented
	 and J will never be less than 0.  */
      for (j = k - 1; j < symtab->len; k = ++j)
	{
	  sym_low_pc = symtab->base[j].hist.scaled_addr;
	  sym_high_pc = symtab->base[j + 1].hist.scaled_addr;

	  /* If high end of bin is below entry address,
	     go for next bin.  */
	  if (bin_high_pc < sym_low_pc)
	    break;

	  /* If low end of bin is above high end of symbol,
	     go for next symbol.  */
	  if (bin_low_pc >= sym_high_pc)
	    continue;

	  overlap =
	    MIN (bin_high_pc, sym_high_pc) - MAX (bin_low_pc, sym_low_pc);
	  if (overlap > 0)
	    {
	      DBG (SAMPLEDEBUG,
		   printf (
	       "[assign_samples] [0x%lx,0x%lx) %s gets %f ticks %ld overlap\n",
			   (unsigned long) symtab->base[j].addr,
			   (unsigned long) (sizeof (UNIT) * sym_high_pc),
			   symtab->base[j].name, overlap * count_time / hist_scale,
			   (long) overlap));

	      credit = overlap * count_time / hist_scale;
	      symtab->base[j].hist.time += credit;
	    }
	}
    }

  DBG (SAMPLEDEBUG, printf ("[assign_samples] total_time %f\n",
			    total_time));
}

/* Calls 'hist_assign_samples_1' for all histogram records read so far. */
void
hist_assign_samples (const char *whoami)
{
  unsigned i;

  scale_and_align_entries (whoami);

  for (i = 0; i < num_histograms; ++i)
    hist_assign_samples_1 (&histograms[i], whoami);

}

#if ! defined(min)
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif
#if ! defined(max)
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

static void
hist_clip_symbol_address (bfd_vma *p_lowpc,
			  bfd_vma *p_highpc, const char *whoami)
{
  unsigned i;
  int found = 0;

  if (num_histograms == 0)
    {
      *p_highpc = *p_lowpc;
      return;
    }

  for (i = 0; i < num_histograms; ++i)
    {
      bfd_vma common_low, common_high;
      common_low = max (histograms[i].lowpc, *p_lowpc);
      common_high = min (histograms[i].highpc, *p_highpc);

      if (common_low < common_high)
	{
	  if (found)
	    {
	      fprintf (stderr,
		       _("%s: found a symbol that covers "
			 "several histogram records"),
			 whoami);
	      done (1);
	    }

	  found = 1;
	  *p_lowpc = common_low;
	  *p_highpc = common_high;
	}
    }

  if (!found)
    *p_highpc = *p_lowpc;
}

/* Find and return existing histogram record having the same lowpc and
   highpc as passed via the parameters.  Return NULL if nothing is found.
   The return value is valid until any new histogram is read.  */
static histogram *
find_histogram (bfd_vma lowpc, bfd_vma highpc)
{
  unsigned i;
  for (i = 0; i < num_histograms; ++i)
    {
      if (histograms[i].lowpc == lowpc && histograms[i].highpc == highpc)
	return &histograms[i];
    }
  return 0;
}

/* Given a PC, return histogram record which address range include this PC.
   Return NULL if there's no such record.  */
static histogram *
find_histogram_for_pc (bfd_vma pc)
{
  unsigned i;
  for (i = 0; i < num_histograms; ++i)
    {
      if (histograms[i].lowpc <= pc && pc < histograms[i].highpc)
	return &histograms[i];
    }
  return 0;
}

/* Return the profile rate in hz. */
long
hist_get_hz (void)
{
  return hz;
}
