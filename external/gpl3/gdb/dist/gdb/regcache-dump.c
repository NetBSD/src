/* Copyright (C) 1986-2025 Free Software Foundation, Inc.

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

#include "cli/cli-cmds.h"
#include "regcache.h"
#include "gdbsupport/def-vector.h"
#include "valprint.h"
#include "remote.h"
#include "reggroups.h"
#include "target.h"
#include "gdbarch.h"
#include "inferior.h"

/* Dump registers from regcache, used for dumping raw registers and
   cooked registers.  */

class register_dump_regcache : public register_dump
{
public:
  register_dump_regcache (regcache *regcache, bool dump_pseudo)
    : register_dump (regcache->arch ()), m_regcache (regcache),
      m_dump_pseudo (dump_pseudo)
  {
  }

protected:

  int num_additional_headers () override
  { return 1; }

  void additional_headers (ui_out *out) override
  {
    out->table_header (0, ui_left, "value",
		       m_dump_pseudo ? "Cooked value" : "Raw value");
  }

  void dump_reg (ui_out *out, int regnum) override
  {
    if (regnum < gdbarch_num_regs (m_gdbarch) || m_dump_pseudo)
      {
	auto size = register_size (m_gdbarch, regnum);

	if (size == 0)
	  return;

	gdb::byte_vector buf (size);
	auto status = m_regcache->cooked_read (regnum, buf.data ());

	if (status == REG_UNKNOWN)
	  out->field_string ("value", "<invalid>");
	else if (status == REG_UNAVAILABLE)
	  out->field_string ("value", "<unavailable>");
	else
	  {
	    string_file str;
	    print_hex_chars (&str, buf.data (), size,
			     gdbarch_byte_order (m_gdbarch), true);
	    out->field_stream ("value", str);
	  }
      }
    else
      {
	/* Just print "<cooked>" for pseudo register when
	   regcache_dump_raw.  */
	out->field_string ("value", "<cooked>");
      }
  }

private:
  regcache *m_regcache;

  /* Dump pseudo registers or not.  */
  const bool m_dump_pseudo;
};

/* Dump from reg_buffer, used when there is no thread or
   registers.  */

class register_dump_reg_buffer : public register_dump, reg_buffer
{
public:
  register_dump_reg_buffer (gdbarch *gdbarch, bool dump_pseudo)
    : register_dump (gdbarch), reg_buffer (gdbarch, dump_pseudo)
  {
  }

protected:

  int num_additional_headers () override
  { return 1; }

  void additional_headers (ui_out *out) override
  {
    out->table_header (0, ui_left, "value",
		       m_has_pseudo ? "Cooked value" : "Raw value");
  }

  void dump_reg (ui_out *out, int regnum) override
  {
    if (regnum < gdbarch_num_regs (m_gdbarch) || m_has_pseudo)
      {
	auto size = register_size (regnum);

	if (size == 0)
	  return;

	auto status = get_register_status (regnum);

	gdb_assert (status != REG_VALID);

	if (status == REG_UNKNOWN)
	  out->field_string ("value", "<invalid>");
	else
	  out->field_string ("value", "<unavailable>");
      }
    else
      {
	/* Just print "<cooked>" for pseudo register when
	   regcache_dump_raw.  */
	out->field_string ("value", "<cooked>");
      }
  }
};

/* For "maint print registers".  */

class register_dump_none : public register_dump
{
public:
  register_dump_none (gdbarch *arch)
    : register_dump (arch)
  {}

protected:

  int num_additional_headers () override
  { return 0; }

  void additional_headers (ui_out *out) override
  { }

  void dump_reg (ui_out *out, int regnum) override
  {}
};

/* For "maint print remote-registers".  */

class register_dump_remote : public register_dump
{
public:
  register_dump_remote (gdbarch *arch)
    : register_dump (arch)
  {}

protected:

  int num_additional_headers () override
  { return 3; }

  void additional_headers (ui_out *out) override
  {
    out->table_header (7, ui_left, "remnum", "Rmt Nr");
    out->table_header (11, ui_left, "goffset", "g/G Offset");
    out->table_header (3, ui_left, "expedited", "Expedited");
  }

  void dump_reg (ui_out *out, int regnum) override
  {
    int pnum, poffset;

    if (regnum < gdbarch_num_regs (m_gdbarch)
	&& remote_register_number_and_offset (m_gdbarch, regnum,
					      &pnum, &poffset))
      {
	out->field_signed ("remnum", pnum);
	out->field_signed ("goffset", poffset);

	if (remote_register_is_expedited (regnum))
	  out->field_string ("expedited", "yes");
	else
	  out->field_skip ("expedited");
      }
    else
      {
	out->field_skip ("remnum");
	out->field_skip ("goffset");
	out->field_skip ("expedited");
      }
  }
};

