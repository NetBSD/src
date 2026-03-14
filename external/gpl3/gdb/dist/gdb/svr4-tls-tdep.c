/* Target-dependent code for GNU/Linux, architecture independent.

   Copyright (C) 2009-2025 Free Software Foundation, Inc.

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

#include "svr4-tls-tdep.h"
#include "solib-svr4.h"
#include "inferior.h"
#include "objfiles.h"
#include "cli/cli-cmds.h"
#include <optional>

struct svr4_tls_gdbarch_data
{
  /* Method for looking up TLS DTV.  */
  get_tls_dtv_addr_ftype *get_tls_dtv_addr = nullptr;

  /* Method for looking up the TLS DTP offset.  */
  get_tls_dtp_offset_ftype *get_tls_dtp_offset = nullptr;

  /* Cached libc value for TLS lookup purposes.  */
  enum svr4_tls_libc libc = svr4_tls_libc_unknown;
};

static const registry<gdbarch>::key<svr4_tls_gdbarch_data>
     svr4_tls_gdbarch_data_handle;

static struct svr4_tls_gdbarch_data *
get_svr4_tls_gdbarch_data (struct gdbarch *gdbarch)
{
  struct svr4_tls_gdbarch_data *result = svr4_tls_gdbarch_data_handle.get (gdbarch);
  if (result == nullptr)
    result = svr4_tls_gdbarch_data_handle.emplace (gdbarch);
  return result;
}

/* When true, force internal TLS address lookup instead of lookup via
   the thread stratum.  */

static bool force_internal_tls_address_lookup = false;

/* For TLS lookup purposes, use heuristics to decide whether program
   was linked against MUSL or GLIBC.  */

static enum svr4_tls_libc
libc_tls_sniffer (struct gdbarch *gdbarch)
{
  /* Check for cached libc value.  */
  svr4_tls_gdbarch_data *gdbarch_data = get_svr4_tls_gdbarch_data (gdbarch);
  if (gdbarch_data->libc != svr4_tls_libc_unknown)
    return gdbarch_data->libc;

  svr4_tls_libc libc = svr4_tls_libc_unknown;

  /* Fetch the program interpreter.  */
  std::optional<gdb::byte_vector> interp_name_holder
    = svr4_find_program_interpreter ();
  if (interp_name_holder)
    {
      /* A dynamically linked program linked against MUSL will have a
	 "ld-musl-" in its interpreter name.  (Two examples of MUSL
	 interpreter names are "/lib/ld-musl-x86_64.so.1" and
	 "lib/ld-musl-aarch64.so.1".)  If it's not found, assume GLIBC. */
      const char *interp_name = (const char *) interp_name_holder->data ();
      if (strstr (interp_name, "/ld-musl-") != nullptr)
	libc = svr4_tls_libc_musl;
      else
	libc = svr4_tls_libc_glibc;
      gdbarch_data->libc = libc;
      return libc;
    }

  /* If there is no interpreter name, it's statically linked.  For
     programs with TLS data, a program statically linked against MUSL
     will have the symbols 'main_tls' and 'builtin_tls'.  If both of
     these are present, assume that it was statically linked against
     MUSL, otherwise assume GLIBC.  */
  if (lookup_minimal_symbol (current_program_space, "main_tls").minsym
	!= nullptr
      && lookup_minimal_symbol (current_program_space, "builtin_tls").minsym
	!= nullptr)
    libc = svr4_tls_libc_musl;
  else
    libc = svr4_tls_libc_glibc;
  gdbarch_data->libc = libc;
  return libc;
}

/* Implement gdbarch method, get_thread_local_address, for architectures
   which provide a method for determining the DTV and possibly the DTP
   offset.  */

