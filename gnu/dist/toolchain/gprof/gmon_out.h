/* gmon_out.h
   
   Copyright (C) 2000  Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This file specifies the format of gmon.out files.  It should have
   as few external dependencies as possible as it is going to be
   included in many different programs.  That is, minimize the
   number of #include's.
  
   A gmon.out file consists of a header (defined by gmon_hdr) followed
   by a sequence of records.  Each record starts with a one-byte tag
   identifying the type of records, followed by records specific data.  */
#ifndef gmon_out_h
#define gmon_out_h

#define	GMON_MAGIC	"gmon"	/* magic cookie */
#define GMON_VERSION	1	/* version number */

/* Raw header as it appears on file (without padding).  */
struct gmon_hdr
  {
    char cookie[4];
    char version[4];
    char spare[3 * 4];
  };

/* Types of records in this file.  */
typedef enum
  {
    GMON_TAG_TIME_HIST = 0, GMON_TAG_CG_ARC = 1, GMON_TAG_BB_COUNT = 2
  }
GMON_Record_Tag;

struct gmon_hist_hdr
  {
    char low_pc[sizeof (char*)];	/* Base pc address of sample buffer.  */
    char high_pc[sizeof (char*)];	/* Max pc address of sampled buffer.  */
    char hist_size[4];			/* Size of sample buffer.  */
    char prof_rate[4];			/* Profiling clock rate.  */
    char dimen[15];			/* Phys. dim., usually "seconds".  */
    char dimen_abbrev;			/* Usually 's' for "seconds".  */
  };

struct gmon_cg_arc_record
  {
    char from_pc[sizeof (char*)];	/* Address within caller's body.  */
    char self_pc[sizeof (char*)];	/* Address within callee's body.  */
    char count[4];			/* Number of arc traversals.  */
  };

#endif /* gmon_out_h */