/* For "maint print register-groups".  */

class register_dump_groups : public register_dump
{
public:
  register_dump_groups (gdbarch *arch)
    : register_dump (arch)
  {}

protected:

  int num_additional_headers () override
  { return 1; }

  void additional_headers (ui_out *out) override
  {
    out->table_header (0, ui_left, "groups", "Groups");
  }

  void dump_reg (ui_out *out, int regnum) override
  {
    string_file file;
    const char *sep = "";
    for (const struct reggroup *group : gdbarch_reggroups (m_gdbarch))
      {
	if (gdbarch_register_reggroup_p (m_gdbarch, regnum, group))
	  {
	    gdb_printf (&file, "%s%s", sep, group->name ());
	    sep = ",";
	  }
      }
    out->field_stream ("groups", file);
  }
};

enum regcache_dump_what
{
  regcache_dump_none, regcache_dump_raw,
  regcache_dump_cooked, regcache_dump_groups,
  regcache_dump_remote
};

/* Helper for the various maint commands that print registers.  ARGS
   is the arguments passed to the command.  WHAT_TO_DUMP indicates
   exactly which registers to display.  COMMAND is the command name,
   used in error messages.  */

static void
regcache_print (const char *args, enum regcache_dump_what what_to_dump,
		const char *command)
{
  /* Where to send output.  */
  stdio_file file;
  std::optional<ui_out_redirect_pop> redirect;

  if (args != nullptr)
    {
      if (!file.open (args, "w"))
	perror_with_name (command);
      redirect.emplace (current_uiout, &file);
    }

  std::unique_ptr<register_dump> dump;
  std::unique_ptr<regcache> regs;
  gdbarch *gdbarch;

  if (target_has_registers ())
    gdbarch = get_thread_regcache (inferior_thread ())->arch ();
  else
    gdbarch = current_inferior ()->arch ();

  const char *name;
  switch (what_to_dump)
    {
    case regcache_dump_none:
      dump = std::make_unique<register_dump_none> (gdbarch);
      name = "Registers";
      break;
    case regcache_dump_remote:
      dump = std::make_unique<register_dump_remote> (gdbarch);
      name = "RegisterRemote";
      break;
    case regcache_dump_groups:
      dump = std::make_unique<register_dump_groups> (gdbarch);
      name = "RegisterGroups";
      break;
    case regcache_dump_raw:
    case regcache_dump_cooked:
      {
	name = "RegisterDump";
	auto dump_pseudo = (what_to_dump == regcache_dump_cooked);

	if (target_has_registers ())
	  dump = (std::make_unique<register_dump_regcache>
		  (get_thread_regcache (inferior_thread ()), dump_pseudo));
	else
	  {
	    /* For the benefit of "maint print registers" & co when
	       debugging an executable, allow dumping a regcache even when
	       there is no thread selected / no registers.  */
	    dump = std::make_unique<register_dump_reg_buffer> (gdbarch,
							       dump_pseudo);
	  }
      }
      break;
    }

  dump->dump (current_uiout, name);
}

static void
maintenance_print_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_none, "maintenance print registers");
}

static void
maintenance_print_raw_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_raw, "maintenance print raw-registers");
}

static void
maintenance_print_cooked_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_cooked,
		  "maintenance print cooked-registers");
}

static void
maintenance_print_register_groups (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_groups,
		  "maintenance print register-groups");
}

static void
maintenance_print_remote_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_remote,
		  "maintenance print remote-registers");
}

INIT_GDB_FILE (regcache_dump)
{
  add_cmd ("registers", class_maintenance, maintenance_print_registers,
	   _("Print the internal register configuration.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("raw-registers", class_maintenance,
	   maintenance_print_raw_registers,
	   _("Print the internal register configuration "
	     "including raw values.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("cooked-registers", class_maintenance,
	   maintenance_print_cooked_registers,
	   _("Print the internal register configuration "
	     "including cooked values.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("register-groups", class_maintenance,
	   maintenance_print_register_groups,
	   _("Print the internal register configuration "
	     "including each register's group.\n"
	     "Takes an optional file parameter."),
	   &maintenanceprintlist);
  add_cmd ("remote-registers", class_maintenance,
	   maintenance_print_remote_registers, _("\
Print the internal register configuration.\n\
Usage: maintenance print remote-registers [FILE]\n\
The remote register number and g/G packets offset are included,\n\
as well as which registers were sent in the last stop reply packet\n\
(i.e., expedited)."),
	   &maintenanceprintlist);
}
