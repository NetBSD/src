/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdint.h>

uint64_t global_var;

/* How long to usleep, in us.  The exp file assumes this is lower than
   1s.  */
#define MS_USLEEP 100000

/* How many increments per second MS_USLEEP corresponds to.  */
#define INCREMENTS_PER_SEC (1000000 / MS_USLEEP)

int
main ()
{
  while (1)
    {
      usleep (MS_USLEEP);
      global_var++;

      /* Time out after 30s.  */
      if (global_var >= (30 * INCREMENTS_PER_SEC))
	return 1;
    }

  return 0;
}
