/* DWARF CU data structure

   Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

#include "dwarf2/cu.h"
#include "dwarf2/read.h"
#include "objfiles.h"
#include "filenames.h"
#include "producer.h"
#include "gdbsupport/pathstuff.h"

/* Initialize dwarf2_cu to read PER_CU, in the context of PER_OBJFILE.  */

dwarf2_cu::dwarf2_cu (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile)
  : per_cu (per_cu),
    per_objfile (per_objfile),
    m_mark (false),
    has_loclist (false),
    m_checked_producer (false),
    m_producer_is_gxx_lt_4_6 (false),
    m_producer_is_gcc_lt_4_5 (false),
    m_producer_is_gcc_lt_4_3 (false),
    m_producer_is_gcc_ge_4 (false),
    m_producer_is_gcc_11 (false),
    m_producer_is_gcc (false),
    m_producer_is_icc (false),
    m_producer_is_icc_lt_14 (false),
    m_producer_is_codewarrior (false),
    m_producer_is_clang (false),
    m_producer_is_gas_lt_2_38 (false),
    m_producer_is_gas_2_39 (false),
    m_producer_is_gas_ge_2_40 (false),
    m_producer_is_realview (false),
    m_producer_is_xlc (false),
    m_producer_is_xlc_opencl (false),
    m_producer_is_gf77 (false),
    m_producer_is_ggo (false),
    processing_has_namespace_info (false)
{
}

/* See cu.h.  */

struct type *
dwarf2_cu::addr_sized_int_type (bool unsigned_p) const
{
  return objfile_int_type (this->per_objfile->objfile, this->header.addr_size,
			   unsigned_p);
}

/* Start a symtab for DWARF.  NAME, COMP_DIR, LOW_PC are passed to the
   buildsym_compunit constructor.  */

struct compunit_symtab *
dwarf2_cu::start_compunit_symtab (const char *name, const char *comp_dir,
				  CORE_ADDR low_pc)
{
  gdb_assert (m_builder == nullptr);

  std::string name_for_id_holder;
  const char *name_for_id = name;

  /* Prepend the compilation directory to the filename if needed (if not
     absolute already) to get the "name for id" for our main symtab.  The name
     for the main file coming from the line table header will be generated using
     the same logic, so will hopefully match what we pass here.  */
  if (!IS_ABSOLUTE_PATH (name) && comp_dir != nullptr)
    {
      name_for_id_holder = path_join (comp_dir, name);
      name_for_id = name_for_id_holder.c_str ();
    }

  m_builder = std::make_unique<buildsym_compunit> (this->per_objfile->objfile,
						   name,
						   comp_dir,
						   name_for_id,
						   lang (),
						   low_pc);

  list_in_scope = get_builder ()->get_file_symbols ();

  /* DWARF versions are restricted to [2, 5], thanks to the check in
     read_comp_unit_head.  */
  gdb_assert (this->header.version >= 2 && this->header.version <= 5);
  static const char *debugformat_strings[] = {
    "DWARF 2",
    "DWARF 3",
    "DWARF 4",
    "DWARF 5",
  };
  const char *debugformat = debugformat_strings[this->header.version - 2];

  get_builder ()->record_debugformat (debugformat);
  get_builder ()->record_producer (m_producer);

  processing_has_namespace_info = false;

  return get_builder ()->get_compunit_symtab ();
}

/* See read.h.  */

struct type *
dwarf2_cu::addr_type () const
{
  struct objfile *objfile = this->per_objfile->objfile;
  struct type *void_type = builtin_type (objfile)->builtin_void;
  struct type *addr_type = lookup_pointer_type (void_type);

  if (addr_type->length () == this->header.addr_size)
    return addr_type;

  addr_type = addr_sized_int_type (addr_type->is_unsigned ());
  return addr_type;
}

/* See dwarf2/cu.h.  */

void
dwarf2_cu::mark ()
{
  if (m_mark)
    return;

  m_mark = true;

  for (dwarf2_per_cu *per_cu : m_dependencies)
    {
      /* cu->m_dependencies references may not yet have been ever
	 read if QUIT aborts reading of the chain.  As such
	 dependencies remain valid it is not much useful to track
	 and undo them during QUIT cleanups.  */
      dwarf2_cu *cu = per_objfile->get_cu (per_cu);

      if (cu == nullptr)
	continue;

      cu->mark ();
    }
}

/* See dwarf2/cu.h.  */

buildsym_compunit *
dwarf2_cu::get_builder ()
{
  /* If this CU has a builder associated with it, use that.  */
  if (m_builder != nullptr)
    return m_builder.get ();

  if (per_objfile->sym_cu != nullptr)
    return per_objfile->sym_cu->m_builder.get ();

  gdb_assert_not_reached ("");
}

/* See dwarf2/cu.h.  */

void
dwarf2_cu::set_producer (const char *producer)
{
  gdb_assert (m_producer == nullptr || strcmp (producer, m_producer) == 0);
  m_producer = producer;

  int major, minor;

  if (m_producer == nullptr)
    {
      /* For unknown compilers expect their behavior is DWARF version
	 compliant.

	 GCC started to support .debug_types sections by -gdwarf-4 since
	 gcc-4.5.x.  As the .debug_types sections are missing DW_AT_producer
	 for their space efficiency GDB cannot workaround gcc-4.5.x -gdwarf-4
	 combination.  gcc-4.5.x -gdwarf-4 binaries have DW_AT_accessibility
	 interpreted incorrectly by GDB now - GCC PR debug/48229.  */
    }
  else if (::producer_is_gcc (m_producer, &major, &minor))
    {
      m_producer_is_gxx_lt_4_6 = major < 4 || (major == 4 && minor < 6);
      m_producer_is_gcc_lt_4_5 = major < 4 || (major == 4 && minor < 5);
      m_producer_is_gcc_lt_4_3 = major < 4 || (major == 4 && minor < 3);
      m_producer_is_gcc_ge_4 = major >= 4;
      m_producer_is_gcc_11 = major == 11;
      m_producer_is_gcc = true;
    }
  else if (::producer_is_icc (m_producer, &major, &minor))
    {
      m_producer_is_icc = true;
      m_producer_is_icc_lt_14 = major < 14;
    }
  else if (startswith (m_producer, "CodeWarrior S12/L-ISA"))
    m_producer_is_codewarrior = true;
  else if (::producer_is_clang (m_producer, &major, &minor))
    m_producer_is_clang = true;
  else if (::producer_is_gas (m_producer, &major, &minor))
    {
      m_producer_is_gas_lt_2_38 = major < 2 || (major == 2 && minor < 38);
      m_producer_is_gas_2_39 = major == 2 && minor == 39;
      m_producer_is_gas_ge_2_40 = major > 2 || (major == 2 && minor >= 40);
    }
  else if (::producer_is_realview (m_producer))
    m_producer_is_realview = true;
  else if (startswith (m_producer, "IBM(R) XL C/C++ Advanced Edition"))
    m_producer_is_xlc = true;
  else if (strstr (m_producer, "IBM XL C for OpenCL") != nullptr)
    m_producer_is_xlc_opencl = true;
  else
    {
      /* For other non-GCC compilers, expect their behavior is DWARF version
	 compliant.  */
    }

  if (m_producer != nullptr)
    {
      if (strstr (m_producer, "GNU F77") != nullptr)
	m_producer_is_gf77 = true;
      else if (strstr (m_producer, "GNU Go ") != nullptr)
	m_producer_is_ggo = true;
    }

  m_checked_producer = true;
}
