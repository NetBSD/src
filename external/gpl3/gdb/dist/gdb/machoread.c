/* Darwin support for GDB, the GNU debugger.
   Copyright (C) 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

   Contributed by AdaCore.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "buildsym.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "mach-o.h"
#include "gdb_assert.h"
#include "aout/stab_gnu.h"
#include "vec.h"
#include "psympriv.h"

#include <string.h>

/* If non-zero displays debugging message.  */
static int mach_o_debug_level = 0;

/* Dwarf debugging information are never in the final executable.  They stay
   in object files and the executable contains the list of object files read
   during the link.
   Each time an oso (other source) is found in the executable, the reader
   creates such a structure.  They are read after the processing of the
   executable.  */

typedef struct oso_el
{
  /* Object file name.  */
  const char *name;

  /* Associated time stamp.  */
  unsigned long mtime;

  /* Number of sections.  This is the length of SYMBOLS and OFFSETS array.  */
  int num_sections;

  /* Each seaction of the object file is represented by a symbol and its
     offset.  If the offset is 0, we assume that the symbol is at offset 0
     in the OSO object file and a symbol lookup in the main file is
     required to get the offset.  */
  asymbol **symbols;
  bfd_vma *offsets;
}
oso_el;

/* Vector of object files to be read after the executable.  This is one
   global variable but it's life-time is the one of macho_symfile_read.  */
DEF_VEC_O (oso_el);
static VEC (oso_el) *oso_vector;

struct macho_oso_data
{
  /* Per objfile symbol table.  This is used to apply relocation to sections
     It is loaded only once, then relocated, and free after sections are
     relocated.  */
  asymbol **symbol_table;

  /* The offsets for this objfile.  Used to relocate the symbol_table.  */
  struct oso_el *oso;

  struct objfile *main_objfile;
};

/* Data for OSO being processed.  */

static struct macho_oso_data current_oso;

static void
macho_new_init (struct objfile *objfile)
{
}

static void
macho_symfile_init (struct objfile *objfile)
{
  objfile->flags |= OBJF_REORDERED;
  init_entry_point_info (objfile);
}

/*  Add a new OSO to the vector of OSO to load.  */

static void
macho_register_oso (const asymbol *oso_sym, int nbr_sections,
                    asymbol **symbols, bfd_vma *offsets)
{
  oso_el el;

  el.name = oso_sym->name;
  el.mtime = oso_sym->value;
  el.num_sections = nbr_sections;
  el.symbols = symbols;
  el.offsets = offsets;
  VEC_safe_push (oso_el, oso_vector, &el);
}

/* Build the minimal symbol table from SYMBOL_TABLE of length
   NUMBER_OF_SYMBOLS for OBJFILE.
   Read OSO files at the end.  */

