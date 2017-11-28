/* Darwin support for GDB, the GNU debugger.
   Copyright (C) 2008-2016 Free Software Foundation, Inc.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#include "aout/stab_gnu.h"
#include "vec.h"
#include "psympriv.h"
#include "complaints.h"
#include "gdb_bfd.h"

/* If non-zero displays debugging message.  */
static unsigned int mach_o_debug_level = 0;

/* Dwarf debugging information are never in the final executable.  They stay
   in object files and the executable contains the list of object files read
   during the link.
   Each time an oso (other source) is found in the executable, the reader
   creates such a structure.  They are read after the processing of the
   executable.  */

typedef struct oso_el
{
  /* Object file name.  Can also be a member name.  */
  const char *name;

  /* Associated time stamp.  */
  unsigned long mtime;

  /* Stab symbols range for this OSO.  */
  asymbol **oso_sym;
  asymbol **end_sym;

  /* Number of interesting stabs in the range.  */
  unsigned int nbr_syms;
}
oso_el;

/* Vector of object files to be read after the executable.  */
DEF_VEC_O (oso_el);

static void
macho_new_init (struct objfile *objfile)
{
}

static void
macho_symfile_init (struct objfile *objfile)
{
  objfile->flags |= OBJF_REORDERED;
}

/*  Add a new OSO to the vector of OSO to load.  */

static void
macho_register_oso (VEC (oso_el) **oso_vector_ptr,
		    struct objfile *objfile,
                    asymbol **oso_sym, asymbol **end_sym,
                    unsigned int nbr_syms)
{
  oso_el el;

  el.name = (*oso_sym)->name;
  el.mtime = (*oso_sym)->value;
  el.oso_sym = oso_sym;
  el.end_sym = end_sym;
  el.nbr_syms = nbr_syms;
  VEC_safe_push (oso_el, *oso_vector_ptr, &el);
}

/* Add symbol SYM to the minimal symbol table of OBJFILE.  */

static void
macho_symtab_add_minsym (struct objfile *objfile, const asymbol *sym)
{
  if (sym->name == NULL || *sym->name == '\0')
    {
      /* Skip names that don't exist (shouldn't happen), or names
         that are null strings (may happen).  */
      return;
    }

  if (sym->flags & (BSF_GLOBAL | BSF_LOCAL | BSF_WEAK))
    {
      CORE_ADDR symaddr;
      enum minimal_symbol_type ms_type;

      /* Bfd symbols are section relative.  */
      symaddr = sym->value + sym->section->vma;

      if (sym->section == bfd_abs_section_ptr)
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
        return;	/* Skip this symbol.  */

      prim_record_minimal_symbol_and_info
        (sym->name, symaddr, ms_type,
	 gdb_bfd_section_index (objfile->obfd, sym->section),
	 objfile);
    }
}

/* Build the minimal symbol table from SYMBOL_TABLE of length
   NUMBER_OF_SYMBOLS for OBJFILE.  Registers OSO filenames found.  */

