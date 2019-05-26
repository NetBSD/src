/* Read the export table symbols from a portable executable and
   convert to internal format, for GDB. Used as a last resort if no
   debugging symbols recognized.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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

   Contributed by Raoul M. Gough (RaoulGough@yahoo.co.uk).  */

#include "defs.h"

#include "coff-pe-read.h"

#include "bfd.h"
#include "gdbtypes.h"

#include "command.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "common/common-utils.h"
#include "coff/internal.h"

#include <ctype.h>

/* Internal section information */

/* Coff PE read debugging flag:
   default value is 0,
   value 1 outputs problems encountered while parsing PE file,
   value above 1 also lists all generated minimal symbols.  */
static unsigned int debug_coff_pe_read;

struct read_pe_section_data
{
  CORE_ADDR vma_offset;		/* Offset to loaded address of section.  */
  unsigned long rva_start;	/* Start offset within the pe.  */
  unsigned long rva_end;	/* End offset within the pe.  */
  enum minimal_symbol_type ms_type;	/* Type to assign symbols in
					   section.  */
  unsigned int index;		/* BFD section number.  */
  const char *section_name;	/* Recorded section name.  */
};

#define IMAGE_SCN_CNT_CODE 0x20
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x40
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x80
#define PE_SECTION_INDEX_TEXT     0
#define PE_SECTION_INDEX_DATA     1
#define PE_SECTION_INDEX_BSS      2
#define PE_SECTION_TABLE_SIZE     3
#define PE_SECTION_INDEX_INVALID -1

/* Get the index of the named section in our own array, which contains
   text, data and bss in that order.  Return PE_SECTION_INDEX_INVALID
   if passed an unrecognised section name.  */

static int
read_pe_section_index (const char *section_name)
{
  if (strcmp (section_name, ".text") == 0)
    {
      return PE_SECTION_INDEX_TEXT;
    }

  else if (strcmp (section_name, ".data") == 0)
    {
      return PE_SECTION_INDEX_DATA;
    }

  else if (strcmp (section_name, ".bss") == 0)
    {
      return PE_SECTION_INDEX_BSS;
    }

  else
    {
      return PE_SECTION_INDEX_INVALID;
    }
}

/* Get the index of the named section in our own full array.
   text, data and bss in that order.  Return PE_SECTION_INDEX_INVALID
   if passed an unrecognised section name.  */

static int
get_pe_section_index (const char *section_name,
		      struct read_pe_section_data *sections,
		      int nb_sections)
{
  int i;

  for (i = 0; i < nb_sections; i++)
    if (strcmp (sections[i].section_name, section_name) == 0)
      return i;
  return PE_SECTION_INDEX_INVALID;
}

/* Structure used by get_section_vmas function below
   to access section_data array and the size of the array
   stored in nb_sections field.  */
struct pe_sections_info
{
  int nb_sections;
  struct read_pe_section_data *sections;
};

/* Record the virtual memory address of a section.  */

static void
get_section_vmas (bfd *abfd, asection *sectp, void *context)
{
  struct pe_sections_info *data = (struct pe_sections_info *) context;
  struct read_pe_section_data *sections = data->sections;
  int sectix = get_pe_section_index (sectp->name, sections,
				     data->nb_sections);

  if (sectix != PE_SECTION_INDEX_INVALID)
    {
      /* Data within the section start at rva_start in the pe and at
         bfd_get_section_vma() within memory.  Store the offset.  */

      sections[sectix].vma_offset
	= bfd_get_section_vma (abfd, sectp) - sections[sectix].rva_start;
    }
}

/* Create a minimal symbol entry for an exported symbol.
   SYM_NAME contains the exported name or NULL if exported by ordinal,
   FUNC_RVA contains the Relative Virtual Address of the symbol,
   ORDINAL is the ordinal index value of the symbol,
   SECTION_DATA contains information about the section in which the
   symbol is declared,
   DLL_NAME is the internal name of the DLL file,
   OBJFILE is the objfile struct of DLL_NAME.  */