static void
macho_symtab_read (struct objfile *objfile,
		   long number_of_symbols, asymbol **symbol_table)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  long storage_needed;
  long i, j;
  CORE_ADDR offset;
  enum minimal_symbol_type ms_type;
  unsigned int nbr_sections = bfd_count_sections (objfile->obfd);
  asymbol **first_symbol = NULL;
  bfd_vma *first_offset = NULL;
  const asymbol *oso_file = NULL;

  for (i = 0; i < number_of_symbols; i++)
    {
      asymbol *sym = symbol_table[i];
      bfd_mach_o_asymbol *mach_o_sym = (bfd_mach_o_asymbol *)sym;

      offset = ANOFFSET (objfile->section_offsets, sym->section->index);

      if (sym->flags & BSF_DEBUGGING)
	{
	  bfd_vma addr;

          /* Debugging symbols are used to collect OSO file names as well
             as section offsets.  */

	  switch (mach_o_sym->n_type)
	    {
	    case N_SO:
              /* An empty SO entry terminates a chunk for an OSO file.  */
	      if ((sym->name == NULL || sym->name[0] == 0) && oso_file != NULL)
		{
		  macho_register_oso (oso_file, nbr_sections,
                                      first_symbol, first_offset);
		  first_symbol = NULL;
		  first_offset = NULL;
		  oso_file = NULL;
		}
	      break;
	    case N_FUN:
	    case N_STSYM:
	      if (sym->name == NULL || sym->name[0] == '\0')
		break;
	      /* Fall through.  */
	    case N_BNSYM:
	      gdb_assert (oso_file != NULL);
	      addr = sym->value 
		+ bfd_get_section_vma (sym->section->bfd, sym->section);
	      if (addr != 0
		  && first_symbol[sym->section->index] == NULL)
		{
                  /* These STAB entries can directly relocate a section.  */
		  first_symbol[sym->section->index] = sym;
		  first_offset[sym->section->index] = addr + offset;
		}
	      break;
	    case N_GSYM:
	      gdb_assert (oso_file != NULL);
	      if (first_symbol[sym->section->index] == NULL)
                {
                  /* This STAB entry needs a symbol look-up to relocate
                     the section.  */
                  first_symbol[sym->section->index] = sym;
                  first_offset[sym->section->index] = 0;
                }
	      break;
	    case N_OSO:
              /* New OSO file.  */
	      gdb_assert (oso_file == NULL);
	      first_symbol = (asymbol **)xmalloc (nbr_sections
						  * sizeof (asymbol *));
	      first_offset = (bfd_vma *)xmalloc (nbr_sections
						 * sizeof (bfd_vma));
	      for (j = 0; j < nbr_sections; j++)
		first_symbol[j] = NULL;
	      oso_file = sym;
	      break;
	    }
	  continue;
	}

      if (sym->name == NULL || *sym->name == '\0')
	{
	  /* Skip names that don't exist (shouldn't happen), or names
	     that are null strings (may happen).  */
	  continue;
	}

      if (sym->flags & (BSF_GLOBAL | BSF_LOCAL | BSF_WEAK))
	{
	  struct minimal_symbol *msym;
	  CORE_ADDR symaddr;

	  /* Bfd symbols are section relative.  */
	  symaddr = sym->value + sym->section->vma;

	  /* Select global/local/weak symbols.  Note that bfd puts abs
	     symbols in their own section, so all symbols we are
	     interested in will have a section.  */
	  /* Relocate all non-absolute and non-TLS symbols by the
	     section offset.  */
	  if (sym->section != &bfd_abs_section
	      && !(sym->section->flags & SEC_THREAD_LOCAL))
	    symaddr += offset;

	  if (sym->section == &bfd_abs_section)
	    ms_type = mst_abs;
	  else if (sym->section->flags & SEC_CODE)
	    {
	      if (sym->flags & (BSF_GLOBAL | BSF_WEAK))
		ms_type = mst_text;
	      else
		ms_type = mst_file_text;
	    }
	  else if (sym->section->flags & SEC_ALLOC)
	    {
	      if (sym->flags & (BSF_GLOBAL | BSF_WEAK))
		{
		  if (sym->section->flags & SEC_LOAD)
		    ms_type = mst_data;
		  else
		    ms_type = mst_bss;
		}
	      else if (sym->flags & BSF_LOCAL)
		{
		  /* Not a special stabs-in-elf symbol, do regular
		     symbol processing.  */
		  if (sym->section->flags & SEC_LOAD)
		    ms_type = mst_file_data;
		  else
		    ms_type = mst_file_bss;
		}
	      else
		ms_type = mst_unknown;
	    }
	  else
	    continue;	/* Skip this symbol.  */

	  gdb_assert (sym->section->index < nbr_sections);
	  if (oso_file != NULL
	      && first_symbol[sym->section->index] == NULL)
	    {
              /* Standard symbols can directly relocate sections.  */
	      first_symbol[sym->section->index] = sym;
	      first_offset[sym->section->index] = symaddr;
	    }

	  msym = prim_record_minimal_symbol_and_info
	    (sym->name, symaddr, ms_type, sym->section->index,
	     sym->section, objfile);
	}
    }

  /* Just in case there is no trailing SO entry.  */
  if (oso_file != NULL)
    macho_register_oso (oso_file, nbr_sections, first_symbol, first_offset);
}