static void
macho_symtab_read (struct objfile *objfile,
		   long number_of_symbols, asymbol **symbol_table,
		   VEC (oso_el) **oso_vector_ptr)
{
  long i;
  const asymbol *file_so = NULL;
  asymbol **oso_file = NULL;
  unsigned int nbr_syms = 0;

  /* Current state while reading stabs.  */
  enum
  {
    /* Not within an SO part.  Only non-debugging symbols should be present,
       and will be added to the minimal symbols table.  */
    S_NO_SO,

    /* First SO read.  Introduce an SO section, and may be followed by a second
       SO.  The SO section should contain onl debugging symbols.  */
    S_FIRST_SO,

    /* Second non-null SO found, just after the first one.  Means that the first
       is in fact a directory name.  */
    S_SECOND_SO,

    /* Non-null OSO found.  Debugging info are DWARF in this OSO file.  */
    S_DWARF_FILE,

    S_STAB_FILE
  } state = S_NO_SO;

  for (i = 0; i < number_of_symbols; i++)
    {
      const asymbol *sym = symbol_table[i];
      bfd_mach_o_asymbol *mach_o_sym = (bfd_mach_o_asymbol *)sym;

      switch (state)
        {
        case S_NO_SO:
	  if (mach_o_sym->n_type == N_SO)
            {
              /* Start of object stab.  */
	      if (sym->name == NULL || sym->name[0] == 0)
                {
                  /* Unexpected empty N_SO.  */
                  complaint (&symfile_complaints,
                             _("Unexpected empty N_SO stab"));
                }
              else
                {
                  file_so = sym;
                  state = S_FIRST_SO;
                }
            }
          else if (sym->flags & BSF_DEBUGGING)
            {
              if (mach_o_sym->n_type == N_OPT)
                {
                  /* No complaint for OPT.  */
                  break;
                }

              /* Debugging symbols are not expected here.  */
              complaint (&symfile_complaints,
                         _("%s: Unexpected debug stab outside SO markers"),
                         objfile_name (objfile));
            }
          else
            {
              /* Non-debugging symbols go to the minimal symbol table.  */
              macho_symtab_add_minsym (objfile, sym);
            }
          break;

        case S_FIRST_SO:
        case S_SECOND_SO:
	  if (mach_o_sym->n_type == N_SO)
            {
	      if (sym->name == NULL || sym->name[0] == 0)
                {
                  /* Unexpected empty N_SO.  */
                  complaint (&symfile_complaints, _("Empty SO section"));
                  state = S_NO_SO;
                }
              else if (state == S_FIRST_SO)
                {
                  /* Second SO stab for the file name.  */
                  file_so = sym;
                  state = S_SECOND_SO;
                }
              else
                complaint (&symfile_complaints, _("Three SO in a raw"));
            }
          else if (mach_o_sym->n_type == N_OSO)
            {
	      if (sym->name == NULL || sym->name[0] == 0)
                {
                  /* Empty OSO.  Means that this file was compiled with
                     stabs.  */
                  state = S_STAB_FILE;
                  warning (_("stabs debugging not supported for %s"),
                           file_so->name);
                }
              else
                {
                  /* Non-empty OSO for a Dwarf file.  */
                  oso_file = symbol_table + i;
                  nbr_syms = 0;
                  state = S_DWARF_FILE;
                }
            }
          else
            complaint (&symfile_complaints,
                       _("Unexpected stab after SO"));
          break;

        case S_STAB_FILE:
        case S_DWARF_FILE:
	  if (mach_o_sym->n_type == N_SO)
            {
	      if (sym->name == NULL || sym->name[0] == 0)
                {
                  /* End of file.  */
                  if (state == S_DWARF_FILE)
                    macho_register_oso (oso_vector_ptr, objfile,
					oso_file, symbol_table + i,
                                        nbr_syms);
                  state = S_NO_SO;
                }
              else
                {
                  complaint (&symfile_complaints, _("Missing nul SO"));
                  file_so = sym;
                  state = S_FIRST_SO;
                }
            }
          else if (sym->flags & BSF_DEBUGGING)
            {
              if (state == S_STAB_FILE)
                {
                  /* FIXME: to be implemented.  */
                }
              else
                {
                  switch (mach_o_sym->n_type)
                    {
                    case N_FUN:
                      if (sym->name == NULL || sym->name[0] == 0)
                        break;
                      /* Fall through.  */
                    case N_STSYM:
                      /* Interesting symbol.  */
                      nbr_syms++;
                      break;
                    case N_ENSYM:
                    case N_BNSYM:
                    case N_GSYM:
                      break;
                    default:
                      complaint (&symfile_complaints,
                                 _("unhandled stab for dwarf OSO file"));
                      break;
                    }
                }
            }
          else
            complaint (&symfile_complaints,
                       _("non-debugging symbol within SO"));
          break;
        }
    }

  if (state != S_NO_SO)
    complaint (&symfile_complaints, _("missing nul SO"));
}

/* If NAME describes an archive member (ie: ARCHIVE '(' MEMBER ')'),
   returns the length of the archive name.
   Returns -1 otherwise.  */

static int
get_archive_prefix_len (const char *name)
{
  const char *lparen;
  int name_len = strlen (name);

  if (name_len == 0 || name[name_len - 1] != ')')
    return -1;

  lparen = strrchr (name, '(');
  if (lparen == NULL || lparen == name)
    return -1;
  return lparen - name;
}

/* Compare function to qsort OSOs, so that members of a library are
   gathered.  */

static int
oso_el_compare_name (const void *vl, const void *vr)
{
  const oso_el *l = (const oso_el *)vl;
  const oso_el *r = (const oso_el *)vr;

  return strcmp (l->name, r->name);
}

/* Hash table entry structure for the stabs symbols in the main object file.
   This is used to speed up lookup for symbols in the OSO.  */

