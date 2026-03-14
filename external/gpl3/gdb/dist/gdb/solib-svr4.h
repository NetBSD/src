/* Handle shared libraries for GDB, the GNU Debugger.

   Copyright (C) 2000-2025 Free Software Foundation, Inc.

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

#ifndef GDB_SOLIB_SVR4_H
#define GDB_SOLIB_SVR4_H

#include "gdbarch.h"
#include "solib.h"

struct objfile;
struct link_map_offsets;
struct probe_and_action;
struct svr4_info;
struct svr4_library_list;
struct svr4_so;

/* Link map info to include in an allocated solib entry.  */

struct lm_info_svr4 final : public lm_info
{
  explicit lm_info_svr4 (CORE_ADDR debug_base)
    : debug_base (debug_base)
  {}

  /* Amount by which addresses in the binary should be relocated to
     match the inferior.  The direct inferior value is L_ADDR_INFERIOR.
     When prelinking is involved and the prelink base address changes,
     we may need a different offset - the recomputed offset is in L_ADDR.
     It is commonly the same value.  It is cached as we want to warn about
     the difference and compute it only once.  L_ADDR is valid
     iff L_ADDR_P.  */
  CORE_ADDR l_addr = 0, l_addr_inferior = 0;
  bool l_addr_p = false;

  /* The target location of lm.  */
  CORE_ADDR lm_addr = 0;

  /* Values read in from inferior's fields of the same name.  */
  CORE_ADDR l_ld = 0, l_next = 0, l_prev = 0, l_name = 0;

  /* The address of the dynamic linker structure (r_debug) this solib comes
     from.  This identifies which namespace this library is in.

     This field can be 0 in the following situations:

       - we receive the libraries through XML from an old gdbserver that
	 doesn't include the "lmid" field
       - the default debug base is not yet known

     In other cases, this field should have a sensible value.  */
  CORE_ADDR debug_base;
};

using lm_info_svr4_up = std::unique_ptr<lm_info_svr4>;

/* What to do when a probe stop occurs.  */

enum probe_action
{
  /* Something went seriously wrong.  Stop using probes and
     revert to using the older interface.  */
  PROBES_INTERFACE_FAILED,

  /* No action is required.  The shared object list is still
     valid.  */
  DO_NOTHING,

  /* The shared object list should be reloaded entirely.  */
  FULL_RELOAD,

  /* Attempt to incrementally update the shared object list. If
     the update fails or is not possible, fall back to reloading
     the list in full.  */
  UPDATE_OR_RELOAD,
};

/* solib_ops for SVR4 systems.  */

struct svr4_solib_ops : public solib_ops
{
  using solib_ops::solib_ops;

  void relocate_section_addresses (solib &so, target_section *) const override;
  void clear_so (const solib &so) const override;
  void clear_solib (program_space *pspace) const override;
  void create_inferior_hook (int from_tty) const override;
  owning_intrusive_list<solib> current_sos () const override;
  bool open_symbol_file_object (int from_tty) const override;
  bool in_dynsym_resolve_code (CORE_ADDR pc) const override;
  bool same (const solib &gdb, const solib &inferior) const override;
  bool keep_data_in_core (CORE_ADDR vaddr, unsigned long size) const override;
  void update_breakpoints () const override;
  void handle_event () const override;
  std::optional<CORE_ADDR> find_solib_addr (solib &so) const override;
  bool supports_namespaces () const override { return true; }
  int find_solib_ns (const solib &so) const override;
  int num_active_namespaces () const override;
  std::vector<const solib *> get_solibs_in_ns (int nsid) const override;
  void iterate_over_objfiles_in_search_order
    (iterate_over_objfiles_in_search_order_cb_ftype cb,
     objfile *current_objfile) const override;

  /* Return the appropriate link map offsets table for the architecture.  */
  virtual link_map_offsets *fetch_link_map_offsets () const = 0;