static void
add_pe_exported_sym (minimal_symbol_reader &reader,
		     const char *sym_name,
		     unsigned long func_rva,
		     int ordinal,
		     const struct read_pe_section_data *section_data,
		     const char *dll_name, struct objfile *objfile)
{
  char *qualified_name, *bare_name;
  /* Add the stored offset to get the loaded address of the symbol.  */
  CORE_ADDR vma = func_rva + section_data->vma_offset;

  /* Generate a (hopefully unique) qualified name using the first part
     of the dll name, e.g. KERNEL32!AddAtomA.  This matches the style
     used by windbg from the "Microsoft Debugging Tools for Windows".  */

  if (sym_name == NULL || *sym_name == '\0')
    bare_name = xstrprintf ("#%d", ordinal);
  else
    bare_name = xstrdup (sym_name);

  qualified_name = xstrprintf ("%s!%s", dll_name, bare_name);

  if ((section_data->ms_type == mst_unknown) && debug_coff_pe_read)
    fprintf_unfiltered (gdb_stdlog , _("Unknown section type for \"%s\""
			" for entry \"%s\" in dll \"%s\"\n"),
			section_data->section_name, sym_name, dll_name);

  reader.record_with_info (qualified_name, vma, section_data->ms_type,
			   section_data->index);

  /* Enter the plain name as well, which might not be unique.  */
  reader.record_with_info (bare_name, vma, section_data->ms_type,
			   section_data->index);
  if (debug_coff_pe_read > 1)
    fprintf_unfiltered (gdb_stdlog, _("Adding exported symbol \"%s\""
			" in dll \"%s\"\n"), sym_name, dll_name);
  xfree (qualified_name);
  xfree (bare_name);
}

/* Create a minimal symbol entry for an exported forward symbol.
   Return 1 if the forwarded function was found 0 otherwise.
   SYM_NAME contains the exported name or NULL if exported by ordinal,
   FORWARD_DLL_NAME is the name of the DLL in which the target symobl resides,
   FORWARD_FUNC_NAME is the name of the target symbol in that DLL,
   ORDINAL is the ordinal index value of the symbol,
   DLL_NAME is the internal name of the DLL file,
   OBJFILE is the objfile struct of DLL_NAME.  */

static int
add_pe_forwarded_sym (minimal_symbol_reader &reader,
		      const char *sym_name, const char *forward_dll_name,
		      const char *forward_func_name, int ordinal,
		      const char *dll_name, struct objfile *objfile)
{
  CORE_ADDR vma, baseaddr;
  struct bound_minimal_symbol msymbol;
  enum minimal_symbol_type msymtype;
  char *qualified_name, *bare_name;
  int forward_dll_name_len = strlen (forward_dll_name);
  int forward_func_name_len = strlen (forward_func_name);
  int forward_len = forward_dll_name_len + forward_func_name_len + 2;
  char *forward_qualified_name = (char *) alloca (forward_len);
  short section;

  xsnprintf (forward_qualified_name, forward_len, "%s!%s", forward_dll_name,
	     forward_func_name);


  msymbol = lookup_minimal_symbol_and_objfile (forward_qualified_name);

  if (!msymbol.minsym)
    {
      int i;

      for (i = 0; i < forward_dll_name_len; i++)
	forward_qualified_name[i] = tolower (forward_qualified_name[i]);
      msymbol = lookup_minimal_symbol_and_objfile (forward_qualified_name);
    }

  if (!msymbol.minsym)
    {
      if (debug_coff_pe_read)
	fprintf_unfiltered (gdb_stdlog, _("Unable to find function \"%s\" in"
			    " dll \"%s\", forward of \"%s\" in dll \"%s\"\n"),
			    forward_func_name, forward_dll_name, sym_name,
			    dll_name);
      return 0;
    }

  if (debug_coff_pe_read > 1)
    fprintf_unfiltered (gdb_stdlog, _("Adding forwarded exported symbol"
			" \"%s\" in dll \"%s\", pointing to \"%s\"\n"),
			sym_name, dll_name, forward_qualified_name);

  vma = BMSYMBOL_VALUE_ADDRESS (msymbol);
  msymtype = MSYMBOL_TYPE (msymbol.minsym);
  section = MSYMBOL_SECTION (msymbol.minsym);

  /* Generate a (hopefully unique) qualified name using the first part
     of the dll name, e.g. KERNEL32!AddAtomA.  This matches the style
     used by windbg from the "Microsoft Debugging Tools for Windows".  */

  if (sym_name == NULL || *sym_name == '\0')
    bare_name = xstrprintf ("#%d", ordinal);
  else
    bare_name = xstrdup (sym_name);

  qualified_name = xstrprintf ("%s!%s", dll_name, bare_name);

  /* Note that this code makes a minimal symbol whose value may point
     outside of any section in this objfile.  These symbols can't
     really be relocated properly, but nevertheless we make a stab at
     it, choosing an approach consistent with the history of this
     code.  */
  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  reader.record_with_info (qualified_name, vma - baseaddr, msymtype, section);

  /* Enter the plain name as well, which might not be unique.  */
  reader.record_with_info (bare_name, vma - baseaddr, msymtype, section);
  xfree (qualified_name);
  xfree (bare_name);

  return 1;
}

