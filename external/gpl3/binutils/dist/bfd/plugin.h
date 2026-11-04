/* Plugin support for BFD.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

struct ld_plugin_input_file;

#if BFD_SUPPORTS_PLUGINS
void bfd_plugin_set_program_name (const char *);
int bfd_plugin_open_input (bfd *, struct ld_plugin_input_file *);
void bfd_plugin_set_plugin (const char *);
bool bfd_link_plugin_object_p (bfd *);
void register_ld_plugin_object_p (bfd_cleanup (*object_p) (bfd *, bool));
void bfd_plugin_close_file_descriptor (bfd *, int);

static inline const bfd_target *
bfd_plugin_vec (void)
{
  extern const bfd_target plugin_vec;
  return &plugin_vec;
}

static inline bool
bfd_plugin_target_p (const bfd_target *target)
{
  return target == bfd_plugin_vec ();
}
#else
static inline void
bfd_plugin_set_program_name (const char *name ATTRIBUTE_UNUSED)
{
}

static inline int
bfd_plugin_open_input (bfd *ibfd ATTRIBUTE_UNUSED,
		       struct ld_plugin_input_file *file ATTRIBUTE_UNUSED)
{
  return 0;
}

static inline void
bfd_plugin_set_plugin (const char *p ATTRIBUTE_UNUSED)
{
}

static inline bool
bfd_link_plugin_object_p (bfd *abfd ATTRIBUTE_UNUSED)
{
  return false;
}

static inline void
register_ld_plugin_object_p
  (bfd_cleanup (*object_p) (bfd *, bool) ATTRIBUTE_UNUSED)
{
}

static inline void
bfd_plugin_close_file_descriptor (bfd *abfd ATTRIBUTE_UNUSED,
				  int fd ATTRIBUTE_UNUSED)
{
}

static inline const bfd_target *
bfd_plugin_vec (void)
{
  return NULL;
}

static inline bool
bfd_plugin_target_p (const bfd_target *target ATTRIBUTE_UNUSED)
{
  return false;
}
#endif

#endif