/* If NAME describes an archive member (ie: ARCHIVE '(' MEMBER ')'),
   returns the length of the archive name.
   Returns -1 otherwise.  */

static int
get_archive_prefix_len (const char *name)
{
  char *lparen;
  int name_len = strlen (name);

  if (name_len == 0 || name[name_len - 1] != ')')
    return -1;
  
  lparen = strrchr (name, '(');
  if (lparen == NULL || lparen == name)
    return -1;
  return lparen - name;
}

static int
oso_el_compare_name (const void *vl, const void *vr)
{
  const oso_el *l = (const oso_el *)vl;
  const oso_el *r = (const oso_el *)vr;

  return strcmp (l->name, r->name);
}

/* Add an oso file as a symbol file.  */

static void
macho_add_oso_symfile (oso_el *oso, bfd *abfd,
                       struct objfile *main_objfile, int symfile_flags)
{
  struct objfile *objfile;
  int i;
  char leading_char;

  if (mach_o_debug_level > 0)
    printf_unfiltered (_("Loading symbols from oso: %s\n"), oso->name);

  if (!bfd_check_format (abfd, bfd_object))
    {
      warning (_("`%s': can't read symbols: %s."), oso->name,
               bfd_errmsg (bfd_get_error ()));
      bfd_close (abfd);
      return;
    }

  bfd_set_cacheable (abfd, 1);

  /* Relocate sections.  */

  leading_char = bfd_get_symbol_leading_char (main_objfile->obfd);

  for (i = 0; i < oso->num_sections; i++)
    {
      asection *sect;
      const char *sectname;
      bfd_vma vma;

      /* Empty slot.  */
      if (oso->symbols[i] == NULL)
        continue;

      if (oso->offsets[i])
        vma = oso->offsets[i];
      else
        {
          struct minimal_symbol *msym;
          const char *name = oso->symbols[i]->name;

          if (name[0] == leading_char)
            ++name;

          if (mach_o_debug_level > 3)
            printf_unfiltered (_("resolve sect %s with %s\n"),
                               oso->symbols[i]->section->name,
                               oso->symbols[i]->name);
          msym = lookup_minimal_symbol (name, NULL, main_objfile);
          if (msym == NULL)
            {
              warning (_("can't find symbol '%s' in minsymtab"), name);
              continue;
            }
          else
            vma = SYMBOL_VALUE_ADDRESS (msym);
        }
      sectname = (char *)oso->symbols[i]->section->name;

      sect = bfd_get_section_by_name (abfd, sectname);
      if (sect == NULL)
        {
          warning (_("can't find section '%s' in OSO file %s"),
                   sectname, oso->name);
          continue;
        }
      bfd_set_section_vma (abfd, sect, vma);

      if (mach_o_debug_level > 1)
        printf_unfiltered (_("  %s: %s\n"),
                           core_addr_to_string (vma), sectname);
    }

  /* Make sure that the filename was malloc'ed.  The current filename comes
     either from an OSO symbol name or from an archive name.  Memory for both
     is not managed by gdb.  */
  abfd->filename = xstrdup (abfd->filename);

  gdb_assert (current_oso.symbol_table == NULL);
  current_oso.main_objfile = main_objfile;

  /* We need to clear SYMFILE_MAINLINE to avoid interractive question
     from symfile.c:symbol_file_add_with_addrs_or_offsets.  */
  objfile = symbol_file_add_from_bfd
    (abfd, symfile_flags & ~(SYMFILE_MAINLINE | SYMFILE_VERBOSE), NULL,
     main_objfile->flags & (OBJF_REORDERED | OBJF_SHARED
                            | OBJF_READNOW | OBJF_USERLOADED));
  add_separate_debug_objfile (objfile, main_objfile);