  /* This needs to be public because it's accessed from an observer.  */
  void current_sos_direct (svr4_info *info) const;

private:
  void create_probe_breakpoints (svr4_info *info, gdbarch *gdbarch,
				 const std::vector<probe *> *probes,
				 objfile *objfile) const;
  bool find_and_create_probe_breakpoints (svr4_info *info, gdbarch *gdbarch,
					  obj_section *os,
					  bool with_prefix) const;
  void create_event_breakpoints (svr4_info *info, gdbarch *gdbarch,
				 CORE_ADDR address) const;
  int enable_break (svr4_info *info, int from_tty) const;
  void free_probes_table (svr4_info *info) const;
  CORE_ADDR find_r_brk (svr4_info *info) const;
  CORE_ADDR find_r_ldsomap (svr4_info *info) const;
  owning_intrusive_list<solib> default_sos (svr4_info *info) const;
  int read_so_list (svr4_info *info, CORE_ADDR lm, CORE_ADDR prev_lm,
		    CORE_ADDR debug_base, std::vector<svr4_so> &sos,
		    int ignore_first) const;
  lm_info_svr4_up read_lm_info (CORE_ADDR lm_addr, CORE_ADDR debug_base) const;
  int has_lm_dynamic_from_link_map () const;
  CORE_ADDR lm_addr_check (const solib &so, bfd *abfd) const;
  CORE_ADDR read_r_next (CORE_ADDR debug_base) const;
  CORE_ADDR read_r_map (CORE_ADDR debug_base) const;
  owning_intrusive_list<solib> collect_probes_sos (svr4_info *info) const;
  owning_intrusive_list<solib> current_sos_1 (svr4_info *info) const;
  owning_intrusive_list<solib> solibs_from_svr4_sos
    (const std::vector<svr4_so> &sos) const;
  void disable_probes_interface (svr4_info *info) const;
  void update_full (svr4_info *info) const;
  int update_incremental (svr4_info *info, CORE_ADDR debug_base,
			  CORE_ADDR lm) const;
  bool update_event_breakpoint (breakpoint *b) const;

  /* Return the base address of the dynamic linker structure for the default
     namespace.  */
  CORE_ADDR default_debug_base (svr4_info *info, bool *changed = nullptr) const;
};

/* solib_ops for ILP32 SVR4 systems.  */

struct ilp32_svr4_solib_ops : public svr4_solib_ops
{
  using svr4_solib_ops::svr4_solib_ops;

  link_map_offsets *fetch_link_map_offsets () const override;
};

/* Critical offsets and sizes which describe struct r_debug and
   struct link_map on SVR4-like targets.  All offsets and sizes are
   in bytes unless otherwise specified.  */

struct link_map_offsets
  {
    /* Offset and size of r_debug.r_version.  */
    int r_version_offset, r_version_size;

    /* Offset of r_debug.r_map.  */
    int r_map_offset;

    /* Offset of r_debug.r_brk.  */
    int r_brk_offset;

    /* Offset of r_debug.r_ldsomap.  */
    int r_ldsomap_offset;

    /* Offset of r_debug_extended.r_next.  */
    int r_next_offset;

    /* Size of struct link_map (or equivalent), or at least enough of it
       to be able to obtain the fields below.  */
    int link_map_size;

    /* Offset to l_addr field in struct link_map.  */
    int l_addr_offset;

    /* Offset to l_ld field in struct link_map.  */
    int l_ld_offset;

    /* Offset to l_next field in struct link_map.  */
    int l_next_offset;

    /* Offset to l_prev field in struct link_map.  */
    int l_prev_offset;

    /* Offset to l_name field in struct link_map.  */
    int l_name_offset;
  };

/* Set the gdbarch methods for SVR4 systems.  */

extern void set_solib_svr4_ops (gdbarch *gdbarch,
				gdbarch_make_solib_ops_ftype make_solib_ops);

/* This function is called by thread_db.c.  Return the address of the
   link map for the given objfile.  */
extern CORE_ADDR svr4_fetch_objfile_link_map (struct objfile *objfile);

/* Return a new solib_ops for ILP32 SVR4 systems.  */

extern solib_ops_up make_svr4_ilp32_solib_ops (program_space *pspace);

/* Return a new solib_ops for LP64 SVR4 systems.  */

extern solib_ops_up make_svr4_lp64_solib_ops (program_space *pspace);

/* For the MUSL C library, given link map address LM_ADDR, return the
   corresponding TLS module id, or 0 if not found.  */
int musl_link_map_to_tls_module_id (CORE_ADDR lm_addr);

/* For GLIBC, given link map address LM_ADDR, return the corresponding TLS
   module id, or 0 if not found.  */
int glibc_link_map_to_tls_module_id (CORE_ADDR lm_addr);

/* Return program interpreter string.  */

std::optional<gdb::byte_vector> svr4_find_program_interpreter ();

#endif /* GDB_SOLIB_SVR4_H */
