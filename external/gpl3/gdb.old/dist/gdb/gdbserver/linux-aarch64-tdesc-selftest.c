/* Copyright (C) 2017-2019 Free Software Foundation, Inc.

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
#include "common/selftest.h"
#include "linux-aarch64-tdesc.h"

/* Defined in auto-generated file features/aarch64.c.  */
void init_registers_aarch64 (void);
extern const struct target_desc *tdesc_aarch64;

namespace selftests {
namespace tdesc {
static void
aarch64_tdesc_test ()
{
  const target_desc *tdesc = aarch64_linux_read_description (0);
  SELF_CHECK (*tdesc == *tdesc_aarch64);
}
}
} // namespace selftests

void
initialize_low_tdesc ()
{
  init_registers_aarch64 ();

  selftests::register_test ("aarch64-tdesc",
			    selftests::tdesc::aarch64_tdesc_test);
}