  current_oso.main_objfile = NULL;
  if (current_oso.symbol_table)
    {
      xfree (current_oso.symbol_table);
      current_oso.symbol_table = NULL;
    }
}

/* Read symbols from the vector of oso files.  */

static void
macho_symfile_read_all_oso (struct objfile *main_objfile, int symfile_flags)
{
  int ix;
  VEC (oso_el) *vec;
  oso_el *oso;

  vec = oso_vector;
  oso_vector = NULL;
  
  /* Sort oso by name so that files from libraries are gathered.  */
  qsort (VEC_address (oso_el, vec), VEC_length (oso_el, vec),
         sizeof (oso_el), oso_el_compare_name);

  for (ix = 0; VEC_iterate (oso_el, vec, ix, oso);)
    {
      int pfx_len;
      
      /* Check if this is a library name.  */
      pfx_len = get_archive_prefix_len (oso->name);
      if (pfx_len > 0)
	{
	  bfd *archive_bfd;
	  bfd *member_bfd;
	  char *archive_name = XNEWVEC (char, pfx_len + 1);
          int last_ix;
          oso_el *oso2;
          int ix2;

	  memcpy (archive_name, oso->name, pfx_len);
	  archive_name[pfx_len] = '\0';

          /* Compute number of oso for this archive.  */
          for (last_ix = ix;
               VEC_iterate (oso_el, vec, last_ix, oso2); last_ix++)
            {
              if (strncmp (oso2->name, archive_name, pfx_len) != 0)
                break;
            }
	  
	  /* Open the archive and check the format.  */
	  archive_bfd = bfd_openr (archive_name, gnutarget);
	  if (archive_bfd == NULL)
	    {
	      warning (_("Could not open OSO archive file \"%s\""),
		       archive_name);
              ix = last_ix;
	      continue;
	    }
	  if (!bfd_check_format (archive_bfd, bfd_archive))
	    {
	      warning (_("OSO archive file \"%s\" not an archive."),
		       archive_name);
	      bfd_close (archive_bfd);
              ix = last_ix;
	      continue;
	    }
	  member_bfd = bfd_openr_next_archived_file (archive_bfd, NULL);
	  
	  if (member_bfd == NULL)
	    {
	      warning (_("Could not read archive members out of "
			 "OSO archive \"%s\""), archive_name);
	      bfd_close (archive_bfd);
              ix = last_ix;
	      continue;
	    }

          /* Load all oso in this library.  */
	  while (member_bfd != NULL)
	    {
	      bfd *prev;
	      const char *member_name = member_bfd->filename;
              int member_len = strlen (member_name);

              /* If this member is referenced, add it as a symfile.  */
              for (ix2 = ix; ix2 < last_ix; ix2++)
                {
                  oso2 = VEC_index (oso_el, vec, ix2);

                  if (oso2->name
                      && strlen (oso2->name) == pfx_len + member_len + 2
                      && !memcmp (member_name, oso2->name + pfx_len + 1,
                                  member_len))
                    {
                      macho_add_oso_symfile (oso2, member_bfd,
                                             main_objfile, symfile_flags);
                      oso2->name = NULL;
                      break;
                    }
                }

              prev = member_bfd;
	      member_bfd = bfd_openr_next_archived_file
		(archive_bfd, member_bfd);

              /* Free previous member if not referenced by an oso.  */
              if (ix2 >= last_ix)
                bfd_close (prev);
	    }
          for (ix2 = ix; ix2 < last_ix; ix2++)
            {
              oso_el *oso2 = VEC_index (oso_el, vec, ix2);

              if (oso2->name != NULL)
                warning (_("Could not find specified archive member "
                           "for OSO name \"%s\""), oso->name);
            }
          ix = last_ix;
	}
      else
	{
          bfd *abfd;

	  abfd = bfd_openr (oso->name, gnutarget);
	  if (!abfd)
            warning (_("`%s': can't open to read symbols: %s."), oso->name,
                     bfd_errmsg (bfd_get_error ()));
          else
            macho_add_oso_symfile (oso, abfd, main_objfile, symfile_flags);

          ix++;
        }
    }

  for (ix = 0; VEC_iterate (oso_el, vec, ix, oso); ix++)
    {
      xfree (oso->symbols);
      xfree (oso->offsets);
    }
  VEC_free (oso_el, vec);
}