struct macho_sym_hash_entry
{
  struct bfd_hash_entry base;
  const asymbol *sym;
};

/* Routine to create an entry in the hash table.  */

static struct bfd_hash_entry *
macho_sym_hash_newfunc (struct bfd_hash_entry *entry,
                        struct bfd_hash_table *table,
                        const char *string)
{
  struct macho_sym_hash_entry *ret = (struct macho_sym_hash_entry *) entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == NULL)
    ret = (struct macho_sym_hash_entry *) bfd_hash_allocate (table,
                                                             sizeof (* ret));
  if (ret == NULL)
    return NULL;

  /* Call the allocation method of the superclass.  */
  ret = (struct macho_sym_hash_entry *)
	 bfd_hash_newfunc ((struct bfd_hash_entry *) ret, table, string);

  if (ret)
    {
      /* Initialize the local fields.  */
      ret->sym = NULL;
    }

  return (struct bfd_hash_entry *) ret;
}

/* Get the value of SYM from the minimal symtab of MAIN_OBJFILE.  This is used
   to get the value of global and common symbols.  */

static CORE_ADDR
macho_resolve_oso_sym_with_minsym (struct objfile *main_objfile, asymbol *sym)
{
  /* For common symbol and global symbols, use the min symtab.  */
  struct bound_minimal_symbol msym;
  const char *name = sym->name;

  if (name[0] == bfd_get_symbol_leading_char (main_objfile->obfd))
    ++name;
  msym = lookup_minimal_symbol (name, NULL, main_objfile);
  if (msym.minsym == NULL)
    {
      warning (_("can't find symbol '%s' in minsymtab"), name);
      return 0;
    }
  else
    return BMSYMBOL_VALUE_ADDRESS (msym);
}

/* Add oso file OSO/ABFD as a symbol file.  */

static void
macho_add_oso_symfile (oso_el *oso, bfd *abfd, const char *name,
                       struct objfile *main_objfile, int symfile_flags)
{
  int storage;
  int i;
  asymbol **symbol_table;
  asymbol **symp;
  struct bfd_hash_table table;
  int nbr_sections;
  struct cleanup *cleanup;

  /* Per section flag to mark which section have been rebased.  */
  unsigned char *sections_rebased;

  if (mach_o_debug_level > 0)
    printf_unfiltered
      (_("Loading debugging symbols from oso: %s\n"), oso->name);

  if (!bfd_check_format (abfd, bfd_object))
    {
      warning (_("`%s': can't read symbols: %s."), oso->name,
               bfd_errmsg (bfd_get_error ()));
      gdb_bfd_unref (abfd);
      return;
    }

  if (abfd->my_archive == NULL && oso->mtime != bfd_get_mtime (abfd))
    {
      warning (_("`%s': file time stamp mismatch."), oso->name);
      gdb_bfd_unref (abfd);
      return;
    }

  if (!bfd_hash_table_init_n (&table, macho_sym_hash_newfunc,
                              sizeof (struct macho_sym_hash_entry),
                              oso->nbr_syms))
    {
      warning (_("`%s': can't create hash table"), oso->name);
      gdb_bfd_unref (abfd);
      return;
    }

  bfd_set_cacheable (abfd, 1);

  /* Read symbols table.  */
  storage = bfd_get_symtab_upper_bound (abfd);
  symbol_table = (asymbol **) xmalloc (storage);
  bfd_canonicalize_symtab (abfd, symbol_table);

  /* Init section flags.  */
  nbr_sections = bfd_count_sections (abfd);
  sections_rebased = (unsigned char *) alloca (nbr_sections);
  for (i = 0; i < nbr_sections; i++)
    sections_rebased[i] = 0;

  /* Put symbols for the OSO file in the hash table.  */
  for (symp = oso->oso_sym; symp != oso->end_sym; symp++)
    {
      const asymbol *sym = *symp;
      bfd_mach_o_asymbol *mach_o_sym = (bfd_mach_o_asymbol *)sym;

      switch (mach_o_sym->n_type)
        {
        case N_ENSYM:
        case N_BNSYM:
        case N_GSYM:
          sym = NULL;
          break;
        case N_FUN:
          if (sym->name == NULL || sym->name[0] == 0)
            sym = NULL;
          break;
        case N_STSYM:
          break;
        default:
          sym = NULL;
          break;
        }
      if (sym != NULL)
        {
          struct macho_sym_hash_entry *ent;

          ent = (struct macho_sym_hash_entry *)
            bfd_hash_lookup (&table, sym->name, TRUE, FALSE);
          if (ent->sym != NULL)
            complaint (&symfile_complaints,
                       _("Duplicated symbol %s in symbol table"), sym->name);
          else
            {
              if (mach_o_debug_level > 4)
                {
                  struct gdbarch *arch = get_objfile_arch (main_objfile);
                  printf_unfiltered
                    (_("Adding symbol %s (addr: %s)\n"),
                     sym->name, paddress (arch, sym->value));
                }
              ent->sym = sym;
            }
        }
    }

