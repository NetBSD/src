/* Copyright 2024-2025 Free Software Foundation, Inc.

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

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

volatile void* library_base_address = 0;
volatile int *ptr = 0;

int
main ()
{
  struct stat buf;
  int res;

  int fd = open (SHLIB_FILENAME, O_RDONLY);
  assert (fd != -1);

  res = fstat (fd, &buf);
  assert (res == 0);

  library_base_address
    = mmap (NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  res = *ptr;	/* Undefined behavior here.  */

  return 0;
}