/* DSYM (debug symbols) files contain the debug info of an executable.
   This is a separate file created by dsymutil(1) and is similar to debug
   link feature on ELF.
   DSYM files are located in a subdirectory.  Append DSYM_SUFFIX to the
   executable name and the executable base name to get the DSYM file name.  */
#define DSYM_SUFFIX ".dSYM/Contents/Resources/DWARF/"

/* Check if a dsym file exists for OBJFILE.  If so, returns a bfd for it.
   Return NULL if no valid dsym file is found.  */

static bfd *
macho_check_dsym (struct objfile *objfile)
{
  size_t name_len = strlen (objfile->name);
  size_t dsym_len = strlen (DSYM_SUFFIX);
  const char *base_name = lbasename (objfile->name);
  size_t base_len = strlen (base_name);
  char *dsym_filename = alloca (name_len + dsym_len + base_len + 1);
  bfd *dsym_bfd;
  bfd_mach_o_load_command *main_uuid;
  bfd_mach_o_load_command *dsym_uuid;

  strcpy (dsym_filename, objfile->name);
  strcpy (dsym_filename + name_len, DSYM_SUFFIX);
  strcpy (dsym_filename + name_len + dsym_len, base_name);

  if (access (dsym_filename, R_OK) != 0)
    return NULL;

  if (bfd_mach_o_lookup_command (objfile->obfd,
                                 BFD_MACH_O_LC_UUID, &main_uuid) == 0)
    {
      warning (_("can't find UUID in %s"), objfile->name);
      return NULL;
    }
  dsym_filename = xstrdup (dsym_filename);
  dsym_bfd = bfd_openr (dsym_filename, gnutarget);
  if (dsym_bfd == NULL)
    {
      warning (_("can't open dsym file %s"), dsym_filename);
      xfree (dsym_filename);
      return NULL;
    }

  if (!bfd_check_format (dsym_bfd, bfd_object))
    {
      bfd_close (dsym_bfd);
      warning (_("bad dsym file format: %s"), bfd_errmsg (bfd_get_error ()));
      xfree (dsym_filename);
      return NULL;
    }

  if (bfd_mach_o_lookup_command (dsym_bfd,
                                 BFD_MACH_O_LC_UUID, &dsym_uuid) == 0)
    {
      warning (_("can't find UUID in %s"), dsym_filename);
      bfd_close (dsym_bfd);
      xfree (dsym_filename);
      return NULL;
    }
  if (memcmp (dsym_uuid->command.uuid.uuid, main_uuid->command.uuid.uuid,
              sizeof (main_uuid->command.uuid.uuid)))
    {
      warning (_("dsym file UUID doesn't match the one in %s"), objfile->name);
      bfd_close (dsym_bfd);
      xfree (dsym_filename);
      return NULL;
    }
  return dsym_bfd;
}

