/* Copyright (C) 2012-2019 Free Software Foundation, Inc.

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

#include "server.h"
#include "tdesc.h"
#include "regdef.h"

#ifndef IN_PROCESS_AGENT

target_desc::~target_desc ()
{
  xfree ((char *) arch);
  xfree ((char *) osabi);
}

bool target_desc::operator== (const target_desc &other) const
{
  if (reg_defs != other.reg_defs)
    return false;

  /* Compare expedite_regs.  */
  int i = 0;
  for (; expedite_regs[i] != NULL; i++)
    {
      if (strcmp (expedite_regs[i], other.expedite_regs[i]) != 0)
	return false;
    }
  if (other.expedite_regs[i] != NULL)
    return false;

  return true;
}

#endif

void target_desc::accept (tdesc_element_visitor &v) const
{
#ifndef IN_PROCESS_AGENT
  v.visit_pre (this);

  for (const tdesc_feature_up &feature : features)
    feature->accept (v);

  v.visit_post (this);
#endif
}

void
init_target_desc (struct target_desc *tdesc,
		  const char **expedite_regs)
{
  int offset = 0;

  /* Go through all the features and populate reg_defs.  */
  for (const tdesc_feature_up &feature : tdesc->features)
    for (const tdesc_reg_up &treg : feature->registers)
      {
	int regnum = treg->target_regnum;

	/* Register number will increase (possibly with gaps) or be zero.  */
	gdb_assert (regnum == 0 || regnum >= tdesc->reg_defs.size ());

	if (regnum != 0)
	  tdesc->reg_defs.resize (regnum, reg (offset));

	tdesc->reg_defs.emplace_back (treg->name.c_str (), offset,
				      treg->bitsize);
	offset += treg->bitsize;
      }

  tdesc->registers_size = offset / 8;

  /* Make sure PBUFSIZ is large enough to hold a full register
     packet.  */
  gdb_assert (2 * tdesc->registers_size + 32 <= PBUFSIZ);

#ifndef IN_PROCESS_AGENT
  tdesc->expedite_regs = expedite_regs;
#endif
}

struct target_desc *
allocate_target_description (void)
{
  return new target_desc ();
}

#ifndef IN_PROCESS_AGENT

static const struct target_desc default_description {};

void
copy_target_description (struct target_desc *dest,
			 const struct target_desc *src)
{
  dest->reg_defs = src->reg_defs;
  dest->expedite_regs = src->expedite_regs;
  dest->registers_size = src->registers_size;
  dest->xmltarget = src->xmltarget;
}

const struct target_desc *
current_target_desc (void)
{
  if (current_thread == NULL)
    return &default_description;

  return current_process ()->tdesc;
}

/* See common/tdesc.h.  */

const char *
tdesc_architecture_name (const struct target_desc *target_desc)
{
  return target_desc->arch;
}

/* See common/tdesc.h.  */

void
set_tdesc_architecture (struct target_desc *target_desc,
			const char *name)
{
  target_desc->arch = xstrdup (name);
}

/* See common/tdesc.h.  */

const char *
tdesc_osabi_name (const struct target_desc *target_desc)
{
  return target_desc->osabi;
}

/* See common/tdesc.h.  */

void
set_tdesc_osabi (struct target_desc *target_desc, const char *name)
{
  target_desc->osabi = xstrdup (name);
}

/* See common/tdesc.h.  */

const char *
tdesc_get_features_xml (const target_desc *tdesc)
{
  /* Either .xmltarget or .features is not NULL.  */
  gdb_assert (tdesc->xmltarget != NULL
	      || (!tdesc->features.empty ()
		  && tdesc->arch != NULL));

  if (tdesc->xmltarget == NULL)
    {
      std::string buffer ("@");
      print_xml_feature v (&buffer);
      tdesc->accept (v);
      tdesc->xmltarget = xstrdup (buffer.c_str ());
    }

  return tdesc->xmltarget;
}
#endif

/* See common/tdesc.h.  */

struct tdesc_feature *
tdesc_create_feature (struct target_desc *tdesc, const char *name)
{
  struct tdesc_feature *new_feature = new tdesc_feature (name);
  tdesc->features.emplace_back (new_feature);
  return new_feature;
}