  /* Relocate symbols of the OSO.  */
  for (i = 0; symbol_table[i]; i++)
    {
      asymbol *sym = symbol_table[i];
      bfd_mach_o_asymbol *mach_o_sym = (bfd_mach_o_asymbol *)sym;

      if (mach_o_sym->n_type & BFD_MACH_O_N_STAB)
        continue;
      if ((mach_o_sym->n_type & BFD_MACH_O_N_TYPE) == BFD_MACH_O_N_UNDF
           && sym->value != 0)
        {
          /* For common symbol use the min symtab and modify the OSO
             symbol table.  */
          CORE_ADDR res;

          res = macho_resolve_oso_sym_with_minsym (main_objfile, sym);
          if (res != 0)
            {
              sym->section = bfd_com_section_ptr;
              sym->value = res;
            }
        }
      else if ((mach_o_sym->n_type & BFD_MACH_O_N_TYPE) == BFD_MACH_O_N_SECT)
        {
          /* Normal symbol.  */
          asection *sec = sym->section;
          bfd_mach_o_section *msec;
          unsigned int sec_type;

          /* Skip buggy ones.  */
          if (sec == NULL || sections_rebased[sec->index] != 0)
            continue;

          /* Only consider regular, non-debugging sections.  */
          msec = bfd_mach_o_get_mach_o_section (sec);
          sec_type = msec->flags & BFD_MACH_O_SECTION_TYPE_MASK;
          if ((sec_type == BFD_MACH_O_S_REGULAR
               || sec_type == BFD_MACH_O_S_ZEROFILL)
              && (msec->flags & BFD_MACH_O_S_ATTR_DEBUG) == 0)
            {
              CORE_ADDR addr = 0;

              if ((mach_o_sym->n_type & BFD_MACH_O_N_EXT) != 0)
                {
                  /* Use the min symtab for global symbols.  */
                  addr = macho_resolve_oso_sym_with_minsym (main_objfile, sym);
                }
              else
                {
                  struct macho_sym_hash_entry *ent;

                  ent = (struct macho_sym_hash_entry *)
                    bfd_hash_lookup (&table, sym->name, FALSE, FALSE);
                  if (ent != NULL)
                    addr = bfd_asymbol_value (ent->sym);
                }

              /* Adjust the section.  */
              if (addr != 0)
                {
                  CORE_ADDR res = addr - sym->value;

                  if (mach_o_debug_level > 3)
                    {
                      struct gdbarch *arch = get_objfile_arch (main_objfile);
                      printf_unfiltered
                        (_("resolve sect %s with %s (set to %s)\n"),
                         sec->name, sym->name,
                         paddress (arch, res));
                    }
                  bfd_set_section_vma (abfd, sec, res);
                  sections_rebased[sec->index] = 1;
                }
            }
          else
            {
              /* Mark the section as never rebased.  */
              sections_rebased[sec->index] = 2;
            }
        }
    }

  bfd_hash_table_free (&table);

  /* We need to clear SYMFILE_MAINLINE to avoid interractive question
     from symfile.c:symbol_file_add_with_addrs_or_offsets.  */
  cleanup = make_cleanup_bfd_unref (abfd);
  symbol_file_add_from_bfd
    (abfd, name, symfile_flags & ~(SYMFILE_MAINLINE | SYMFILE_VERBOSE), NULL,
     main_objfile->flags & (OBJF_REORDERED | OBJF_SHARED
			    | OBJF_READNOW | OBJF_USERLOADED),
     main_objfile);
  do_cleanups (cleanup);
}

/* Read symbols from the vector of oso files.

   Note that this function sorts OSO_VECTOR_PTR.  */