static void
macho_symfile_read (struct objfile *objfile, int symfile_flags)
{
  bfd *abfd = objfile->obfd;
  struct cleanup *back_to;
  CORE_ADDR offset;
  long storage_needed;
  bfd *dsym_bfd;

  init_minimal_symbol_collection ();
  back_to = make_cleanup_discard_minimal_symbols ();

  /* Get symbols from the symbol table only if the file is an executable.
     The symbol table of object files is not relocated and is expected to
     be in the executable.  */
  if (bfd_get_file_flags (abfd) & (EXEC_P | DYNAMIC))
    {
      /* Process the normal symbol table first.  */
      storage_needed = bfd_get_symtab_upper_bound (objfile->obfd);
      if (storage_needed < 0)
	error (_("Can't read symbols from %s: %s"),
	       bfd_get_filename (objfile->obfd),
	       bfd_errmsg (bfd_get_error ()));

      if (storage_needed > 0)
	{
	  asymbol **symbol_table;
	  long symcount;

	  symbol_table = (asymbol **) xmalloc (storage_needed);
	  make_cleanup (xfree, symbol_table);
	  symcount = bfd_canonicalize_symtab (objfile->obfd, symbol_table);
	  
	  if (symcount < 0)
	    error (_("Can't read symbols from %s: %s"),
		   bfd_get_filename (objfile->obfd),
		   bfd_errmsg (bfd_get_error ()));
	  
	  macho_symtab_read (objfile, symcount, symbol_table);
	}
      
      install_minimal_symbols (objfile);

      /* Try to read .eh_frame / .debug_frame.  */
      /* First, locate these sections.  We ignore the result status
	 as it only checks for debug info.  */
      dwarf2_has_info (objfile);
      dwarf2_build_frame_info (objfile);
      
      /* Check for DSYM file.  */
      dsym_bfd = macho_check_dsym (objfile);
      if (dsym_bfd != NULL)
	{
	  int ix;
	  oso_el *oso;
          struct bfd_section *asect, *dsect;

	  if (mach_o_debug_level > 0)
	    printf_unfiltered (_("dsym file found\n"));

	  /* Remove oso.  They won't be used.  */
	  for (ix = 0; VEC_iterate (oso_el, oso_vector, ix, oso); ix++)
	    {
	      xfree (oso->symbols);
	      xfree (oso->offsets);
	    }
	  VEC_free (oso_el, oso_vector);
	  oso_vector = NULL;

          /* Set dsym section size.  */
          for (asect = objfile->obfd->sections, dsect = dsym_bfd->sections;
               asect && dsect;
               asect = asect->next, dsect = dsect->next)
            {
              if (strcmp (asect->name, dsect->name) != 0)
                break;
              bfd_set_section_size (dsym_bfd, dsect,
                                    bfd_get_section_size (asect));
            }

	  /* Add the dsym file as a separate file.  */
          symbol_file_add_separate (dsym_bfd, symfile_flags, objfile);
      
	  /* Don't try to read dwarf2 from main file or shared libraries.  */
          return;
	}
    }

  if (dwarf2_has_info (objfile))
    {
      /* DWARF 2 sections */
      dwarf2_build_psymtabs (objfile);
    }

  /* Do not try to read .eh_frame/.debug_frame as they are not relocated
     and dwarf2_build_frame_info cannot deal with unrelocated sections.  */

  /* Then the oso.  */
  if (oso_vector != NULL)
    macho_symfile_read_all_oso (objfile, symfile_flags);
}

static bfd_byte *
macho_symfile_relocate (struct objfile *objfile, asection *sectp,
                        bfd_byte *buf)
{
  bfd *abfd = objfile->obfd;

  /* We're only interested in sections with relocation
     information.  */
  if ((sectp->flags & SEC_RELOC) == 0)
    return NULL;

  if (mach_o_debug_level > 0)
    printf_unfiltered (_("Relocate section '%s' of %s\n"),
                       sectp->name, objfile->name);

  if (current_oso.symbol_table == NULL)
    {
      int storage;
      int i;
      char leading_char;

      storage = bfd_get_symtab_upper_bound (abfd);
      current_oso.symbol_table = (asymbol **) xmalloc (storage);
      bfd_canonicalize_symtab (abfd, current_oso.symbol_table);

      leading_char = bfd_get_symbol_leading_char (abfd);

      for (i = 0; current_oso.symbol_table[i]; i++)
        {
          asymbol *sym = current_oso.symbol_table[i];

          if (bfd_is_com_section (sym->section))
            {
              /* This one must be solved.  */
              struct minimal_symbol *msym;
              const char *name = sym->name;

              if (name[0] == leading_char)
                name++;

              msym = lookup_minimal_symbol
                (name, NULL, current_oso.main_objfile);
              if (msym == NULL)
                {
                  warning (_("can't find symbol '%s' in minsymtab"), name);
                  continue;
                }
              else
                {
                  sym->section = &bfd_abs_section;
                  sym->value = SYMBOL_VALUE_ADDRESS (msym);
                }
            }
        }
    }

  return bfd_simple_get_relocated_section_contents (abfd, sectp, buf, NULL);
}