/* Truncate a dll_name at the last dot character.  */

static void
read_pe_truncate_name (char *dll_name)
{
  char *last_point = strrchr (dll_name, '.');

  if (last_point != NULL)
    *last_point = '\0';
}

/* Low-level support functions, direct from the ld module pe-dll.c.  */
static unsigned int
pe_get16 (bfd *abfd, int where)
{
  unsigned char b[2];

  bfd_seek (abfd, (file_ptr) where, SEEK_SET);
  bfd_bread (b, (bfd_size_type) 2, abfd);
  return b[0] + (b[1] << 8);
}

static unsigned int
pe_get32 (bfd *abfd, int where)
{
  unsigned char b[4];

  bfd_seek (abfd, (file_ptr) where, SEEK_SET);
  bfd_bread (b, (bfd_size_type) 4, abfd);
  return b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24);
}

static unsigned int
pe_as16 (void *ptr)
{
  unsigned char *b = (unsigned char *) ptr;

  return b[0] + (b[1] << 8);
}

static unsigned int
pe_as32 (void *ptr)
{
  unsigned char *b = (unsigned char *) ptr;

  return b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24);
}

/* Read the (non-debug) export symbol table from a portable
   executable.  Code originally lifted from the ld function
   pe_implied_import_dll in pe-dll.c.  */

void
read_pe_exported_syms (minimal_symbol_reader &reader,
		       struct objfile *objfile)
{
  bfd *dll = objfile->obfd;
  unsigned long nbnormal, nbforward;
  unsigned long pe_header_offset, opthdr_ofs, num_entries, i;
  unsigned long export_opthdrrva, export_opthdrsize;
  unsigned long export_rva, export_size, nsections, secptr, expptr;
  unsigned long exp_funcbase;
  unsigned char *expdata, *erva;
  unsigned long name_rvas, ordinals, nexp, ordbase;
  char *dll_name = (char *) dll->filename;
  int otherix = PE_SECTION_TABLE_SIZE;
  int is_pe64 = 0;
  int is_pe32 = 0;

  /* Array elements are for text, data and bss in that order
     Initialization with RVA_START > RVA_END guarantees that
     unused sections won't be matched.  */
  struct read_pe_section_data *section_data;
  struct pe_sections_info pe_sections_info;

  struct cleanup *back_to = make_cleanup (null_cleanup, 0);

  char const *target = bfd_get_target (objfile->obfd);

  section_data = XCNEWVEC (struct read_pe_section_data, PE_SECTION_TABLE_SIZE);

  make_cleanup (free_current_contents, &section_data);

  for (i=0; i < PE_SECTION_TABLE_SIZE; i++)
    {
      section_data[i].vma_offset = 0;
      section_data[i].rva_start = 1;
      section_data[i].rva_end = 0;
    };
  section_data[PE_SECTION_INDEX_TEXT].ms_type = mst_text;
  section_data[PE_SECTION_INDEX_TEXT].section_name = ".text";
  section_data[PE_SECTION_INDEX_DATA].ms_type = mst_data;
  section_data[PE_SECTION_INDEX_DATA].section_name = ".data";
  section_data[PE_SECTION_INDEX_BSS].ms_type = mst_bss;
  section_data[PE_SECTION_INDEX_BSS].section_name = ".bss";