static void
macho_symfile_read_all_oso (VEC (oso_el) **oso_vector_ptr,
			    struct objfile *main_objfile,
			    int symfile_flags)
{
  int ix;
  VEC (oso_el) *vec = *oso_vector_ptr;
  oso_el *oso;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);

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

	  make_cleanup (xfree, archive_name);

          /* Compute number of oso for this archive.  */
          for (last_ix = ix;
               VEC_iterate (oso_el, vec, last_ix, oso2); last_ix++)
            {
              if (strncmp (oso2->name, archive_name, pfx_len) != 0)
                break;
            }

	  /* Open the archive and check the format.  */
	  archive_bfd = gdb_bfd_open (archive_name, gnutarget, -1);
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
	      gdb_bfd_unref (archive_bfd);
              ix = last_ix;
	      continue;
	    }

	  member_bfd = gdb_bfd_openr_next_archived_file (archive_bfd, NULL);

	  if (member_bfd == NULL)
	    {
	      warning (_("Could not read archive members out of "
			 "OSO archive \"%s\""), archive_name);
	      gdb_bfd_unref (archive_bfd);
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
					     bfd_get_filename (member_bfd),
                                             main_objfile, symfile_flags);
                      oso2->name = NULL;
                      break;
                    }
                }

              prev = member_bfd;
	      member_bfd = gdb_bfd_openr_next_archived_file (archive_bfd,
							     member_bfd);

              /* Free previous member if not referenced by an oso.  */
              if (ix2 >= last_ix)
                gdb_bfd_unref (prev);
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

	  abfd = gdb_bfd_open (oso->name, gnutarget, -1);
	  if (!abfd)
            warning (_("`%s': can't open to read symbols: %s."), oso->name,
                     bfd_errmsg (bfd_get_error ()));
          else
            macho_add_oso_symfile (oso, abfd, oso->name, main_objfile,
				   symfile_flags);

          ix++;
        }
    }

  do_cleanups (cleanup);
}

/* DSYM (debug symbols) files contain the debug info of an executable.
   This is a separate file created by dsymutil(1) and is similar to debug
   link feature on ELF.
   DSYM files are located in a subdirectory.  Append DSYM_SUFFIX to the
   executable name and the executable base name to get the DSYM file name.  */
#define DSYM_SUFFIX ".dSYM/Contents/Resources/DWARF/"

/* Check if a dsym file exists for OBJFILE.  If so, returns a bfd for it
   and return *FILENAMEP with its original xmalloc-ated filename.
   Return NULL if no valid dsym file is found (FILENAMEP is not used in
   such case).  */

static bfd *
macho_check_dsym (struct objfile *objfile, char **filenamep)
{
  size_t name_len = strlen (objfile_name (objfile));
  size_t dsym_len = strlen (DSYM_SUFFIX);
  const char *base_name = lbasename (objfile_name (objfile));
  size_t base_len = strlen (base_name);
  char *dsym_filename = (char *) alloca (name_len + dsym_len + base_len + 1);
  bfd *dsym_bfd;
  bfd_mach_o_load_command *main_uuid;
  bfd_mach_o_load_command *dsym_uuid;

  strcpy (dsym_filename, objfile_name (objfile));
  strcpy (dsym_filename + name_len, DSYM_SUFFIX);
  strcpy (dsym_filename + name_len + dsym_len, base_name);

  if (access (dsym_filename, R_OK) != 0)
    return NULL;

  if (bfd_mach_o_lookup_command (objfile->obfd,
                                 BFD_MACH_O_LC_UUID, &main_uuid) == 0)
    {
      warning (_("can't find UUID in %s"), objfile_name (objfile));
      return NULL;
    }
  dsym_bfd = gdb_bfd_openr (dsym_filename, gnutarget);
  if (dsym_bfd == NULL)
    {
      warning (_("can't open dsym file %s"), dsym_filename);
      return NULL;
    }

  if (!bfd_check_format (dsym_bfd, bfd_object))
    {
      gdb_bfd_unref (dsym_bfd);
      warning (_("bad dsym file format: %s"), bfd_errmsg (bfd_get_error ()));
      return NULL;
    }

  if (bfd_mach_o_lookup_command (dsym_bfd,
                                 BFD_MACH_O_LC_UUID, &dsym_uuid) == 0)
    {
      warning (_("can't find UUID in %s"), dsym_filename);
      gdb_bfd_unref (dsym_bfd);
      return NULL;
    }
  if (memcmp (dsym_uuid->command.uuid.uuid, main_uuid->command.uuid.uuid,
              sizeof (main_uuid->command.uuid.uuid)))
    {
      warning (_("dsym file UUID doesn't match the one in %s"),
	       objfile_name (objfile));
      gdb_bfd_unref (dsym_bfd);
      return NULL;
    }
  *filenamep = xstrdup (dsym_filename);
  return dsym_bfd;
}