static void
macho_symfile_finish (struct objfile *objfile)
{
}

static void
macho_symfile_offsets (struct objfile *objfile,
                       struct section_addr_info *addrs)
{
  unsigned int i;
  unsigned int num_sections;
  struct obj_section *osect;

  /* Allocate section_offsets.  */
  objfile->num_sections = bfd_count_sections (objfile->obfd);
  objfile->section_offsets = (struct section_offsets *)
    obstack_alloc (&objfile->objfile_obstack,
                   SIZEOF_N_SECTION_OFFSETS (objfile->num_sections));
  memset (objfile->section_offsets, 0,
          SIZEOF_N_SECTION_OFFSETS (objfile->num_sections));

  /* This code is run when we first add the objfile with
     symfile_add_with_addrs_or_offsets, when "addrs" not "offsets" are
     passed in.  The place in symfile.c where the addrs are applied
     depends on the addrs having section names.  But in the dyld code
     we build an anonymous array of addrs, so that code is a no-op.
     Because of that, we have to apply the addrs to the sections here.
     N.B. if an objfile slides after we've already created it, then it
     goes through objfile_relocate.  */

  for (i = 0; i < addrs->num_sections; i++)
    {
      if (addrs->other[i].name == NULL)
	continue;

      ALL_OBJFILE_OSECTIONS (objfile, osect)
	{
	  const char *bfd_sect_name = osect->the_bfd_section->name;

	  if (strcmp (bfd_sect_name, addrs->other[i].name) == 0)
	    {
	      obj_section_offset (osect) = addrs->other[i].addr;
	      break;
	    }
	}
    }

  objfile->sect_index_text = 0;

  ALL_OBJFILE_OSECTIONS (objfile, osect)
    {
      const char *bfd_sect_name = osect->the_bfd_section->name;
      int sect_index = osect->the_bfd_section->index;
      
      if (strncmp (bfd_sect_name, "LC_SEGMENT.", 11) == 0)
	bfd_sect_name += 11;
      if (strcmp (bfd_sect_name, "__TEXT") == 0
	  || strcmp (bfd_sect_name, "__TEXT.__text") == 0)
	objfile->sect_index_text = sect_index;
    }
}

static const struct sym_fns macho_sym_fns = {
  bfd_target_mach_o_flavour,

  macho_new_init,               /* init anything gbl to entire symtab */
  macho_symfile_init,           /* read initial info, setup for sym_read() */
  macho_symfile_read,           /* read a symbol file into symtab */
  NULL,				/* sym_read_psymbols */
  macho_symfile_finish,         /* finished with file, cleanup */
  macho_symfile_offsets,        /* xlate external to internal form */
  default_symfile_segments,	/* Get segment information from a file.  */
  NULL,
  macho_symfile_relocate,	/* Relocate a debug section.  */
  &psym_functions
};

void
_initialize_machoread ()
{
  add_symtab_fns (&macho_sym_fns);

  add_setshow_zinteger_cmd ("mach-o", class_obscure,
			    &mach_o_debug_level,
			    _("Set if printing Mach-O symbols processing."),
			    _("Show if printing Mach-O symbols processing."),
			    NULL, NULL, NULL,
			    &setdebuglist, &showdebuglist);
}
