/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

int
main (void)
{
  void *handle[4];
  int (*fun) (int);
  Lmid_t lmid;
  int dl;

  handle[0] = dlmopen (LM_ID_NEWLM, DSO_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[0] != NULL);

  handle[1] = dlmopen (LM_ID_NEWLM, DSO_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[1] != NULL);

  handle[2] = dlmopen (LM_ID_NEWLM, DSO_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[2] != NULL);

  for (dl = 2; dl >= 0; dl--)
    {
      fun = dlsym (handle[dl], "inc");
      fun (dl);
    }

  dlclose (handle[0]); /* TAG: first dlclose */
  dlclose (handle[1]); /* TAG: second dlclose */
  dlclose (handle[2]); /* TAG: third dlclose */

  handle[3] = dlmopen (LM_ID_NEWLM, DSO_NAME, RTLD_LAZY | RTLD_LOCAL);
  dlinfo (handle[3], RTLD_DI_LMID, &lmid);

  dlclose (handle[3]); /* TAG: fourth dlclose */

  return 0;
}