static void
macho_symfile_read (struct objfile *objfile, int symfile_flags)
{
  bfd *abfd = objfile->obfd;
  long storage_needed;
  bfd *dsym_bfd;
  VEC (oso_el) *oso_vector = NULL;
  struct cleanup *old_chain = make_cleanup (VEC_cleanup (oso_el), &oso_vector);

  /* Get symbols from the symbol table only if the file is an executable.
     The symbol table of object files is not relocated and is expected to
     be in the executable.  */
  if (bfd_get_file_flags (abfd) & (EXEC_P | DYNAMIC))
    {
      char *dsym_filename;

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

          init_minimal_symbol_collection ();
          make_cleanup_discard_minimal_symbols ();

	  symcount = bfd_canonicalize_symtab (objfile->obfd, symbol_table);

	  if (symcount < 0)
	    error (_("Can't read symbols from %s: %s"),
		   bfd_get_filename (objfile->obfd),
		   bfd_errmsg (bfd_get_error ()));

	  macho_symtab_read (objfile, symcount, symbol_table, &oso_vector);

          install_minimal_symbols (objfile);
	}

      /* Try to read .eh_frame / .debug_frame.  */
      /* First, locate these sections.  We ignore the result status
	 as it only checks for debug info.  */
      dwarf2_has_info (objfile, NULL);
      dwarf2_build_frame_info (objfile);

      /* Check for DSYM file.  */
      dsym_bfd = macho_check_dsym (objfile, &dsym_filename);
      if (dsym_bfd != NULL)
	{
          struct bfd_section *asect, *dsect;

	  make_cleanup (xfree, dsym_filename);

	  if (mach_o_debug_level > 0)
	    printf_unfiltered (_("dsym file found\n"));

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
	  make_cleanup_bfd_unref (dsym_bfd);
          symbol_file_add_separate (dsym_bfd, dsym_filename, symfile_flags,
				    objfile);

	  /* Don't try to read dwarf2 from main file or shared libraries.  */
	  do_cleanups (old_chain);
          return;
	}
    }

  if (dwarf2_has_info (objfile, NULL))
    {
      /* DWARF 2 sections */
      dwarf2_build_psymtabs (objfile);
    }

  /* Then the oso.  */
  if (oso_vector != NULL)
    macho_symfile_read_all_oso (&oso_vector, objfile, symfile_flags);

  do_cleanups (old_chain);
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
                       sectp->name, objfile_name (objfile));

  return bfd_simple_get_relocated_section_contents (abfd, sectp, buf, NULL);
}

static void
macho_symfile_finish (struct objfile *objfile)
{
}

static void
macho_symfile_offsets (struct objfile *objfile,
                       const struct section_addr_info *addrs)
{
  unsigned int i;
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
      int sect_index = osect - objfile->sections;;

      if (startswith (bfd_sect_name, "LC_SEGMENT."))
	bfd_sect_name += 11;
      if (strcmp (bfd_sect_name, "__TEXT") == 0
	  || strcmp (bfd_sect_name, "__TEXT.__text") == 0)
	objfile->sect_index_text = sect_index;
    }
}

static const struct sym_fns macho_sym_fns = {
  macho_new_init,               /* init anything gbl to entire symtab */
  macho_symfile_init,           /* read initial info, setup for sym_read() */
  macho_symfile_read,           /* read a symbol file into symtab */
  NULL,				/* sym_read_psymbols */
  macho_symfile_finish,         /* finished with file, cleanup */
  macho_symfile_offsets,        /* xlate external to internal form */
  default_symfile_segments,	/* Get segment information from a file.  */
  NULL,
  macho_symfile_relocate,	/* Relocate a debug section.  */
  NULL,				/* sym_get_probes */
  &psym_functions
};

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_machoread;

void
_initialize_machoread (void)
{
  add_symtab_fns (bfd_target_mach_o_flavour, &macho_sym_fns);

  add_setshow_zuinteger_cmd ("mach-o", class_obscure,
			     &mach_o_debug_level,
			     _("Set if printing Mach-O symbols processing."),
			     _("Show if printing Mach-O symbols processing."),
			     NULL, NULL, NULL,
			     &setdebuglist, &showdebuglist);
}