CORE_ADDR
svr4_tls_get_thread_local_address (struct gdbarch *gdbarch, ptid_t ptid,
				CORE_ADDR lm_addr, CORE_ADDR offset)
{
  svr4_tls_gdbarch_data *gdbarch_data = get_svr4_tls_gdbarch_data (gdbarch);

  /* Use the target's get_thread_local_address method when:

     - No method has been provided for finding the TLS DTV.

     or

     - The thread stratum has been pushed (at some point) onto the
       target stack, except when 'force_internal_tls_address_lookup'
       has been set.

     The idea here is to prefer use of of the target's thread_stratum
     method since it should be more accurate.  */
  if (gdbarch_data->get_tls_dtv_addr == nullptr
      || (find_target_at (thread_stratum) != nullptr
	  && !force_internal_tls_address_lookup))
    {
      struct target_ops *target = current_inferior ()->top_target ();
      return target->get_thread_local_address (ptid, lm_addr, offset);
    }
  else
    {
      /* Details, found below, regarding TLS layout is for the GNU C
	 library (glibc) and the MUSL C library (musl), circa 2024.
	 While some of this layout is defined by the TLS ABI, some of
	 it, such as how/where to find the DTV pointer in the TCB, is
	 not.  A good source of ABI info for some architectures can be
	 found in "ELF Handling For Thread-Local Storage" by Ulrich
	 Drepper.  That document is worth consulting even for
	 architectures not described there, since the general approach
	 and terminology is used regardless.

	 Some architectures, such as aarch64, are not described in
	 that document, so some details had to ferreted out using the
	 glibc source code.  Likewise, the MUSL source code was
	 consulted for details which differ from GLIBC.  */
      enum svr4_tls_libc libc = libc_tls_sniffer (gdbarch);
      int mod_id;
      if (libc == svr4_tls_libc_glibc)
	mod_id = glibc_link_map_to_tls_module_id (lm_addr);
      else /* Assume MUSL.  */
	mod_id = musl_link_map_to_tls_module_id (lm_addr);
      if (mod_id == 0)
	throw_error (TLS_GENERIC_ERROR, _("Unable to determine TLS module id"));

      /* Use the architecture specific DTV fetcher to obtain the DTV.  */
      CORE_ADDR dtv_addr = gdbarch_data->get_tls_dtv_addr (gdbarch, ptid, libc);

      /* In GLIBC, The DTV (dynamic thread vector) is an array of
	 structs consisting of two fields, the first of which is a
	 pointer to the TLS block of interest.  (The second field is a
	 pointer that assists with memory management, but that's not
	 of interest here.)  Also, the 0th entry is the generation
	 number, but although it's a single scalar, the 0th entry is
	 padded to be the same size as all the rest.  Thus each
	 element of the DTV array is two pointers in size.

	 In MUSL, the DTV is simply an array of pointers.  The 0th
	 entry is still the generation number, but contains no padding
	 aside from that which is needed to make it pointer sized.  */
      int m;	/* Multiplier, for size of DTV entry.  */
      switch (libc)
	{
	  case svr4_tls_libc_glibc:
	    m = 2;
	    break;
	  default:
	    m = 1;
	    break;
	}

      /* Obtain TLS block address.  Module ids start at 1, so there's
	 no need to adjust it to skip over the 0th entry of the DTV,
	 which is the generation number.  */
      CORE_ADDR dtv_elem_addr
	= dtv_addr + mod_id * m * (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
      gdb::byte_vector buf (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
      if (target_read_memory (dtv_elem_addr, buf.data (), buf.size ()) != 0)
	throw_error (TLS_GENERIC_ERROR, _("Unable to fetch TLS block address"));
      const struct builtin_type *builtin = builtin_type (gdbarch);
      CORE_ADDR tls_block_addr = gdbarch_pointer_to_address
				   (gdbarch, builtin->builtin_data_ptr,
				    buf.data ());

      /* When the TLS block addr is 0 or -1, this usually indicates that
	 the TLS storage hasn't been allocated yet.  (In GLIBC, some
	 architectures use 0 while others use -1.)  */
      if (tls_block_addr == 0 || tls_block_addr == (CORE_ADDR) -1)
	throw_error (TLS_NOT_ALLOCATED_YET_ERROR, _("TLS not allocated yet"));

      /* MUSL (and perhaps other C libraries, though not GLIBC) have
	 TLS implementations for some architectures which, for some
	 reason, have DTV entries which must be negatively offset by
	 DTP_OFFSET in order to obtain the TLS block address.
	 DTP_OFFSET is a constant in the MUSL sources - these offsets,
	 when they're non-zero, seem to be either 0x800 or 0x8000,
	 and are present for riscv[32/64], powerpc[32/64], m68k, and
	 mips.

	 Use the architecture specific get_tls_dtp_offset method, if
	 present, to obtain this offset.  */
      ULONGEST dtp_offset
	= gdbarch_data->get_tls_dtp_offset == nullptr
	  ? 0
	  : gdbarch_data->get_tls_dtp_offset (gdbarch, ptid, libc);

      return tls_block_addr - dtp_offset + offset;
    }
}

/* See svr4-tls-tdep.h.  */

void
svr4_tls_register_tls_methods (struct gdbarch_info info, struct gdbarch *gdbarch,
			    get_tls_dtv_addr_ftype *get_tls_dtv_addr,
			    get_tls_dtp_offset_ftype *get_tls_dtp_offset)
{
  gdb_assert (get_tls_dtv_addr != nullptr);

  svr4_tls_gdbarch_data *gdbarch_data = get_svr4_tls_gdbarch_data (gdbarch);
  gdbarch_data->get_tls_dtv_addr = get_tls_dtv_addr;
  gdbarch_data->get_tls_dtp_offset = get_tls_dtp_offset;
}

INIT_GDB_FILE (svr4_tls_tdep)
{
  add_setshow_boolean_cmd ("force-internal-tls-address-lookup", class_obscure,
			   &force_internal_tls_address_lookup, _("\
Set to force internal TLS address lookup."), _("\
Show whether GDB is forced to use internal TLS address lookup."), _("\
When resolving addresses for TLS (Thread Local Storage) variables,\n\
GDB will attempt to use facilities provided by the thread library (i.e.\n\
libthread_db).  If those facilities aren't available, GDB will fall\n\
back to using some internal (to GDB), but possibly less accurate\n\
mechanisms to resolve the addresses for TLS variables.  When this flag\n\
is set, GDB will force use of the fall-back TLS resolution mechanisms.\n\
This flag is used by some GDB tests to ensure that the internal fallback\n\
code is exercised and working as expected.  The default is to not force\n\
the internal fall-back mechanisms to be used."),
			   NULL, NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}