  is_pe64 = (strcmp (target, "pe-x86-64") == 0
	     || strcmp (target, "pei-x86-64") == 0);
  is_pe32 = (strcmp (target, "pe-i386") == 0
	     || strcmp (target, "pei-i386") == 0
	     || strcmp (target, "pe-arm-wince-little") == 0
	     || strcmp (target, "pei-arm-wince-little") == 0);
  if (!is_pe32 && !is_pe64)
    {
      /* This is not a recognized PE format file.  Abort now, because
	 the code is untested on anything else.  *FIXME* test on
	 further architectures and loosen or remove this test.  */
      do_cleanups (back_to);
      return;
    }

  /* Get pe_header, optional header and numbers of export entries.  */
  pe_header_offset = pe_get32 (dll, 0x3c);
  opthdr_ofs = pe_header_offset + 4 + 20;
  if (is_pe64)
    num_entries = pe_get32 (dll, opthdr_ofs + 108);
  else
    num_entries = pe_get32 (dll, opthdr_ofs + 92);

  if (num_entries < 1)		/* No exports.  */
    {
      do_cleanups (back_to);
      return;
    }
  if (is_pe64)
    {
      export_opthdrrva = pe_get32 (dll, opthdr_ofs + 112);
      export_opthdrsize = pe_get32 (dll, opthdr_ofs + 116);
    }
  else
    {
      export_opthdrrva = pe_get32 (dll, opthdr_ofs + 96);
      export_opthdrsize = pe_get32 (dll, opthdr_ofs + 100);
    }
  nsections = pe_get16 (dll, pe_header_offset + 4 + 2);
  secptr = (pe_header_offset + 4 + 20 +
	    pe_get16 (dll, pe_header_offset + 4 + 16));
  expptr = 0;
  export_size = 0;

  /* Get the rva and size of the export section.  */
  for (i = 0; i < nsections; i++)
    {
      char sname[8];
      unsigned long secptr1 = secptr + 40 * i;
      unsigned long vaddr = pe_get32 (dll, secptr1 + 12);
      unsigned long vsize = pe_get32 (dll, secptr1 + 16);
      unsigned long fptr = pe_get32 (dll, secptr1 + 20);

      bfd_seek (dll, (file_ptr) secptr1, SEEK_SET);
      bfd_bread (sname, (bfd_size_type) sizeof (sname), dll);

      if ((strcmp (sname, ".edata") == 0)
	  || (vaddr <= export_opthdrrva && export_opthdrrva < vaddr + vsize))
	{
	  if (strcmp (sname, ".edata") != 0)
	    {
	      if (debug_coff_pe_read)
		fprintf_unfiltered (gdb_stdlog, _("Export RVA for dll "
				    "\"%s\" is in section \"%s\"\n"),
				    dll_name, sname);
	    }
	  else if (export_opthdrrva != vaddr && debug_coff_pe_read)
	    fprintf_unfiltered (gdb_stdlog, _("Wrong value of export RVA"
				" for dll \"%s\": 0x%lx instead of 0x%lx\n"),
				dll_name, export_opthdrrva, vaddr);
	  expptr = fptr + (export_opthdrrva - vaddr);
	  break;
	}
    }

  export_rva = export_opthdrrva;
  export_size = export_opthdrsize;

  if (export_size == 0)
    {
      /* Empty export table.  */
      do_cleanups (back_to);
      return;
    }

