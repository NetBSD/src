// OBSOLETE /* Read apollo DST symbol tables and convert to internal format, for GDB.
// OBSOLETE    Contributed by Troy Rollo, University of NSW (troy@cbme.unsw.edu.au).
// OBSOLETE    Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "gdbtypes.h"
// OBSOLETE #include "breakpoint.h"
// OBSOLETE #include "bfd.h"
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "objfiles.h"
// OBSOLETE #include "buildsym.h"
// OBSOLETE #include "gdb_obstack.h"
// OBSOLETE 
// OBSOLETE #include "gdb_string.h"
// OBSOLETE 
// OBSOLETE #include "dst.h"
// OBSOLETE 
// OBSOLETE CORE_ADDR cur_src_start_addr, cur_src_end_addr;
// OBSOLETE dst_sec blocks_info, lines_info, symbols_info;
// OBSOLETE 
// OBSOLETE /* Vector of line number information.  */
// OBSOLETE 
// OBSOLETE static struct linetable *line_vector;
// OBSOLETE 
// OBSOLETE /* Index of next entry to go in line_vector_index.  */
// OBSOLETE 
// OBSOLETE static int line_vector_index;
// OBSOLETE 
// OBSOLETE /* Last line number recorded in the line vector.  */
// OBSOLETE 
// OBSOLETE static int prev_line_number;
// OBSOLETE 
// OBSOLETE /* Number of elements allocated for line_vector currently.  */
// OBSOLETE 
// OBSOLETE static int line_vector_length;
// OBSOLETE 
// OBSOLETE static int init_dst_sections (int);
// OBSOLETE 
// OBSOLETE static void read_dst_symtab (struct objfile *);
// OBSOLETE 
// OBSOLETE static void find_dst_sections (bfd *, sec_ptr, PTR);
// OBSOLETE 
// OBSOLETE static void dst_symfile_init (struct objfile *);
// OBSOLETE 
// OBSOLETE static void dst_new_init (struct objfile *);
// OBSOLETE 
// OBSOLETE static void dst_symfile_read (struct objfile *, int);
// OBSOLETE 
// OBSOLETE static void dst_symfile_finish (struct objfile *);
// OBSOLETE 
// OBSOLETE static void dst_end_symtab (struct objfile *);
// OBSOLETE 
// OBSOLETE static void complete_symtab (char *, CORE_ADDR, unsigned int);
// OBSOLETE 
// OBSOLETE static void dst_start_symtab (void);
// OBSOLETE 
// OBSOLETE static void dst_record_line (int, CORE_ADDR);
// OBSOLETE 
// OBSOLETE /* Manage the vector of line numbers.  */
// OBSOLETE /* FIXME: Use record_line instead.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE dst_record_line (int line, CORE_ADDR pc)
// OBSOLETE {
// OBSOLETE   struct linetable_entry *e;
// OBSOLETE   /* Make sure line vector is big enough.  */
// OBSOLETE 
// OBSOLETE   if (line_vector_index + 2 >= line_vector_length)
// OBSOLETE     {
// OBSOLETE       line_vector_length *= 2;
// OBSOLETE       line_vector = (struct linetable *)
// OBSOLETE 	xrealloc ((char *) line_vector, sizeof (struct linetable)
// OBSOLETE 		  + (line_vector_length
// OBSOLETE 		     * sizeof (struct linetable_entry)));
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   e = line_vector->item + line_vector_index++;
// OBSOLETE   e->line = line;
// OBSOLETE   e->pc = pc;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Start a new symtab for a new source file.
// OBSOLETE    It indicates the start of data for one original source file.  */
// OBSOLETE /* FIXME: use start_symtab, like coffread.c now does.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE dst_start_symtab (void)
// OBSOLETE {
// OBSOLETE   /* Initialize the source file line number information for this file.  */
// OBSOLETE 
// OBSOLETE   if (line_vector)		/* Unlikely, but maybe possible? */
// OBSOLETE     xfree (line_vector);
// OBSOLETE   line_vector_index = 0;
// OBSOLETE   line_vector_length = 1000;
// OBSOLETE   prev_line_number = -2;	/* Force first line number to be explicit */
// OBSOLETE   line_vector = (struct linetable *)
// OBSOLETE     xmalloc (sizeof (struct linetable)
// OBSOLETE 	     + line_vector_length * sizeof (struct linetable_entry));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Save the vital information from when starting to read a file,
// OBSOLETE    for use when closing off the current file.
// OBSOLETE    NAME is the file name the symbols came from, START_ADDR is the first
// OBSOLETE    text address for the file, and SIZE is the number of bytes of text.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE complete_symtab (char *name, CORE_ADDR start_addr, unsigned int size)
// OBSOLETE {
// OBSOLETE   last_source_file = savestring (name, strlen (name));
// OBSOLETE   cur_src_start_addr = start_addr;
// OBSOLETE   cur_src_end_addr = start_addr + size;
// OBSOLETE 
// OBSOLETE   if (current_objfile->ei.entry_point >= cur_src_start_addr &&
// OBSOLETE       current_objfile->ei.entry_point < cur_src_end_addr)
// OBSOLETE     {
// OBSOLETE       current_objfile->ei.entry_file_lowpc = cur_src_start_addr;
// OBSOLETE       current_objfile->ei.entry_file_highpc = cur_src_end_addr;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Finish the symbol definitions for one main source file,
// OBSOLETE    close off all the lexical contexts for that file
// OBSOLETE    (creating struct block's for them), then make the
// OBSOLETE    struct symtab for that file and put it in the list of all such. */
// OBSOLETE /* FIXME: Use end_symtab, like coffread.c now does.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE dst_end_symtab (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   register struct symtab *symtab;
// OBSOLETE   register struct blockvector *blockvector;
// OBSOLETE   register struct linetable *lv;
// OBSOLETE 
// OBSOLETE   /* Create the blockvector that points to all the file's blocks.  */
// OBSOLETE 
// OBSOLETE   blockvector = make_blockvector (objfile);
// OBSOLETE 
// OBSOLETE   /* Now create the symtab object for this source file.  */
// OBSOLETE   symtab = allocate_symtab (last_source_file, objfile);
// OBSOLETE 
// OBSOLETE   /* Fill in its components.  */
// OBSOLETE   symtab->blockvector = blockvector;
// OBSOLETE   symtab->free_code = free_linetable;
// OBSOLETE   symtab->free_ptr = 0;
// OBSOLETE   symtab->filename = last_source_file;
// OBSOLETE   symtab->dirname = NULL;
// OBSOLETE   symtab->debugformat = obsavestring ("Apollo DST", 10,
// OBSOLETE 				      &objfile->symbol_obstack);
// OBSOLETE   lv = line_vector;
// OBSOLETE   lv->nitems = line_vector_index;
// OBSOLETE   symtab->linetable = (struct linetable *)
// OBSOLETE     xrealloc ((char *) lv, (sizeof (struct linetable)
// OBSOLETE 			    + lv->nitems * sizeof (struct linetable_entry)));
// OBSOLETE 
// OBSOLETE   free_named_symtabs (symtab->filename);
// OBSOLETE 
// OBSOLETE   /* Reinitialize for beginning of new file. */
// OBSOLETE   line_vector = 0;
// OBSOLETE   line_vector_length = -1;
// OBSOLETE   last_source_file = NULL;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dst_symfile_init ()
// OBSOLETE    is the dst-specific initialization routine for reading symbols.
// OBSOLETE 
// OBSOLETE    We will only be called if this is a DST or DST-like file.
// OBSOLETE    BFD handles figuring out the format of the file, and code in symtab.c
// OBSOLETE    uses BFD's determination to vector to us.
// OBSOLETE 
// OBSOLETE    The ultimate result is a new symtab (or, FIXME, eventually a psymtab).  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE dst_symfile_init (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   asection *section;
// OBSOLETE   bfd *abfd = objfile->obfd;
// OBSOLETE 
// OBSOLETE   init_entry_point_info (objfile);
// OBSOLETE 
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* This function is called for every section; it finds the outer limits
// OBSOLETE    of the line table (minimum and maximum file offset) so that the
// OBSOLETE    mainline code can read the whole thing for efficiency.  */
// OBSOLETE 
// OBSOLETE /* ARGSUSED */
// OBSOLETE static void
// OBSOLETE find_dst_sections (bfd *abfd, sec_ptr asect, PTR vpinfo)
// OBSOLETE {
// OBSOLETE   int size, count;
// OBSOLETE   long base;
// OBSOLETE   file_ptr offset, maxoff;
// OBSOLETE   dst_sec *section;
// OBSOLETE 
// OBSOLETE /* WARNING WILL ROBINSON!  ACCESSING BFD-PRIVATE DATA HERE!  FIXME!  */
// OBSOLETE   size = asect->_raw_size;
// OBSOLETE   offset = asect->filepos;
// OBSOLETE   base = asect->vma;
// OBSOLETE /* End of warning */
// OBSOLETE 
// OBSOLETE   section = NULL;
// OBSOLETE   if (!strcmp (asect->name, ".blocks"))
// OBSOLETE     section = &blocks_info;
// OBSOLETE   else if (!strcmp (asect->name, ".lines"))
// OBSOLETE     section = &lines_info;
// OBSOLETE   else if (!strcmp (asect->name, ".symbols"))
// OBSOLETE     section = &symbols_info;
// OBSOLETE   if (!section)
// OBSOLETE     return;
// OBSOLETE   section->size = size;
// OBSOLETE   section->position = offset;
// OBSOLETE   section->base = base;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* The BFD for this file -- only good while we're actively reading
// OBSOLETE    symbols into a psymtab or a symtab.  */
// OBSOLETE 
// OBSOLETE static bfd *symfile_bfd;
// OBSOLETE 
// OBSOLETE /* Read a symbol file, after initialization by dst_symfile_init.  */
// OBSOLETE /* FIXME!  Addr and Mainline are not used yet -- this will not work for
// OBSOLETE    shared libraries or add_file!  */
// OBSOLETE 
// OBSOLETE /* ARGSUSED */
// OBSOLETE static void
// OBSOLETE dst_symfile_read (struct objfile *objfile, int mainline)
// OBSOLETE {
// OBSOLETE   bfd *abfd = objfile->obfd;
// OBSOLETE   char *name = bfd_get_filename (abfd);
// OBSOLETE   int desc;
// OBSOLETE   register int val;
// OBSOLETE   int num_symbols;
// OBSOLETE   int symtab_offset;
// OBSOLETE   int stringtab_offset;
// OBSOLETE 
// OBSOLETE   symfile_bfd = abfd;		/* Kludge for swap routines */
// OBSOLETE 
// OBSOLETE /* WARNING WILL ROBINSON!  ACCESSING BFD-PRIVATE DATA HERE!  FIXME!  */
// OBSOLETE   desc = fileno ((FILE *) (abfd->iostream));	/* File descriptor */
// OBSOLETE 
// OBSOLETE   /* Read the line number table, all at once.  */
// OBSOLETE   bfd_map_over_sections (abfd, find_dst_sections, (PTR) NULL);
// OBSOLETE 
// OBSOLETE   val = init_dst_sections (desc);
// OBSOLETE   if (val < 0)
// OBSOLETE     error ("\"%s\": error reading debugging symbol tables\n", name);
// OBSOLETE 
// OBSOLETE   init_minimal_symbol_collection ();
// OBSOLETE   make_cleanup_discard_minimal_symbols ();
// OBSOLETE 
// OBSOLETE   /* Now that the executable file is positioned at symbol table,
// OBSOLETE      process it and define symbols accordingly.  */
// OBSOLETE 
// OBSOLETE   read_dst_symtab (objfile);
// OBSOLETE 
// OBSOLETE   /* Sort symbols alphabetically within each block.  */
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     struct symtab *s;
// OBSOLETE     for (s = objfile->symtabs; s != NULL; s = s->next)
// OBSOLETE       {
// OBSOLETE 	sort_symtab_syms (s);
// OBSOLETE       }
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   /* Install any minimal symbols that have been collected as the current
// OBSOLETE      minimal symbols for this objfile. */
// OBSOLETE 
// OBSOLETE   install_minimal_symbols (objfile);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE dst_new_init (struct objfile *ignore)
// OBSOLETE {
// OBSOLETE   /* Nothin' to do */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Perform any local cleanups required when we are done with a particular
// OBSOLETE    objfile.  I.E, we are in the process of discarding all symbol information
// OBSOLETE    for an objfile, freeing up all memory held for it, and unlinking the
// OBSOLETE    objfile struct from the global list of known objfiles. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE dst_symfile_finish (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   /* Nothing to do */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Get the next line number from the DST. Returns 0 when we hit an
// OBSOLETE  * end directive or cannot continue for any other reason.
// OBSOLETE  *
// OBSOLETE  * Note that ordinary pc deltas are multiplied by two. Apparently
// OBSOLETE  * this is what was really intended.
// OBSOLETE  */
// OBSOLETE static int
// OBSOLETE get_dst_line (signed char **buffer, long *pc)
// OBSOLETE {
// OBSOLETE   static last_pc = 0;
// OBSOLETE   static long last_line = 0;
// OBSOLETE   static int last_file = 0;
// OBSOLETE   dst_ln_entry_ptr_t entry;
// OBSOLETE   int size;
// OBSOLETE   dst_src_loc_t *src_loc;
// OBSOLETE 
// OBSOLETE   if (*pc != -1)
// OBSOLETE     {
// OBSOLETE       last_pc = *pc;
// OBSOLETE       *pc = -1;
// OBSOLETE     }
// OBSOLETE   entry = (dst_ln_entry_ptr_t) * buffer;
// OBSOLETE 
// OBSOLETE   while (dst_ln_ln_delta (*entry) == dst_ln_escape_flag)
// OBSOLETE     {
// OBSOLETE       switch (entry->esc.esc_code)
// OBSOLETE 	{
// OBSOLETE 	case dst_ln_pad:
// OBSOLETE 	  size = 1;		/* pad byte */
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_file:
// OBSOLETE 	  /* file escape.  Next 4 bytes are a dst_src_loc_t */
// OBSOLETE 	  size = 5;
// OBSOLETE 	  src_loc = (dst_src_loc_t *) (*buffer + 1);
// OBSOLETE 	  last_line = src_loc->line_number;
// OBSOLETE 	  last_file = src_loc->file_index;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_dln1_dpc1:
// OBSOLETE 	  /* 1 byte line delta, 1 byte pc delta */
// OBSOLETE 	  last_line += (*buffer)[1];
// OBSOLETE 	  last_pc += 2 * (unsigned char) (*buffer)[2];
// OBSOLETE 	  dst_record_line (last_line, last_pc);
// OBSOLETE 	  size = 3;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_dln2_dpc2:
// OBSOLETE 	  /* 2 bytes line delta, 2 bytes pc delta */
// OBSOLETE 	  last_line += *(short *) (*buffer + 1);
// OBSOLETE 	  last_pc += 2 * (*(short *) (*buffer + 3));
// OBSOLETE 	  size = 5;
// OBSOLETE 	  dst_record_line (last_line, last_pc);
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_ln4_pc4:
// OBSOLETE 	  /* 4 bytes ABSOLUTE line number, 4 bytes ABSOLUTE pc */
// OBSOLETE 	  last_line = *(unsigned long *) (*buffer + 1);
// OBSOLETE 	  last_pc = *(unsigned long *) (*buffer + 5);
// OBSOLETE 	  size = 9;
// OBSOLETE 	  dst_record_line (last_line, last_pc);
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_dln1_dpc0:
// OBSOLETE 	  /* 1 byte line delta, pc delta = 0 */
// OBSOLETE 	  size = 2;
// OBSOLETE 	  last_line += (*buffer)[1];
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_ln_off_1:
// OBSOLETE 	  /* statement escape, stmt # = 1 (2nd stmt on line) */
// OBSOLETE 	  size = 1;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_ln_off:
// OBSOLETE 	  /* statement escape, stmt # = next byte */
// OBSOLETE 	  size = 2;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_entry:
// OBSOLETE 	  /* entry escape, next byte is entry number */
// OBSOLETE 	  size = 2;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_exit:
// OBSOLETE 	  /* exit escape */
// OBSOLETE 	  size = 1;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_stmt_end:
// OBSOLETE 	  /* gap escape, 4 bytes pc delta */
// OBSOLETE 	  size = 5;
// OBSOLETE 	  /* last_pc += 2 * (*(long *) (*buffer + 1)); */
// OBSOLETE 	  /* Apparently this isn't supposed to actually modify
// OBSOLETE 	   * the pc value. Totally weird.
// OBSOLETE 	   */
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_escape_11:
// OBSOLETE 	case dst_ln_escape_12:
// OBSOLETE 	case dst_ln_escape_13:
// OBSOLETE 	  size = 1;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_nxt_byte:
// OBSOLETE 	  /* This shouldn't happen. If it does, we're SOL */
// OBSOLETE 	  return 0;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ln_end:
// OBSOLETE 	  /* end escape, final entry follows */
// OBSOLETE 	  return 0;
// OBSOLETE 	}
// OBSOLETE       *buffer += (size < 0) ? -size : size;
// OBSOLETE       entry = (dst_ln_entry_ptr_t) * buffer;
// OBSOLETE     }
// OBSOLETE   last_line += dst_ln_ln_delta (*entry);
// OBSOLETE   last_pc += entry->delta.pc_delta * 2;
// OBSOLETE   (*buffer)++;
// OBSOLETE   dst_record_line (last_line, last_pc);
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE enter_all_lines (char *buffer, long address)
// OBSOLETE {
// OBSOLETE   if (buffer)
// OBSOLETE     while (get_dst_line (&buffer, &address));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE get_dst_entry (char *buffer, dst_rec_ptr_t *ret_entry)
// OBSOLETE {
// OBSOLETE   int size;
// OBSOLETE   dst_rec_ptr_t entry;
// OBSOLETE   static int last_type;
// OBSOLETE   int ar_size;
// OBSOLETE   static unsigned lu3;
// OBSOLETE 
// OBSOLETE   entry = (dst_rec_ptr_t) buffer;
// OBSOLETE   switch (entry->rec_type)
// OBSOLETE     {
// OBSOLETE     case dst_typ_pad:
// OBSOLETE       size = 0;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_comp_unit:
// OBSOLETE       size = sizeof (DST_comp_unit (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_section_tab:
// OBSOLETE       size = sizeof (DST_section_tab (entry))
// OBSOLETE 	+ ((int) DST_section_tab (entry).number_of_sections
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (long);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_file_tab:
// OBSOLETE       size = sizeof (DST_file_tab (entry))
// OBSOLETE 	+ ((int) DST_file_tab (entry).number_of_files
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_file_desc_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_block:
// OBSOLETE       size = sizeof (DST_block (entry))
// OBSOLETE 	+ ((int) DST_block (entry).n_of_code_ranges
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_code_range_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_5:
// OBSOLETE       size = -1;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_var:
// OBSOLETE       size = sizeof (DST_var (entry)) -
// OBSOLETE 	sizeof (dst_var_loc_long_t) * dst_dummy_array_size +
// OBSOLETE 	DST_var (entry).no_of_locs *
// OBSOLETE 	(DST_var (entry).short_locs ?
// OBSOLETE 	 sizeof (dst_var_loc_short_t) :
// OBSOLETE 	 sizeof (dst_var_loc_long_t));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_pointer:
// OBSOLETE       size = sizeof (DST_pointer (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_array:
// OBSOLETE       size = sizeof (DST_array (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_subrange:
// OBSOLETE       size = sizeof (DST_subrange (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_set:
// OBSOLETE       size = sizeof (DST_set (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_implicit_enum:
// OBSOLETE       size = sizeof (DST_implicit_enum (entry))
// OBSOLETE 	+ ((int) DST_implicit_enum (entry).nelems
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_rel_offset_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_explicit_enum:
// OBSOLETE       size = sizeof (DST_explicit_enum (entry))
// OBSOLETE 	+ ((int) DST_explicit_enum (entry).nelems
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_enum_elem_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_short_rec:
// OBSOLETE       size = sizeof (DST_short_rec (entry))
// OBSOLETE 	+ DST_short_rec (entry).nfields * sizeof (dst_short_field_t)
// OBSOLETE 	- dst_dummy_array_size * sizeof (dst_field_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_short_union:
// OBSOLETE       size = sizeof (DST_short_union (entry))
// OBSOLETE 	+ DST_short_union (entry).nfields * sizeof (dst_short_field_t)
// OBSOLETE 	- dst_dummy_array_size * sizeof (dst_field_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_file:
// OBSOLETE       size = sizeof (DST_file (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_offset:
// OBSOLETE       size = sizeof (DST_offset (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_alias:
// OBSOLETE       size = sizeof (DST_alias (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_signature:
// OBSOLETE       size = sizeof (DST_signature (entry)) +
// OBSOLETE 	((int) DST_signature (entry).nargs -
// OBSOLETE 	 dst_dummy_array_size) * sizeof (dst_arg_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_21:
// OBSOLETE       size = -1;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_old_label:
// OBSOLETE       size = sizeof (DST_old_label (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_scope:
// OBSOLETE       size = sizeof (DST_scope (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_end_scope:
// OBSOLETE       size = 0;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_25:
// OBSOLETE     case dst_typ_26:
// OBSOLETE       size = -1;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_string_tab:
// OBSOLETE     case dst_typ_global_name_tab:
// OBSOLETE       size = sizeof (DST_string_tab (entry))
// OBSOLETE 	+ DST_string_tab (entry).length
// OBSOLETE 	- dst_dummy_array_size;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_forward:
// OBSOLETE       size = sizeof (DST_forward (entry));
// OBSOLETE       get_dst_entry ((char *) entry + DST_forward (entry).rec_off, &entry);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_size:
// OBSOLETE       size = sizeof (DST_aux_size (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_align:
// OBSOLETE       size = sizeof (DST_aux_align (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_field_size:
// OBSOLETE       size = sizeof (DST_aux_field_size (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_field_off:
// OBSOLETE       size = sizeof (DST_aux_field_off (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_field_align:
// OBSOLETE       size = sizeof (DST_aux_field_align (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_qual:
// OBSOLETE       size = sizeof (DST_aux_qual (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_var_bound:
// OBSOLETE       size = sizeof (DST_aux_var_bound (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_extension:
// OBSOLETE       size = DST_extension (entry).rec_size;
// OBSOLETE       break;
// OBSOLETE     case dst_typ_string:
// OBSOLETE       size = sizeof (DST_string (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_old_entry:
// OBSOLETE       size = 48;		/* Obsolete entry type */
// OBSOLETE       break;
// OBSOLETE     case dst_typ_const:
// OBSOLETE       size = sizeof (DST_const (entry))
// OBSOLETE 	+ DST_const (entry).value.length
// OBSOLETE 	- sizeof (DST_const (entry).value.val);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_reference:
// OBSOLETE       size = sizeof (DST_reference (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_old_record:
// OBSOLETE     case dst_typ_old_union:
// OBSOLETE     case dst_typ_record:
// OBSOLETE     case dst_typ_union:
// OBSOLETE       size = sizeof (DST_record (entry))
// OBSOLETE 	+ ((int) DST_record (entry).nfields
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_field_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_type_deriv:
// OBSOLETE       size = sizeof (DST_aux_type_deriv (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_locpool:
// OBSOLETE       size = sizeof (DST_locpool (entry))
// OBSOLETE 	+ ((int) DST_locpool (entry).length -
// OBSOLETE 	   dst_dummy_array_size);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_variable:
// OBSOLETE       size = sizeof (DST_variable (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_label:
// OBSOLETE       size = sizeof (DST_label (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_entry:
// OBSOLETE       size = sizeof (DST_entry (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_lifetime:
// OBSOLETE       size = sizeof (DST_aux_lifetime (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_ptr_base:
// OBSOLETE       size = sizeof (DST_aux_ptr_base (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_src_range:
// OBSOLETE       size = sizeof (DST_aux_src_range (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_reg_val:
// OBSOLETE       size = sizeof (DST_aux_reg_val (entry));
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_unit_names:
// OBSOLETE       size = sizeof (DST_aux_unit_names (entry))
// OBSOLETE 	+ ((int) DST_aux_unit_names (entry).number_of_names
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_rel_offset_t);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_aux_sect_info:
// OBSOLETE       size = sizeof (DST_aux_sect_info (entry))
// OBSOLETE 	+ ((int) DST_aux_sect_info (entry).number_of_refs
// OBSOLETE 	   - dst_dummy_array_size) * sizeof (dst_sect_ref_t);
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       size = -1;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   if (size == -1)
// OBSOLETE     {
// OBSOLETE       fprintf_unfiltered (gdb_stderr, "Warning: unexpected DST entry type (%d) found\nLast valid entry was of type: %d\n",
// OBSOLETE 			  (int) entry->rec_type,
// OBSOLETE 			  last_type);
// OBSOLETE       fprintf_unfiltered (gdb_stderr, "Last unknown_3 value: %d\n", lu3);
// OBSOLETE       size = 0;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     last_type = entry->rec_type;
// OBSOLETE   if (size & 1)			/* Align on a word boundary */
// OBSOLETE     size++;
// OBSOLETE   size += 2;
// OBSOLETE   *ret_entry = entry;
// OBSOLETE   return size;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE next_dst_entry (char **buffer, dst_rec_ptr_t *entry, dst_sec *table)
// OBSOLETE {
// OBSOLETE   if (*buffer - table->buffer >= table->size)
// OBSOLETE     {
// OBSOLETE       *entry = NULL;
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE   *buffer += get_dst_entry (*buffer, entry);
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #define NEXT_BLK(a, b) next_dst_entry(a, b, &blocks_info)
// OBSOLETE #define NEXT_SYM(a, b) next_dst_entry(a, b, &symbols_info)
// OBSOLETE #define	DST_OFFSET(a, b) ((char *) (a) + (b))
// OBSOLETE 
// OBSOLETE static dst_rec_ptr_t section_table = NULL;
// OBSOLETE 
// OBSOLETE char *
// OBSOLETE get_sec_ref (dst_sect_ref_t *ref)
// OBSOLETE {
// OBSOLETE   dst_sec *section = NULL;
// OBSOLETE   long offset;
// OBSOLETE 
// OBSOLETE   if (!section_table || !ref->sect_index)
// OBSOLETE     return NULL;
// OBSOLETE   offset = DST_section_tab (section_table).section_base[ref->sect_index - 1]
// OBSOLETE     + ref->sect_offset;
// OBSOLETE   if (offset >= blocks_info.base &&
// OBSOLETE       offset < blocks_info.base + blocks_info.size)
// OBSOLETE     section = &blocks_info;
// OBSOLETE   else if (offset >= symbols_info.base &&
// OBSOLETE 	   offset < symbols_info.base + symbols_info.size)
// OBSOLETE     section = &symbols_info;
// OBSOLETE   else if (offset >= lines_info.base &&
// OBSOLETE 	   offset < lines_info.base + lines_info.size)
// OBSOLETE     section = &lines_info;
// OBSOLETE   if (!section)
// OBSOLETE     return NULL;
// OBSOLETE   return section->buffer + (offset - section->base);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE dst_get_addr (int section, long offset)
// OBSOLETE {
// OBSOLETE   if (!section_table || !section)
// OBSOLETE     return 0;
// OBSOLETE   return DST_section_tab (section_table).section_base[section - 1] + offset;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE dst_sym_addr (dst_sect_ref_t *ref)
// OBSOLETE {
// OBSOLETE   if (!section_table || !ref->sect_index)
// OBSOLETE     return 0;
// OBSOLETE   return DST_section_tab (section_table).section_base[ref->sect_index - 1]
// OBSOLETE     + ref->sect_offset;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct symbol *
// OBSOLETE create_new_symbol (struct objfile *objfile, char *name)
// OBSOLETE {
// OBSOLETE   struct symbol *sym = (struct symbol *)
// OBSOLETE   obstack_alloc (&objfile->symbol_obstack, sizeof (struct symbol));
// OBSOLETE   memset (sym, 0, sizeof (struct symbol));
// OBSOLETE   SYMBOL_NAME (sym) = obsavestring (name, strlen (name),
// OBSOLETE 				    &objfile->symbol_obstack);
// OBSOLETE   SYMBOL_VALUE (sym) = 0;
// OBSOLETE   SYMBOL_NAMESPACE (sym) = VAR_NAMESPACE;
// OBSOLETE 
// OBSOLETE   SYMBOL_CLASS (sym) = LOC_BLOCK;
// OBSOLETE   return sym;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static struct type *decode_dst_type (struct objfile *, dst_rec_ptr_t);
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE decode_type_desc (struct objfile *objfile, dst_type_t *type_desc,
// OBSOLETE 		  dst_rec_ptr_t base)
// OBSOLETE {
// OBSOLETE   struct type *type;
// OBSOLETE   dst_rec_ptr_t entry;
// OBSOLETE   if (type_desc->std_type.user_defined_type)
// OBSOLETE     {
// OBSOLETE       entry = (dst_rec_ptr_t) DST_OFFSET (base,
// OBSOLETE 					  dst_user_type_offset (*type_desc));
// OBSOLETE       type = decode_dst_type (objfile, entry);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       switch (type_desc->std_type.dtc)
// OBSOLETE 	{
// OBSOLETE 	case dst_int8_type:
// OBSOLETE 	  type = builtin_type_signed_char;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_int16_type:
// OBSOLETE 	  type = builtin_type_short;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_int32_type:
// OBSOLETE 	  type = builtin_type_long;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_uint8_type:
// OBSOLETE 	  type = builtin_type_unsigned_char;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_uint16_type:
// OBSOLETE 	  type = builtin_type_unsigned_short;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_uint32_type:
// OBSOLETE 	  type = builtin_type_unsigned_long;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_real32_type:
// OBSOLETE 	  type = builtin_type_float;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_real64_type:
// OBSOLETE 	  type = builtin_type_double;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_complex_type:
// OBSOLETE 	  type = builtin_type_complex;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_dcomplex_type:
// OBSOLETE 	  type = builtin_type_double_complex;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_bool8_type:
// OBSOLETE 	  type = builtin_type_char;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_bool16_type:
// OBSOLETE 	  type = builtin_type_short;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_bool32_type:
// OBSOLETE 	  type = builtin_type_long;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_char_type:
// OBSOLETE 	  type = builtin_type_char;
// OBSOLETE 	  break;
// OBSOLETE 	  /* The next few are more complex. I will take care
// OBSOLETE 	   * of them properly at a later point.
// OBSOLETE 	   */
// OBSOLETE 	case dst_string_type:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_ptr_type:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_set_type:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_proc_type:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_func_type:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	  /* Back tto some ordinary ones */
// OBSOLETE 	case dst_void_type:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_uchar_type:
// OBSOLETE 	  type = builtin_type_unsigned_char;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  type = builtin_type_void;
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   return type;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE struct structure_list
// OBSOLETE {
// OBSOLETE   struct structure_list *next;
// OBSOLETE   struct type *type;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static struct structure_list *struct_list = NULL;
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE find_dst_structure (char *name)
// OBSOLETE {
// OBSOLETE   struct structure_list *element;
// OBSOLETE 
// OBSOLETE   for (element = struct_list; element; element = element->next)
// OBSOLETE     if (!strcmp (name, TYPE_NAME (element->type)))
// OBSOLETE       return element->type;
// OBSOLETE   return NULL;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE decode_dst_structure (struct objfile *objfile, dst_rec_ptr_t entry, int code,
// OBSOLETE 		      int version)
// OBSOLETE {
// OBSOLETE   struct type *type, *child_type;
// OBSOLETE   char *struct_name;
// OBSOLETE   char *name, *field_name;
// OBSOLETE   int i;
// OBSOLETE   int fieldoffset, fieldsize;
// OBSOLETE   dst_type_t type_desc;
// OBSOLETE   struct structure_list *element;
// OBSOLETE 
// OBSOLETE   struct_name = DST_OFFSET (entry, DST_record (entry).noffset);
// OBSOLETE   name = concat ((code == TYPE_CODE_UNION) ? "union " : "struct ",
// OBSOLETE 		 struct_name, NULL);
// OBSOLETE   type = find_dst_structure (name);
// OBSOLETE   if (type)
// OBSOLETE     {
// OBSOLETE       xfree (name);
// OBSOLETE       return type;
// OBSOLETE     }
// OBSOLETE   type = alloc_type (objfile);
// OBSOLETE   TYPE_NAME (type) = obstack_copy0 (&objfile->symbol_obstack,
// OBSOLETE 				    name, strlen (name));
// OBSOLETE   xfree (name);
// OBSOLETE   TYPE_CODE (type) = code;
// OBSOLETE   TYPE_LENGTH (type) = DST_record (entry).size;
// OBSOLETE   TYPE_NFIELDS (type) = DST_record (entry).nfields;
// OBSOLETE   TYPE_FIELDS (type) = (struct field *)
// OBSOLETE     obstack_alloc (&objfile->symbol_obstack, sizeof (struct field) *
// OBSOLETE 		   DST_record (entry).nfields);
// OBSOLETE   fieldoffset = fieldsize = 0;
// OBSOLETE   INIT_CPLUS_SPECIFIC (type);
// OBSOLETE   element = (struct structure_list *)
// OBSOLETE     xmalloc (sizeof (struct structure_list));
// OBSOLETE   element->type = type;
// OBSOLETE   element->next = struct_list;
// OBSOLETE   struct_list = element;
// OBSOLETE   for (i = 0; i < DST_record (entry).nfields; i++)
// OBSOLETE     {
// OBSOLETE       switch (version)
// OBSOLETE 	{
// OBSOLETE 	case 2:
// OBSOLETE 	  field_name = DST_OFFSET (entry,
// OBSOLETE 				   DST_record (entry).f.ofields[i].noffset);
// OBSOLETE 	  fieldoffset = DST_record (entry).f.ofields[i].foffset * 8 +
// OBSOLETE 	    DST_record (entry).f.ofields[i].bit_offset;
// OBSOLETE 	  fieldsize = DST_record (entry).f.ofields[i].size;
// OBSOLETE 	  type_desc = DST_record (entry).f.ofields[i].type_desc;
// OBSOLETE 	  break;
// OBSOLETE 	case 1:
// OBSOLETE 	  field_name = DST_OFFSET (entry,
// OBSOLETE 				   DST_record (entry).f.fields[i].noffset);
// OBSOLETE 	  type_desc = DST_record (entry).f.fields[i].type_desc;
// OBSOLETE 	  switch (DST_record (entry).f.fields[i].f.field_loc.format_tag)
// OBSOLETE 	    {
// OBSOLETE 	    case dst_field_byte:
// OBSOLETE 	      fieldoffset = DST_record (entry).f.
// OBSOLETE 		fields[i].f.field_byte.offset * 8;
// OBSOLETE 	      fieldsize = -1;
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_field_bit:
// OBSOLETE 	      fieldoffset = DST_record (entry).f.
// OBSOLETE 		fields[i].f.field_bit.byte_offset * 8 +
// OBSOLETE 		DST_record (entry).f.
// OBSOLETE 		fields[i].f.field_bit.bit_offset;
// OBSOLETE 	      fieldsize = DST_record (entry).f.
// OBSOLETE 		fields[i].f.field_bit.nbits;
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_field_loc:
// OBSOLETE 	      fieldoffset += fieldsize;
// OBSOLETE 	      fieldsize = -1;
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	  break;
// OBSOLETE 	case 0:
// OBSOLETE 	  field_name = DST_OFFSET (entry,
// OBSOLETE 				   DST_record (entry).f.sfields[i].noffset);
// OBSOLETE 	  fieldoffset = DST_record (entry).f.sfields[i].foffset;
// OBSOLETE 	  type_desc = DST_record (entry).f.sfields[i].type_desc;
// OBSOLETE 	  if (i < DST_record (entry).nfields - 1)
// OBSOLETE 	    fieldsize = DST_record (entry).f.sfields[i + 1].foffset;
// OBSOLETE 	  else
// OBSOLETE 	    fieldsize = DST_record (entry).size;
// OBSOLETE 	  fieldsize -= fieldoffset;
// OBSOLETE 	  fieldoffset *= 8;
// OBSOLETE 	  fieldsize *= 8;
// OBSOLETE 	}
// OBSOLETE       TYPE_FIELDS (type)[i].name =
// OBSOLETE 	obstack_copy0 (&objfile->symbol_obstack,
// OBSOLETE 		       field_name, strlen (field_name));
// OBSOLETE       TYPE_FIELDS (type)[i].type = decode_type_desc (objfile,
// OBSOLETE 						     &type_desc,
// OBSOLETE 						     entry);
// OBSOLETE       if (fieldsize == -1)
// OBSOLETE 	fieldsize = TYPE_LENGTH (TYPE_FIELDS (type)[i].type) *
// OBSOLETE 	  8;
// OBSOLETE       TYPE_FIELDS (type)[i].bitsize = fieldsize;
// OBSOLETE       TYPE_FIELDS (type)[i].bitpos = fieldoffset;
// OBSOLETE     }
// OBSOLETE   return type;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE decode_dst_type (struct objfile *objfile, dst_rec_ptr_t entry)
// OBSOLETE {
// OBSOLETE   struct type *child_type, *type, *range_type, *index_type;
// OBSOLETE 
// OBSOLETE   switch (entry->rec_type)
// OBSOLETE     {
// OBSOLETE     case dst_typ_var:
// OBSOLETE       return decode_type_desc (objfile,
// OBSOLETE 			       &DST_var (entry).type_desc,
// OBSOLETE 			       entry);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_variable:
// OBSOLETE       return decode_type_desc (objfile,
// OBSOLETE 			       &DST_variable (entry).type_desc,
// OBSOLETE 			       entry);
// OBSOLETE       break;
// OBSOLETE     case dst_typ_short_rec:
// OBSOLETE       return decode_dst_structure (objfile, entry, TYPE_CODE_STRUCT, 0);
// OBSOLETE     case dst_typ_short_union:
// OBSOLETE       return decode_dst_structure (objfile, entry, TYPE_CODE_UNION, 0);
// OBSOLETE     case dst_typ_union:
// OBSOLETE       return decode_dst_structure (objfile, entry, TYPE_CODE_UNION, 1);
// OBSOLETE     case dst_typ_record:
// OBSOLETE       return decode_dst_structure (objfile, entry, TYPE_CODE_STRUCT, 1);
// OBSOLETE     case dst_typ_old_union:
// OBSOLETE       return decode_dst_structure (objfile, entry, TYPE_CODE_UNION, 2);
// OBSOLETE     case dst_typ_old_record:
// OBSOLETE       return decode_dst_structure (objfile, entry, TYPE_CODE_STRUCT, 2);
// OBSOLETE     case dst_typ_pointer:
// OBSOLETE       return make_pointer_type (
// OBSOLETE 				 decode_type_desc (objfile,
// OBSOLETE 					     &DST_pointer (entry).type_desc,
// OBSOLETE 						   entry),
// OBSOLETE 				 NULL);
// OBSOLETE     case dst_typ_array:
// OBSOLETE       child_type = decode_type_desc (objfile,
// OBSOLETE 				     &DST_pointer (entry).type_desc,
// OBSOLETE 				     entry);
// OBSOLETE       index_type = lookup_fundamental_type (objfile,
// OBSOLETE 					    FT_INTEGER);
// OBSOLETE       range_type = create_range_type ((struct type *) NULL,
// OBSOLETE 				      index_type, DST_array (entry).lo_bound,
// OBSOLETE 				      DST_array (entry).hi_bound);
// OBSOLETE       return create_array_type ((struct type *) NULL, child_type,
// OBSOLETE 				range_type);
// OBSOLETE     case dst_typ_alias:
// OBSOLETE       return decode_type_desc (objfile,
// OBSOLETE 			       &DST_alias (entry).type_desc,
// OBSOLETE 			       entry);
// OBSOLETE     default:
// OBSOLETE       return builtin_type_int;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE struct symbol_list
// OBSOLETE {
// OBSOLETE   struct symbol_list *next;
// OBSOLETE   struct symbol *symbol;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static struct symbol_list *dst_global_symbols = NULL;
// OBSOLETE static int total_globals = 0;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE decode_dst_locstring (char *locstr, struct symbol *sym)
// OBSOLETE {
// OBSOLETE   dst_loc_entry_t *entry, *next_entry;
// OBSOLETE   CORE_ADDR temp;
// OBSOLETE   int count = 0;
// OBSOLETE 
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       if (count++ == 100)
// OBSOLETE 	{
// OBSOLETE 	  fprintf_unfiltered (gdb_stderr, "Error reading locstring\n");
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       entry = (dst_loc_entry_t *) locstr;
// OBSOLETE       next_entry = (dst_loc_entry_t *) (locstr + 1);
// OBSOLETE       switch (entry->header.code)
// OBSOLETE 	{
// OBSOLETE 	case dst_lsc_end:	/* End of string */
// OBSOLETE 	  return;
// OBSOLETE 	case dst_lsc_indirect:	/* Indirect through previous. Arg == 6 */
// OBSOLETE 	  /* Or register ax x == arg */
// OBSOLETE 	  if (entry->header.arg < 6)
// OBSOLETE 	    {
// OBSOLETE 	      SYMBOL_CLASS (sym) = LOC_REGISTER;
// OBSOLETE 	      SYMBOL_VALUE (sym) = entry->header.arg + 8;
// OBSOLETE 	    }
// OBSOLETE 	  /* We predict indirects */
// OBSOLETE 	  locstr++;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_lsc_dreg:
// OBSOLETE 	  SYMBOL_CLASS (sym) = LOC_REGISTER;
// OBSOLETE 	  SYMBOL_VALUE (sym) = entry->header.arg;
// OBSOLETE 	  locstr++;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_lsc_section:	/* Section (arg+1) */
// OBSOLETE 	  SYMBOL_VALUE (sym) = dst_get_addr (entry->header.arg + 1, 0);
// OBSOLETE 	  locstr++;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_lsc_sec_byte:	/* Section (next_byte+1) */
// OBSOLETE 	  SYMBOL_VALUE (sym) = dst_get_addr (locstr[1] + 1, 0);
// OBSOLETE 	  locstr += 2;
// OBSOLETE 	  break;
// OBSOLETE 	case dst_lsc_add:	/* Add (arg+1)*2 */
// OBSOLETE 	case dst_lsc_sub:	/* Subtract (arg+1)*2 */
// OBSOLETE 	  temp = (entry->header.arg + 1) * 2;
// OBSOLETE 	  locstr++;
// OBSOLETE 	  if (*locstr == dst_multiply_256)
// OBSOLETE 	    {
// OBSOLETE 	      temp <<= 8;
// OBSOLETE 	      locstr++;
// OBSOLETE 	    }
// OBSOLETE 	  switch (entry->header.code)
// OBSOLETE 	    {
// OBSOLETE 	    case dst_lsc_add:
// OBSOLETE 	      if (SYMBOL_CLASS (sym) == LOC_LOCAL)
// OBSOLETE 		SYMBOL_CLASS (sym) = LOC_ARG;
// OBSOLETE 	      SYMBOL_VALUE (sym) += temp;
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_lsc_sub:
// OBSOLETE 	      SYMBOL_VALUE (sym) -= temp;
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	  break;
// OBSOLETE 	case dst_lsc_add_byte:
// OBSOLETE 	case dst_lsc_sub_byte:
// OBSOLETE 	  switch (entry->header.arg & 0x03)
// OBSOLETE 	    {
// OBSOLETE 	    case 1:
// OBSOLETE 	      temp = (unsigned char) locstr[1];
// OBSOLETE 	      locstr += 2;
// OBSOLETE 	      break;
// OBSOLETE 	    case 2:
// OBSOLETE 	      temp = *(unsigned short *) (locstr + 1);
// OBSOLETE 	      locstr += 3;
// OBSOLETE 	      break;
// OBSOLETE 	    case 3:
// OBSOLETE 	      temp = *(unsigned long *) (locstr + 1);
// OBSOLETE 	      locstr += 5;
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	  if (*locstr == dst_multiply_256)
// OBSOLETE 	    {
// OBSOLETE 	      temp <<= 8;
// OBSOLETE 	      locstr++;
// OBSOLETE 	    }
// OBSOLETE 	  switch (entry->header.code)
// OBSOLETE 	    {
// OBSOLETE 	    case dst_lsc_add_byte:
// OBSOLETE 	      if (SYMBOL_CLASS (sym) == LOC_LOCAL)
// OBSOLETE 		SYMBOL_CLASS (sym) = LOC_ARG;
// OBSOLETE 	      SYMBOL_VALUE (sym) += temp;
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_lsc_sub_byte:
// OBSOLETE 	      SYMBOL_VALUE (sym) -= temp;
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	  break;
// OBSOLETE 	case dst_lsc_sbreg:	/* Stack base register (frame pointer). Arg==0 */
// OBSOLETE 	  if (next_entry->header.code != dst_lsc_indirect)
// OBSOLETE 	    {
// OBSOLETE 	      SYMBOL_VALUE (sym) = 0;
// OBSOLETE 	      SYMBOL_CLASS (sym) = LOC_STATIC;
// OBSOLETE 	      return;
// OBSOLETE 	    }
// OBSOLETE 	  SYMBOL_VALUE (sym) = 0;
// OBSOLETE 	  SYMBOL_CLASS (sym) = LOC_LOCAL;
// OBSOLETE 	  locstr++;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  SYMBOL_VALUE (sym) = 0;
// OBSOLETE 	  SYMBOL_CLASS (sym) = LOC_STATIC;
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct symbol_list *
// OBSOLETE process_dst_symbols (struct objfile *objfile, dst_rec_ptr_t entry, char *name,
// OBSOLETE 		     int *nsyms_ret)
// OBSOLETE {
// OBSOLETE   struct symbol_list *list = NULL, *element;
// OBSOLETE   struct symbol *sym;
// OBSOLETE   char *symname;
// OBSOLETE   int nsyms = 0;
// OBSOLETE   char *location;
// OBSOLETE   long line;
// OBSOLETE   dst_type_t symtype;
// OBSOLETE   struct type *type;
// OBSOLETE   dst_var_attr_t attr;
// OBSOLETE   dst_var_loc_t loc_type;
// OBSOLETE   unsigned loc_index;
// OBSOLETE   long loc_value;
// OBSOLETE 
// OBSOLETE   if (!entry)
// OBSOLETE     {
// OBSOLETE       *nsyms_ret = 0;
// OBSOLETE       return NULL;
// OBSOLETE     }
// OBSOLETE   location = (char *) entry;
// OBSOLETE   while (NEXT_SYM (&location, &entry) &&
// OBSOLETE 	 entry->rec_type != dst_typ_end_scope)
// OBSOLETE     {
// OBSOLETE       if (entry->rec_type == dst_typ_var)
// OBSOLETE 	{
// OBSOLETE 	  if (DST_var (entry).short_locs)
// OBSOLETE 	    {
// OBSOLETE 	      loc_type = DST_var (entry).locs.shorts[0].loc_type;
// OBSOLETE 	      loc_index = DST_var (entry).locs.shorts[0].loc_index;
// OBSOLETE 	      loc_value = DST_var (entry).locs.shorts[0].location;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      loc_type = DST_var (entry).locs.longs[0].loc_type;
// OBSOLETE 	      loc_index = DST_var (entry).locs.longs[0].loc_index;
// OBSOLETE 	      loc_value = DST_var (entry).locs.longs[0].location;
// OBSOLETE 	    }
// OBSOLETE 	  if (loc_type == dst_var_loc_external)
// OBSOLETE 	    continue;
// OBSOLETE 	  symname = DST_OFFSET (entry, DST_var (entry).noffset);
// OBSOLETE 	  line = DST_var (entry).src_loc.line_number;
// OBSOLETE 	  symtype = DST_var (entry).type_desc;
// OBSOLETE 	  attr = DST_var (entry).attributes;
// OBSOLETE 	}
// OBSOLETE       else if (entry->rec_type == dst_typ_variable)
// OBSOLETE 	{
// OBSOLETE 	  symname = DST_OFFSET (entry,
// OBSOLETE 				DST_variable (entry).noffset);
// OBSOLETE 	  line = DST_variable (entry).src_loc.line_number;
// OBSOLETE 	  symtype = DST_variable (entry).type_desc;
// OBSOLETE 	  attr = DST_variable (entry).attributes;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  continue;
// OBSOLETE 	}
// OBSOLETE       if (symname && name && !strcmp (symname, name))
// OBSOLETE 	/* It's the function return value */
// OBSOLETE 	continue;
// OBSOLETE       sym = create_new_symbol (objfile, symname);
// OBSOLETE 
// OBSOLETE       if ((attr & (1 << dst_var_attr_global)) ||
// OBSOLETE 	  (attr & (1 << dst_var_attr_static)))
// OBSOLETE 	SYMBOL_CLASS (sym) = LOC_STATIC;
// OBSOLETE       else
// OBSOLETE 	SYMBOL_CLASS (sym) = LOC_LOCAL;
// OBSOLETE       SYMBOL_LINE (sym) = line;
// OBSOLETE       SYMBOL_TYPE (sym) = decode_type_desc (objfile, &symtype,
// OBSOLETE 					    entry);
// OBSOLETE       SYMBOL_VALUE (sym) = 0;
// OBSOLETE       switch (entry->rec_type)
// OBSOLETE 	{
// OBSOLETE 	case dst_typ_var:
// OBSOLETE 	  switch (loc_type)
// OBSOLETE 	    {
// OBSOLETE 	    case dst_var_loc_abs:
// OBSOLETE 	      SYMBOL_VALUE_ADDRESS (sym) = loc_value;
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_var_loc_sect_off:
// OBSOLETE 	    case dst_var_loc_ind_sect_off:	/* What is this? */
// OBSOLETE 	      SYMBOL_VALUE_ADDRESS (sym) = dst_get_addr (
// OBSOLETE 							  loc_index,
// OBSOLETE 							  loc_value);
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_var_loc_ind_reg_rel:	/* What is this? */
// OBSOLETE 	    case dst_var_loc_reg_rel:
// OBSOLETE 	      /* If it isn't fp relative, specify the
// OBSOLETE 	       * register it's relative to.
// OBSOLETE 	       */
// OBSOLETE 	      if (loc_index)
// OBSOLETE 		{
// OBSOLETE 		  sym->aux_value.basereg = loc_index;
// OBSOLETE 		}
// OBSOLETE 	      SYMBOL_VALUE (sym) = loc_value;
// OBSOLETE 	      if (loc_value > 0 &&
// OBSOLETE 		  SYMBOL_CLASS (sym) == LOC_BASEREG)
// OBSOLETE 		SYMBOL_CLASS (sym) = LOC_BASEREG_ARG;
// OBSOLETE 	      break;
// OBSOLETE 	    case dst_var_loc_reg:
// OBSOLETE 	      SYMBOL_VALUE (sym) = loc_index;
// OBSOLETE 	      SYMBOL_CLASS (sym) = LOC_REGISTER;
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	  break;
// OBSOLETE 	case dst_typ_variable:
// OBSOLETE 	  /* External variable..... don't try to interpret
// OBSOLETE 	   * its nonexistant locstring.
// OBSOLETE 	   */
// OBSOLETE 	  if (DST_variable (entry).loffset == -1)
// OBSOLETE 	    continue;
// OBSOLETE 	  decode_dst_locstring (DST_OFFSET (entry,
// OBSOLETE 					    DST_variable (entry).loffset),
// OBSOLETE 				sym);
// OBSOLETE 	}
// OBSOLETE       element = (struct symbol_list *)
// OBSOLETE 	xmalloc (sizeof (struct symbol_list));
// OBSOLETE 
// OBSOLETE       if (attr & (1 << dst_var_attr_global))
// OBSOLETE 	{
// OBSOLETE 	  element->next = dst_global_symbols;
// OBSOLETE 	  dst_global_symbols = element;
// OBSOLETE 	  total_globals++;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  element->next = list;
// OBSOLETE 	  list = element;
// OBSOLETE 	  nsyms++;
// OBSOLETE 	}
// OBSOLETE       element->symbol = sym;
// OBSOLETE     }
// OBSOLETE   *nsyms_ret = nsyms;
// OBSOLETE   return list;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE static struct symbol *
// OBSOLETE process_dst_function (struct objfile *objfile, dst_rec_ptr_t entry, char *name,
// OBSOLETE 		      CORE_ADDR address)
// OBSOLETE {
// OBSOLETE   struct symbol *sym;
// OBSOLETE   struct type *type, *ftype;
// OBSOLETE   dst_rec_ptr_t sym_entry, typ_entry;
// OBSOLETE   char *location;
// OBSOLETE   struct symbol_list *element;
// OBSOLETE 
// OBSOLETE   type = builtin_type_int;
// OBSOLETE   sym = create_new_symbol (objfile, name);
// OBSOLETE   SYMBOL_CLASS (sym) = LOC_BLOCK;
// OBSOLETE 
// OBSOLETE   if (entry)
// OBSOLETE     {
// OBSOLETE       location = (char *) entry;
// OBSOLETE       do
// OBSOLETE 	{
// OBSOLETE 	  NEXT_SYM (&location, &sym_entry);
// OBSOLETE 	}
// OBSOLETE       while (sym_entry && sym_entry->rec_type != dst_typ_signature);
// OBSOLETE 
// OBSOLETE       if (sym_entry)
// OBSOLETE 	{
// OBSOLETE 	  SYMBOL_LINE (sym) =
// OBSOLETE 	    DST_signature (sym_entry).src_loc.line_number;
// OBSOLETE 	  if (DST_signature (sym_entry).result)
// OBSOLETE 	    {
// OBSOLETE 	      typ_entry = (dst_rec_ptr_t)
// OBSOLETE 		DST_OFFSET (sym_entry,
// OBSOLETE 			    DST_signature (sym_entry).result);
// OBSOLETE 	      type = decode_dst_type (objfile, typ_entry);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (!type->function_type)
// OBSOLETE     {
// OBSOLETE       ftype = alloc_type (objfile);
// OBSOLETE       type->function_type = ftype;
// OBSOLETE       TYPE_TARGET_TYPE (ftype) = type;
// OBSOLETE       TYPE_CODE (ftype) = TYPE_CODE_FUNC;
// OBSOLETE     }
// OBSOLETE   SYMBOL_TYPE (sym) = type->function_type;
// OBSOLETE 
// OBSOLETE   /* Now add ourselves to the global symbols list */
// OBSOLETE   element = (struct symbol_list *)
// OBSOLETE     xmalloc (sizeof (struct symbol_list));
// OBSOLETE 
// OBSOLETE   element->next = dst_global_symbols;
// OBSOLETE   dst_global_symbols = element;
// OBSOLETE   total_globals++;
// OBSOLETE   element->symbol = sym;
// OBSOLETE 
// OBSOLETE   return sym;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct block *
// OBSOLETE process_dst_block (struct objfile *objfile, dst_rec_ptr_t entry)
// OBSOLETE {
// OBSOLETE   struct block *block;
// OBSOLETE   struct symbol *function = NULL;
// OBSOLETE   CORE_ADDR address;
// OBSOLETE   long size;
// OBSOLETE   char *name;
// OBSOLETE   dst_rec_ptr_t child_entry, symbol_entry;
// OBSOLETE   struct block *child_block;
// OBSOLETE   int total_symbols = 0;
// OBSOLETE   char fake_name[20];
// OBSOLETE   static long fake_seq = 0;
// OBSOLETE   struct symbol_list *symlist, *nextsym;
// OBSOLETE   int symnum;
// OBSOLETE 
// OBSOLETE   if (DST_block (entry).noffset)
// OBSOLETE     name = DST_OFFSET (entry, DST_block (entry).noffset);
// OBSOLETE   else
// OBSOLETE     name = NULL;
// OBSOLETE   if (DST_block (entry).n_of_code_ranges)
// OBSOLETE     {
// OBSOLETE       address = dst_sym_addr (
// OBSOLETE 			       &DST_block (entry).code_ranges[0].code_start);
// OBSOLETE       size = DST_block (entry).code_ranges[0].code_size;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       address = -1;
// OBSOLETE       size = 0;
// OBSOLETE     }
// OBSOLETE   symbol_entry = (dst_rec_ptr_t) get_sec_ref (&DST_block (entry).symbols_start);
// OBSOLETE   switch (DST_block (entry).block_type)
// OBSOLETE     {
// OBSOLETE       /* These are all really functions. Even the "program" type.
// OBSOLETE        * This is because the Apollo OS was written in Pascal, and
// OBSOLETE        * in Pascal, the main procedure is described as the Program.
// OBSOLETE        * Cute, huh?
// OBSOLETE        */
// OBSOLETE     case dst_block_procedure:
// OBSOLETE     case dst_block_function:
// OBSOLETE     case dst_block_subroutine:
// OBSOLETE     case dst_block_program:
// OBSOLETE       prim_record_minimal_symbol (name, address, mst_text, objfile);
// OBSOLETE       function = process_dst_function (
// OBSOLETE 					objfile,
// OBSOLETE 					symbol_entry,
// OBSOLETE 					name,
// OBSOLETE 					address);
// OBSOLETE       enter_all_lines (get_sec_ref (&DST_block (entry).code_ranges[0].lines_start), address);
// OBSOLETE       break;
// OBSOLETE     case dst_block_block_data:
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default:
// OBSOLETE       /* GDB has to call it something, and the module name
// OBSOLETE        * won't cut it
// OBSOLETE        */
// OBSOLETE       sprintf (fake_name, "block_%08lx", fake_seq++);
// OBSOLETE       function = process_dst_function (
// OBSOLETE 					objfile, NULL, fake_name, address);
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   symlist = process_dst_symbols (objfile, symbol_entry,
// OBSOLETE 				 name, &total_symbols);
// OBSOLETE   block = (struct block *)
// OBSOLETE     obstack_alloc (&objfile->symbol_obstack,
// OBSOLETE 		   sizeof (struct block) +
// OBSOLETE 		     (total_symbols - 1) * sizeof (struct symbol *));
// OBSOLETE 
// OBSOLETE   symnum = 0;
// OBSOLETE   while (symlist)
// OBSOLETE     {
// OBSOLETE       nextsym = symlist->next;
// OBSOLETE 
// OBSOLETE       block->sym[symnum] = symlist->symbol;
// OBSOLETE 
// OBSOLETE       xfree (symlist);
// OBSOLETE       symlist = nextsym;
// OBSOLETE       symnum++;
// OBSOLETE     }
// OBSOLETE   BLOCK_NSYMS (block) = total_symbols;
// OBSOLETE   BLOCK_HASHTABLE (block) = 0;
// OBSOLETE   BLOCK_START (block) = address;
// OBSOLETE   BLOCK_END (block) = address + size;
// OBSOLETE   BLOCK_SUPERBLOCK (block) = 0;
// OBSOLETE   if (function)
// OBSOLETE     {
// OBSOLETE       SYMBOL_BLOCK_VALUE (function) = block;
// OBSOLETE       BLOCK_FUNCTION (block) = function;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     BLOCK_FUNCTION (block) = 0;
// OBSOLETE 
// OBSOLETE   if (DST_block (entry).child_block_off)
// OBSOLETE     {
// OBSOLETE       child_entry = (dst_rec_ptr_t) DST_OFFSET (entry,
// OBSOLETE 					 DST_block (entry).child_block_off);
// OBSOLETE       while (child_entry)
// OBSOLETE 	{
// OBSOLETE 	  child_block = process_dst_block (objfile, child_entry);
// OBSOLETE 	  if (child_block)
// OBSOLETE 	    {
// OBSOLETE 	      if (BLOCK_START (child_block) <
// OBSOLETE 		  BLOCK_START (block) ||
// OBSOLETE 		  BLOCK_START (block) == -1)
// OBSOLETE 		BLOCK_START (block) =
// OBSOLETE 		  BLOCK_START (child_block);
// OBSOLETE 	      if (BLOCK_END (child_block) >
// OBSOLETE 		  BLOCK_END (block) ||
// OBSOLETE 		  BLOCK_END (block) == -1)
// OBSOLETE 		BLOCK_END (block) =
// OBSOLETE 		  BLOCK_END (child_block);
// OBSOLETE 	      BLOCK_SUPERBLOCK (child_block) = block;
// OBSOLETE 	    }
// OBSOLETE 	  if (DST_block (child_entry).sibling_block_off)
// OBSOLETE 	    child_entry = (dst_rec_ptr_t) DST_OFFSET (
// OBSOLETE 						       child_entry,
// OBSOLETE 				 DST_block (child_entry).sibling_block_off);
// OBSOLETE 	  else
// OBSOLETE 	    child_entry = NULL;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   record_pending_block (objfile, block, NULL);
// OBSOLETE   return block;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE read_dst_symtab (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   char *buffer;
// OBSOLETE   dst_rec_ptr_t entry, file_table, root_block;
// OBSOLETE   char *source_file;
// OBSOLETE   struct block *block, *global_block;
// OBSOLETE   int symnum;
// OBSOLETE   struct symbol_list *nextsym;
// OBSOLETE   int module_num = 0;
// OBSOLETE   struct structure_list *element;
// OBSOLETE 
// OBSOLETE   current_objfile = objfile;
// OBSOLETE   buffer = blocks_info.buffer;
// OBSOLETE   while (NEXT_BLK (&buffer, &entry))
// OBSOLETE     {
// OBSOLETE       if (entry->rec_type == dst_typ_comp_unit)
// OBSOLETE 	{
// OBSOLETE 	  file_table = (dst_rec_ptr_t) DST_OFFSET (entry,
// OBSOLETE 					  DST_comp_unit (entry).file_table);
// OBSOLETE 	  section_table = (dst_rec_ptr_t) DST_OFFSET (entry,
// OBSOLETE 				       DST_comp_unit (entry).section_table);
// OBSOLETE 	  root_block = (dst_rec_ptr_t) DST_OFFSET (entry,
// OBSOLETE 				   DST_comp_unit (entry).root_block_offset);
// OBSOLETE 	  source_file = DST_OFFSET (file_table,
// OBSOLETE 				DST_file_tab (file_table).files[0].noffset);
// OBSOLETE 	  /* Point buffer to the start of the next comp_unit */
// OBSOLETE 	  buffer = DST_OFFSET (entry,
// OBSOLETE 			       DST_comp_unit (entry).data_size);
// OBSOLETE 	  dst_start_symtab ();
// OBSOLETE 
// OBSOLETE 	  block = process_dst_block (objfile, root_block);
// OBSOLETE 
// OBSOLETE 	  global_block = (struct block *)
// OBSOLETE 	    obstack_alloc (&objfile->symbol_obstack,
// OBSOLETE 			   sizeof (struct block) +
// OBSOLETE 			     (total_globals - 1) *
// OBSOLETE 			   sizeof (struct symbol *));
// OBSOLETE 	  BLOCK_NSYMS (global_block) = total_globals;
// OBSOLETE 	  BLOCK_HASHTABLE (global_block) = 0;
// OBSOLETE 	  for (symnum = 0; symnum < total_globals; symnum++)
// OBSOLETE 	    {
// OBSOLETE 	      nextsym = dst_global_symbols->next;
// OBSOLETE 
// OBSOLETE 	      global_block->sym[symnum] =
// OBSOLETE 		dst_global_symbols->symbol;
// OBSOLETE 
// OBSOLETE 	      xfree (dst_global_symbols);
// OBSOLETE 	      dst_global_symbols = nextsym;
// OBSOLETE 	    }
// OBSOLETE 	  dst_global_symbols = NULL;
// OBSOLETE 	  total_globals = 0;
// OBSOLETE 	  BLOCK_FUNCTION (global_block) = 0;
// OBSOLETE 	  BLOCK_START (global_block) = BLOCK_START (block);
// OBSOLETE 	  BLOCK_END (global_block) = BLOCK_END (block);
// OBSOLETE 	  BLOCK_SUPERBLOCK (global_block) = 0;
// OBSOLETE 	  BLOCK_SUPERBLOCK (block) = global_block;
// OBSOLETE 	  record_pending_block (objfile, global_block, NULL);
// OBSOLETE 
// OBSOLETE 	  complete_symtab (source_file,
// OBSOLETE 			   BLOCK_START (block),
// OBSOLETE 			   BLOCK_END (block) - BLOCK_START (block));
// OBSOLETE 	  module_num++;
// OBSOLETE 	  dst_end_symtab (objfile);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   if (module_num)
// OBSOLETE     prim_record_minimal_symbol ("<end_of_program>",
// OBSOLETE 				BLOCK_END (block), mst_text, objfile);
// OBSOLETE   /* One more faked symbol to make sure nothing can ever run off the
// OBSOLETE    * end of the symbol table. This one represents the end of the
// OBSOLETE    * text space. It used to be (CORE_ADDR) -1 (effectively the highest
// OBSOLETE    * int possible), but some parts of gdb treated it as a signed
// OBSOLETE    * number and failed comparisons. We could equally use 7fffffff,
// OBSOLETE    * but no functions are ever mapped to an address higher than
// OBSOLETE    * 40000000
// OBSOLETE    */
// OBSOLETE   prim_record_minimal_symbol ("<end_of_text>",
// OBSOLETE 			      (CORE_ADDR) 0x40000000,
// OBSOLETE 			      mst_text, objfile);
// OBSOLETE   while (struct_list)
// OBSOLETE     {
// OBSOLETE       element = struct_list;
// OBSOLETE       struct_list = element->next;
// OBSOLETE       xfree (element);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Support for line number handling */
// OBSOLETE static char *linetab = NULL;
// OBSOLETE static long linetab_offset;
// OBSOLETE static unsigned long linetab_size;
// OBSOLETE 
// OBSOLETE /* Read in all the line numbers for fast lookups later.  Leave them in
// OBSOLETE    external (unswapped) format in memory; we'll swap them as we enter
// OBSOLETE    them into GDB's data structures.  */
// OBSOLETE static int
// OBSOLETE init_one_section (int chan, dst_sec *secinfo)
// OBSOLETE {
// OBSOLETE   if (secinfo->size == 0
// OBSOLETE       || lseek (chan, secinfo->position, 0) == -1
// OBSOLETE       || (secinfo->buffer = xmalloc (secinfo->size)) == NULL
// OBSOLETE       || myread (chan, secinfo->buffer, secinfo->size) == -1)
// OBSOLETE     return 0;
// OBSOLETE   else
// OBSOLETE     return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE init_dst_sections (int chan)
// OBSOLETE {
// OBSOLETE 
// OBSOLETE   if (!init_one_section (chan, &blocks_info) ||
// OBSOLETE       !init_one_section (chan, &lines_info) ||
// OBSOLETE       !init_one_section (chan, &symbols_info))
// OBSOLETE     return -1;
// OBSOLETE   else
// OBSOLETE     return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Fake up support for relocating symbol addresses.  FIXME.  */
// OBSOLETE 
// OBSOLETE struct section_offsets dst_symfile_faker =
// OBSOLETE {0};
// OBSOLETE 
// OBSOLETE void
// OBSOLETE dst_symfile_offsets (struct objfile *objfile, struct section_addr_info *addrs)
// OBSOLETE {
// OBSOLETE   objfile->num_sections = 1;
// OBSOLETE   objfile->section_offsets = &dst_symfile_faker;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Register our ability to parse symbols for DST BFD files */
// OBSOLETE 
// OBSOLETE static struct sym_fns dst_sym_fns =
// OBSOLETE {
// OBSOLETE   /* FIXME: Can this be integrated with coffread.c?  If not, should it be
// OBSOLETE      a separate flavour like ecoff?  */
// OBSOLETE   (enum bfd_flavour) -2,
// OBSOLETE 
// OBSOLETE   dst_new_init,			/* sym_new_init: init anything gbl to entire symtab */
// OBSOLETE   dst_symfile_init,		/* sym_init: read initial info, setup for sym_read() */
// OBSOLETE   dst_symfile_read,		/* sym_read: read a symbol file into symtab */
// OBSOLETE   dst_symfile_finish,		/* sym_finish: finished with file, cleanup */
// OBSOLETE   dst_symfile_offsets,		/* sym_offsets:  xlate external to internal form */
// OBSOLETE   NULL				/* next: pointer to next struct sym_fns */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_dstread (void)
// OBSOLETE {
// OBSOLETE   add_symtab_fns (&dst_sym_fns);
// OBSOLETE }
