/* dwarf.h - DWARF support header file
   Copyright 2005, 2007, 2008, 2009, 2010
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

typedef unsigned HOST_WIDEST_INT dwarf_vma;
typedef unsigned HOST_WIDEST_INT dwarf_size_type;

struct dwarf_section
{
  /* A debug section has a different name when it's stored compressed
     or not.  COMPRESSED_NAME and UNCOMPRESSED_NAME are the two
     possibilities.  NAME is set to whichever one is used for this
     input file, as determined by load_debug_section().  */
  const char *uncompressed_name;
  const char *compressed_name;
  const char *name;
  unsigned char *start;
  dwarf_vma address;
  dwarf_size_type size;
};

/* A structure containing the name of a debug section
   and a pointer to a function that can decode it.  */
struct dwarf_section_display
{
  struct dwarf_section section;
  int (*display) (struct dwarf_section *, void *);
  int *enabled;
  unsigned int relocate : 1;
};

enum dwarf_section_display_enum
{
  abbrev = 0,
  aranges,
  frame,
  info,
  line,
  pubnames,
  eh_frame,
  macinfo,
  str,
  loc,
  pubtypes,
  ranges,
  static_func,
  static_vars,
  types,
  weaknames,
  trace_info,
  trace_abbrev,
  trace_aranges,
  max
};

extern struct dwarf_section_display debug_displays [];

/* This structure records the information that
   we extract from the.debug_info section.  */
typedef struct
{
  unsigned int   pointer_size;
  unsigned int   offset_size;
  int            dwarf_version;
  unsigned long  cu_offset;
  unsigned long	 base_address;
  /* This is an array of offsets to the location list table.  */
  unsigned long *loc_offsets;
  int		*have_frame_base;
  unsigned int   num_loc_offsets;
  unsigned int   max_loc_offsets;
  /* List of .debug_ranges offsets seen in this .debug_info.  */
  unsigned long *range_lists;
  unsigned int   num_range_lists;
  unsigned int   max_range_lists;
}
debug_info;

extern int eh_addr_size;

extern int do_debug_info;
extern int do_debug_abbrevs;
extern int do_debug_lines;
extern int do_debug_pubnames;
extern int do_debug_pubtypes;
extern int do_debug_aranges;
extern int do_debug_ranges;
extern int do_debug_frames;
extern int do_debug_frames_interp;
extern int do_debug_macinfo;
extern int do_debug_str;
extern int do_debug_loc;
extern int do_gdb_index;
extern int do_trace_info;
extern int do_trace_abbrevs;
extern int do_trace_aranges;
extern int do_wide;

extern void init_dwarf_regnames (unsigned int);
extern void init_dwarf_regnames_i386 (void);
extern void init_dwarf_regnames_x86_64 (void);

extern int load_debug_section (enum dwarf_section_display_enum,
			       void *);
extern void free_debug_section (enum dwarf_section_display_enum);

extern void free_debug_memory (void);

extern void dwarf_select_sections_by_names (const char *);
extern void dwarf_select_sections_by_letters (const char *);
extern void dwarf_select_sections_all (void);

void *cmalloc (size_t, size_t);
void *xcmalloc (size_t, size_t);
void *xcrealloc (void *, size_t, size_t);

unsigned long int read_leb128 (unsigned char *data,
			       unsigned int *length_return, int sign);