  /* Scan sections and store the base and size of the relevant
     sections.  */
  for (i = 0; i < nsections; i++)
    {
      unsigned long secptr1 = secptr + 40 * i;
      unsigned long vsize = pe_get32 (dll, secptr1 + 8);
      unsigned long vaddr = pe_get32 (dll, secptr1 + 12);
      unsigned long characteristics = pe_get32 (dll, secptr1 + 36);
      char sec_name[SCNNMLEN + 1];
      int sectix;
      unsigned int bfd_section_index;
      asection *section;

      bfd_seek (dll, (file_ptr) secptr1 + 0, SEEK_SET);
      bfd_bread (sec_name, (bfd_size_type) SCNNMLEN, dll);
      sec_name[SCNNMLEN] = '\0';

      sectix = read_pe_section_index (sec_name);
      section = bfd_get_section_by_name (dll, sec_name);
      if (section)
	bfd_section_index = section->index;
      else
	bfd_section_index = -1;

      if (sectix != PE_SECTION_INDEX_INVALID)
	{
	  section_data[sectix].rva_start = vaddr;
	  section_data[sectix].rva_end = vaddr + vsize;
	  section_data[sectix].index = bfd_section_index;
	}
      else
	{
	  char *name;

	  section_data = XRESIZEVEC (struct read_pe_section_data, section_data,
				     otherix + 1);
	  name = xstrdup (sec_name);
	  section_data[otherix].section_name = name;
	  make_cleanup (xfree, name);
	  section_data[otherix].rva_start = vaddr;
	  section_data[otherix].rva_end = vaddr + vsize;
	  section_data[otherix].vma_offset = 0;
	  section_data[otherix].index = bfd_section_index;
	  if (characteristics & IMAGE_SCN_CNT_CODE)
	    section_data[otherix].ms_type = mst_text;
	  else if (characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
	    section_data[otherix].ms_type = mst_data;
	  else if (characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	    section_data[otherix].ms_type = mst_bss;
	  else
	    section_data[otherix].ms_type = mst_unknown;
	  otherix++;
	}
    }

  expdata = (unsigned char *) xmalloc (export_size);
  make_cleanup (xfree, expdata);

  bfd_seek (dll, (file_ptr) expptr, SEEK_SET);
  bfd_bread (expdata, (bfd_size_type) export_size, dll);
  erva = expdata - export_rva;

  nexp = pe_as32 (expdata + 24);
  name_rvas = pe_as32 (expdata + 32);
  ordinals = pe_as32 (expdata + 36);
  ordbase = pe_as32 (expdata + 16);
  exp_funcbase = pe_as32 (expdata + 28);

  /* Use internal dll name instead of full pathname.  */
  dll_name = (char *) (pe_as32 (expdata + 12) + erva);

  pe_sections_info.nb_sections = otherix;
  pe_sections_info.sections = section_data;

  bfd_map_over_sections (dll, get_section_vmas, &pe_sections_info);

  /* Truncate name at first dot. Should maybe also convert to all
     lower case for convenience on Windows.  */
  read_pe_truncate_name (dll_name);

  if (debug_coff_pe_read)
    fprintf_unfiltered (gdb_stdlog, _("DLL \"%s\" has %ld export entries,"
			" base=%ld\n"), dll_name, nexp, ordbase);
  nbforward = 0;
  nbnormal = 0;
  /* Iterate through the list of symbols.  */
  for (i = 0; i < nexp; i++)
    {
      /* Pointer to the names vector.  */
      unsigned long name_rva = pe_as32 (erva + name_rvas + i * 4);
      /* Retrieve ordinal value.  */

      unsigned long ordinal = pe_as16 (erva + ordinals + i * 2);


      /* Pointer to the function address vector.  */
      /* This is relatived to ordinal value. */
      unsigned long func_rva = pe_as32 (erva + exp_funcbase +
                                        ordinal * 4);

      /* Find this symbol's section in our own array.  */
      int sectix = 0;
      int section_found = 0;

      /* First handle forward cases.  */
      if (func_rva >= export_rva && func_rva < export_rva + export_size)
	{
	  char *forward_name = (char *) (erva + func_rva);
	  char *funcname = (char *) (erva + name_rva);
	  char *forward_dll_name = forward_name;
	  char *forward_func_name = forward_name;
	  char *sep = strrchr (forward_name, '.');

	  if (sep)
	    {
	      int len = (int) (sep - forward_name);

	      forward_dll_name = (char *) alloca (len + 1);
	      strncpy (forward_dll_name, forward_name, len);
	      forward_dll_name[len] = '\0';
	      forward_func_name = ++sep;
	    }
	  if (add_pe_forwarded_sym (reader, funcname, forward_dll_name,
				    forward_func_name, ordinal,
				    dll_name, objfile) != 0)
	    ++nbforward;
	  continue;
	}

      for (sectix = 0; sectix < otherix; ++sectix)
	{
	  if ((func_rva >= section_data[sectix].rva_start)
	      && (func_rva < section_data[sectix].rva_end))
	    {
	      char *sym_name = (char *) (erva + name_rva);

	      section_found = 1;
	      add_pe_exported_sym (reader, sym_name, func_rva, ordinal,
				   section_data + sectix, dll_name, objfile);
	      ++nbnormal;
	      break;
	    }
	}
      if (!section_found)
	{
	  char *funcname = (char *) (erva + name_rva);

	  if (name_rva == 0)
	    {
	      add_pe_exported_sym (reader, NULL, func_rva, ordinal,
				   section_data, dll_name, objfile);
	      ++nbnormal;
	    }
	  else if (debug_coff_pe_read)
	    fprintf_unfiltered (gdb_stdlog, _("Export name \"%s\" ord. %lu,"
				" RVA 0x%lx in dll \"%s\" not handled\n"),
				funcname, ordinal, func_rva, dll_name);
	}
    }

  if (debug_coff_pe_read)
    fprintf_unfiltered (gdb_stdlog, _("Finished reading \"%s\", exports %ld,"
			" forwards %ld, total %ld/%ld.\n"), dll_name, nbnormal,
			nbforward, nbnormal + nbforward, nexp);
  /* Discard expdata and section_data.  */
  do_cleanups (back_to);
}

/* Extract from ABFD the offset of the .text section.
   This offset is mainly related to the offset within the file.
   The value was previously expected to be 0x1000 for all files,
   but some Windows OS core DLLs seem to use 0x10000 section alignement
   which modified the return value of that function.
   Still return default 0x1000 value if ABFD is NULL or
   if '.text' section is not found, but that should not happen...  */

#define DEFAULT_COFF_PE_TEXT_SECTION_OFFSET 0x1000

CORE_ADDR
pe_text_section_offset (struct bfd *abfd)

{
  unsigned long pe_header_offset, i;
  unsigned long nsections, secptr;
  int is_pe64 = 0;
  int is_pe32 = 0;
  char const *target;

  if (!abfd)
    return DEFAULT_COFF_PE_TEXT_SECTION_OFFSET;

  target = bfd_get_target (abfd);

  is_pe64 = (strcmp (target, "pe-x86-64") == 0
	     || strcmp (target, "pei-x86-64") == 0);
  is_pe32 = (strcmp (target, "pe-i386") == 0
	     || strcmp (target, "pei-i386") == 0
	     || strcmp (target, "pe-arm-wince-little") == 0
	     || strcmp (target, "pei-arm-wince-little") == 0);

  if (!is_pe32 && !is_pe64)
    {
      /* This is not a recognized PE format file.  Abort now, because
	 the code is untested on anything else.  *FIXME* test on
	 further architectures and loosen or remove this test.  */
      return DEFAULT_COFF_PE_TEXT_SECTION_OFFSET;
    }

  /* Get pe_header, optional header and numbers of sections.  */
  pe_header_offset = pe_get32 (abfd, 0x3c);
  nsections = pe_get16 (abfd, pe_header_offset + 4 + 2);
  secptr = (pe_header_offset + 4 + 20 +
	    pe_get16 (abfd, pe_header_offset + 4 + 16));

  /* Get the rva and size of the export section.  */
  for (i = 0; i < nsections; i++)
    {
      char sname[SCNNMLEN + 1];
      unsigned long secptr1 = secptr + 40 * i;
      unsigned long vaddr = pe_get32 (abfd, secptr1 + 12);

      bfd_seek (abfd, (file_ptr) secptr1, SEEK_SET);
      bfd_bread (sname, (bfd_size_type) SCNNMLEN, abfd);
      sname[SCNNMLEN] = '\0';
      if (strcmp (sname, ".text") == 0)
	return vaddr;
    }

  return DEFAULT_COFF_PE_TEXT_SECTION_OFFSET;
}

/* Implements "show debug coff_pe_read" command.  */

static void
show_debug_coff_pe_read (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Coff PE read debugging is %s.\n"), value);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */

void _initialize_coff_pe_read (void);

/* Adds "Set/show debug coff_pe_read" commands.  */

void
_initialize_coff_pe_read (void)
{
  add_setshow_zuinteger_cmd ("coff-pe-read", class_maintenance,
			     &debug_coff_pe_read,
			     _("Set coff PE read debugging."),
			     _("Show coff PE read debugging."),
			     _("When set, debugging messages for coff reading "
			       "of exported symbols are displayed."),
			     NULL, show_debug_coff_pe_read,
			     &setdebuglist, &showdebuglist);
}
