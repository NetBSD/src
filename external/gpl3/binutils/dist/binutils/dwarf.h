/* dwarf.h - DWARF support header file
   Copyright 2005, 2007, 2008, 2009, 2010, 2011
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

typedef unsigned HOST_WIDEST_INT  dwarf_vma;
typedef HOST_WIDEST_INT           dwarf_signed_vma;
typedef unsigned HOST_WIDEST_INT  dwarf_size_type;

/* Structure found in the .debug_line section.  */
typedef struct
{
  unsigned char li_length          [4];
  unsigned char li_version         [2];
  unsigned char li_prologue_length [4];
  unsigned char li_min_insn_length [1];
  unsigned char li_default_is_stmt [1];
  unsigned char li_line_base       [1];
  unsigned char li_line_range      [1];
  unsigned char li_opcode_base     [1];
}
DWARF2_External_LineInfo;

typedef struct
{
  dwarf_vma	 li_length;
  unsigned short li_version;
  unsigned int   li_prologue_length;
  unsigned char  li_min_insn_length;
  unsigned char  li_max_ops_per_insn;
  unsigned char  li_default_is_stmt;
  int            li_line_base;
  unsigned char  li_line_range;
  unsigned char  li_opcode_base;
}
DWARF2_Internal_LineInfo;

/* Structure found in .debug_pubnames section.  */
typedef struct
{
  unsigned char pn_length  [4];
  unsigned char pn_version [2];
  unsigned char pn_offset  [4];
  unsigned char pn_size    [4];
}
DWARF2_External_PubNames;

typedef struct
{
  dwarf_vma	 pn_length;
  unsigned short pn_version;
  dwarf_vma	 pn_offset;
  dwarf_vma	 pn_size;
}
DWARF2_Internal_PubNames;

/* Structure found in .debug_info section.  */
typedef struct
{
  unsigned char  cu_length        [4];
  unsigned char  cu_version       [2];
  unsigned char  cu_abbrev_offset [4];
  unsigned char  cu_pointer_size  [1];
}
DWARF2_External_CompUnit;

typedef struct
{
  dwarf_vma	 cu_length;
  unsigned short cu_version;
  dwarf_vma	 cu_abbrev_offset;
  unsigned char  cu_pointer_size;
}
DWARF2_Internal_CompUnit;

typedef struct
{
  unsigned char  ar_length       [4];
  unsigned char  ar_version      [2];
  unsigned char  ar_info_offset  [4];
  unsigned char  ar_pointer_size [1];
  unsigned char  ar_segment_size [1];
}
DWARF2_External_ARange;

typedef struct
{
  dwarf_vma	 ar_length;
  unsigned short ar_version;
  dwarf_vma	 ar_info_offset;
  unsigned char  ar_pointer_size;
  unsigned char  ar_segment_size;
}
DWARF2_Internal_ARange;

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
  macro,
  str,
  loc,
  pubtypes,
  ranges,
  static_func,
  static_vars,
  types,
  weaknames,
  gdb_index,
  trace_info,
  trace_abbrev,
  trace_aranges,
  info_dwo,
  abbrev_dwo,
  types_dwo,
  line_dwo,
  loc_dwo,
  macro_dwo,
  macinfo_dwo,
  str_dwo,
  str_index,
  str_index_dwo,
  debug_addr,
  max
};

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
  enum dwarf_section_display_enum abbrev_sec;
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

extern struct dwarf_section_display debug_displays [];

/* This structure records the information that
   we extract from the.debug_info section.  */
typedef struct
{
  unsigned int   pointer_size;
  unsigned int   offset_size;
  int            dwarf_version;
  dwarf_vma	 cu_offset;
  dwarf_vma	 base_address;
  /* This field is filled in when reading the attribute DW_AT_GNU_addr_base and
     is used with the form DW_AT_GNU_FORM_addr_index.  */
  dwarf_vma	 addr_base;
  /* This field is filled in when reading the attribute DW_AT_GNU_ranges_base and
     is used when calculating ranges.  */
  dwarf_vma	 ranges_base;
  /* This is an array of offsets to the location list table.  */
  dwarf_vma *    loc_offsets;
  int *          have_frame_base;
  unsigned int   num_loc_offsets;
  unsigned int   max_loc_offsets;
  /* List of .debug_ranges offsets seen in this .debug_info.  */
  dwarf_vma *    range_lists;
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

extern int dwarf_cutoff_level;
extern unsigned long dwarf_start_die;

extern int dwarf_check;

extern void init_dwarf_regnames (unsigned int);
extern void init_dwarf_regnames_i386 (void);
extern void init_dwarf_regnames_x86_64 (void);

extern int load_debug_section (enum dwarf_section_display_enum, void *);
extern void free_debug_section (enum dwarf_section_display_enum);

extern void free_debug_memory (void);

extern void dwarf_select_sections_by_names (const char *);
extern void dwarf_select_sections_by_letters (const char *);
extern void dwarf_select_sections_all (void);

void * cmalloc (size_t, size_t);
void * xcmalloc (size_t, size_t);
void * xcrealloc (void *, size_t, size_t);

dwarf_vma read_leb128 (unsigned char *, unsigned int *, int);
