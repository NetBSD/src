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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This file contains a library that can be preloaded into GDB on Linux
   using the LD_PRELOAD technique.

   The library intercepts calls to OPEN, CLOSE, READ, and PREAD in order to
   fake the inode number of a shared memory mapping.

   When GDB creates a core file (e.g. with the 'gcore' command), then
   shared memory mappings should be included in the generated core file.

   The 'id' for the shared memory mapping shares the inode slot in the
   /proc/PID/smaps file, which is what GDB consults to decide which
   mappings should be included in the core file.

   It is possible for a shared memory mapping to have an 'id' of zero.

   At one point there was a bug in GDB where mappings with an inode of zero
   would not be included in the generated core file.  This meant that most
   shared memory mappings would be included in the generated core file,
   but, if a shared memory mapping happened to get an 'id' of zero, then,
   because this would appear as a zero inode in the smaps file, this shared
   memory mapping would be excluded from the generated core file.

   This preload library spots when GDB opens a /proc/PID/smaps file and
   immediately copies the contents of this file into an internal buffer.
   The buffer is then scanned looking for a shared memory mapping, and, if
   a shared memory mapping is found, its 'id' (in the inode position) is
   changed to zero.

   Calls to read/pread are intercepted, and attempts to read from the smaps
   file are then served from the modified buffer contents.

   The close calls are monitored and, when the smaps file is closed, the
   internal buffer is released.

   This works with GDB (currently) because the requirements for access to
   the smaps file are pretty simple.  GDB opens the file and grabs the
   entire contents with a single pread call and a large buffer.  There's no
   seeking within the file or anything like that.

   The intention is that this library is preloaded into a GDB session which
   is then used to start an inferior and generate a core file.  GDB will
   then see the zero inode for the shared memory mapping and should, if the
   bug is correctly fixed, still add the shared memory mapping to the
   generated core file.  */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/* Logging.  */

static void
log_msg (const char *fmt, ...)
{
#ifdef LOGGING
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
#endif /* LOGGING */
}

/* Error handling, message and exit.  */

static void
error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  exit (EXIT_FAILURE);
}

/* The type of the open() function.  */
typedef int (*open_func_type)(const char *pathname, int flags, ...);

/* The type of the close() function.  */
typedef int (*close_func_type)(int fd);

/* The type of the read() function.  */
typedef ssize_t (*read_func_type)(int fd, void *buf, size_t count);

/* The type of the pread() function.  */
typedef ssize_t (*pread_func_type) (int fd, void *buf, size_t count, off_t offset);

/* Structure that holds information about a /proc/PID/smaps file that has
   been opened.  */
struct interesting_file
{
  /* The file descriptor for the opened file.  */
  int fd;

  /* The read offset within the file.  Set to zero when the file is
     opened.  Any 'read' calls will update this offset.  */
  size_t offset;

  /* The size of the contents within the buffer.  This is not the total
     buffer size (which might be larger).  Attempts to read beyond SIZE
     indicate an attempt to read beyond the end of the file.  */
  size_t size;

  /* The (possibly modified) contents of the file.  */
  char *content;
};

/* We only track a single interesting file.  Currently, for the use case
   we imagine, GDB will only ever open one /proc/PID/smaps file at once.  */
struct interesting_file the_file = { -1, 0, 0, NULL };

/* Update the contents of the global THE_FILE buffer.  It is assumed that
   the file contents have already been loaded into THE_FILE's content
   buffer.

   Look for any lines that represent a shared memory mapping and modify
   the inode field (which holds the shared memory id) to be zero.  */
static void
update_file_content_buffer (void)
{
  assert (the_file.content != NULL);

  char *start = the_file.content;
  do
    {
      /* Every line, even the last one, ends with a newline.  */
      char *end = strchrnul (start, '\n');
      assert (end != NULL);
      assert (*end != '\0');

      /* Attribute lines start with an uppercase letter.  The lines we want
	 to modify should start with a lower case hex character,
	 i.e. [0-9a-f].  Also, every line that we want to consider should
	 be long enough, but just in case, check the longest possible
	 filename that we care about.  */
      if (isxdigit (*start) && (isdigit (*start) || islower (*start))
	  && (end - start) > 23)
	{
	  /* There are two possible filenames that we look for:
	       /SYSV%08x
	       /SYSV%08x (deleted)
	     The END pointer is pointing to the first character after the
	     filename.

	     Setup OFFSET to be the offset from END to the start of the
	     filename.  As we check the filename we set OFFSET to 0 if the
	     filename doesn't match one of the expected patterns.  */
	  size_t offset;
	  if (strncmp ((end - 13), "/SYSV", 5) == 0)
	    offset = 13;
	  else if (strncmp ((end - 23), "/SYSV", 5) == 0)
	    {
	      if (strncmp ((end - 10), " (deleted)", 10) == 0)
		offset = 23;
	      else
		offset = 0;
	    }
	  else
	    offset = 0;

	  for (int i = 0; i < 8 && offset != 0; ++i)
	    {
	      if (!isdigit (*(end - offset + 5 + i)))
		offset = 0;
	    }

	  /* If OFFSET is non-zero then the filename on this line looks
	     like a shared memory mapping, and OFFSET is the offset from
	     END to the first character of the filename.  */
	  if (offset != 0)
	    {
	      log_msg ("[LD_PRELOAD] shared memory entry: %.*s\n",
		       offset, (end - offset));

	      /* Set PTR to the first character before the filename.  This
		 should be a white space character.  */
	      char *ptr = end - offset - 1;
	      assert (isspace (*ptr));

	      /* Walk backwards until we find the inode field.  */
	      while (isspace (*ptr))
		--ptr;

	      /* Now replace every character in the inode field, except the
		 first one, with a space character.  */
	      while (!isspace (*(ptr - 1)))
		{
		  assert (isdigit (*ptr));
		  *ptr = ' ';
		  --ptr;
		}

	      /* Replace the first character with '0'.  */
	      assert (isdigit (*ptr));
	      *ptr = '0';

	      /* This print is checked for from GDB.  */
	      printf ("[LD_PRELOAD] updated a shared memory mapping\n");
	    }
	}

      /* Update START to point to the next line.  The last line of the
	 file will be empty.  */
      assert (*end == '\n');
      start = end;
      while (*start == '\n')
	++start;
    }
  while (*start != '\0');
}

/* Return true if PATHNAME has for form "/proc/PID/smaps" (without the
   quotes).  Otherwise, return false.  */

static bool
is_smaps_file (const char *pathname)
{
  if (strncmp (pathname, "/proc/", 6) == 0)
    {
      int idx = 6;
      while (isdigit (pathname[idx]))
	idx++;
      if (idx > 6 && strcmp (&pathname[idx], "/smaps") == 0)
	return true;
    }

  return false;
}

/* Return true if PATHNAME should be considered interesting.  PATHNAME is
   interesting if it has the form /proc/PID/smaps, and there is no
   interesting file already opened.  */

static bool
is_interesting_pathname (const char *pathname)
{
  return the_file.fd == -1 && is_smaps_file (pathname);
}

/* Read the contents of an interesting file from FD (and open file
   descriptor) into the global THE_FILE variable, making the file FD the
   current interesting file.  There should be no already open interesting
   file when this function is called.

   The contents of the file FD are read into a memory buffer and updated so
   that any shared memory mappings listed within FD (which will be an smaps
   file) will have the id zero.  */

static void
read_interesting_file_contents (int fd)
{
#define BLOCK_SIZE 1024
  /* Slurp contents into a local buffer.  */
  size_t buffer_size = 1024;
  size_t offset = 0;

  assert (the_file.size == 0);
  assert (the_file.content == NULL);
  assert (the_file.fd == -1);
  assert (the_file.offset == 0);

  do
    {
      the_file.content = (char *) realloc (the_file.content, buffer_size);
      if (the_file.content == NULL)
	error ("[LD_PRELOAD] Failed allocating memory: %s\n", strerror (errno));

      ssize_t bytes_read = read (fd, the_file.content + offset, BLOCK_SIZE);
      if (bytes_read == -1)
	error ("[LD_PRELOAD] Failed reading file: %s\n", strerror (errno));

      the_file.size += bytes_read;

      if (bytes_read < BLOCK_SIZE)
	break;

      offset += BLOCK_SIZE;
      buffer_size += BLOCK_SIZE;
    }
  while (true);

  /* Add a null terminator.  This makes the update easier.  We know
     there will be space because we only break out of the loop above
     when the last read returns less than BLOCK_SIZE bytes.  This means
     we allocated an extra BLOCK_SIZE bytes, but didn't fill them all.
     This means there must be at least 1 byte available for the null.  */
  the_file.content[the_file.size] = '\0';

  /* Reset the seek pointer.  */
  if (lseek (fd, 0, SEEK_SET) == (off_t) -1)
    error ("[LD_PRELOAD] Failed to lseek in file: %s\n", strerror (errno));

  /* Record the file descriptor, this is used in read, pread, and close
     in order to spot when we need to intercept the call.  */
  the_file.fd = fd;

  update_file_content_buffer ();
#undef BLOCK_SIZE
}

/* Intercept calls to 'open'.  If this is an attempt to open a
   /proc/PID/smaps file then intercept it, load the file contents into a
   buffer and update the file contents.  For all other open requests, just
   forward to the real open function.  */
int
open (const char *pathname, int flags, ...)
{
  /* Pointer to the real open function.  */
  static open_func_type real_open = NULL;

  /* Mode is only used if the O_CREAT flag is set in FLAGS.  */
  mode_t mode = 0;

  /* Set true if this is a /proc/PID/smaps file.  */
  bool is_interesting = is_interesting_pathname (pathname);

  /* Check if O_CREAT is in flags. If it is, get the mode.  */
  if (flags & O_CREAT)
    {
      va_list args;
      va_start (args, flags);
      mode = va_arg (args, mode_t);
      va_end (args);
    }

  /* Debug.  */
  if (is_interesting)
    log_msg ("[LD_PRELOAD] Opening file: %s\n", pathname);

  /* Make sure we have a pointer to the real open() function.  */
  if (real_open == NULL)
    {
      /* Get the address of the real open() function.  */
      real_open = (open_func_type) dlsym (RTLD_NEXT, "open");
      if (real_open == NULL)
	error ("[LD_PRELOAD] dlsym() error for 'open': %s\n", dlerror ());
    }

  /* Call the original open() function with the provided arguments.  */
  int res = -1;
  if (flags & O_CREAT)
    res = real_open (pathname, flags, mode);
  else
    res = real_open (pathname, flags);

  if (res != -1 && is_interesting)
    read_interesting_file_contents (res);

  return res;
}

/* Like above, but for open64.  */

int
open64 (const char *pathname, int flags, ...)
{
  /* Pointer to the real open64 function.  */
  static open_func_type real_open64 = NULL;

  /* Mode is only used if the O_CREAT flag is set in FLAGS.  */
  mode_t mode = 0;

  /* Set true if this is a /proc/PID/smaps file.  */
  bool is_interesting = is_interesting_pathname (pathname);

  /* Check if O_CREAT is in flags. If it is, get the mode.  */
  if (flags & O_CREAT)
    {
      va_list args;
      va_start (args, flags);
      mode = va_arg (args, mode_t);
      va_end (args);
    }

  /* Debug.  */
  if (is_interesting)
    log_msg ("[LD_PRELOAD] Opening file: %s\n", pathname);

  /* Make sure we have a pointer to the real open64() function.  */
  if (real_open64 == NULL)
    {
      /* Get the address of the real open64() function.  */
      real_open64 = (open_func_type) dlsym (RTLD_NEXT, "open64");
      if (real_open64 == NULL)
	error ("[LD_PRELOAD] dlsym() error for 'open64': %s\n", dlerror ());
    }

  /* Call the original open64() function with the provided arguments.  */
  int res = -1;
  if (flags & O_CREAT)
    res = real_open64 (pathname, flags, mode);
  else
    res = real_open64 (pathname, flags);

  if (res != -1 && is_interesting)
    read_interesting_file_contents (res);

  return res;
}

/* Intercept the 'close' function.  If this is a previously opened
   interesting file then clean up.  Otherwise, forward to the normal close
   function.  */
int
close (int fd)
{
  static close_func_type real_close = NULL;

  if (fd == the_file.fd)
    {
      the_file.fd = -1;
      free (the_file.content);
      the_file.content = NULL;
      the_file.offset = 0;
      the_file.size = 0;
      log_msg ("[LD_PRELOAD] Closing file.\n");
    }

  /* Make sure we have a pointer to the real open() function.  */
  if (real_close == NULL)
    {
      /* Get the address of the real open() function.  */
      real_close = (close_func_type) dlsym (RTLD_NEXT, "close");
      if (real_close == NULL)
	error ("[LD_PRELOAD] dlsym() error for 'close': %s\n", dlerror ());
    }

  return real_close (fd);
}

/* Intercept 'pread' calls.  If this is a pread from a previously opened
   interesting file, then read from the in memory buffer.  Otherwise,
   forward to the real pread function.  */
ssize_t
pread (int fd, void *buf, size_t count, off_t offset)
{
  static pread_func_type real_pread = NULL;

  if (fd == the_file.fd)
    {
      size_t max;

      if (offset > the_file.size)
	max = 0;
      else
	max = the_file.size - offset;
      if (count > max)
	count = max;

      memcpy (buf, the_file.content + offset, count);
      log_msg ("[LD_PRELOAD] Read from file.\n");
      return count;
    }

  if (real_pread == NULL)
    {
      /* Get the address of the real read() function.  */
      real_pread = (pread_func_type) dlsym (RTLD_NEXT, "pread");
      if (real_pread == NULL)
	error ("[LD_PRELOAD] dlsym() error for 'pread': %s\n", dlerror ());
    }

  return real_pread (fd, buf, count, offset);
}

/* Intercept 'read' calls.  If this is a read from a previously opened
   interesting file, then read from the in memory buffer.  Otherwise,
   forward to the real read function.  */
ssize_t
read (int fd, void *buf, size_t count)
{
  static read_func_type real_read = NULL;

  if (fd == the_file.fd)
    {
      ssize_t bytes_read = pread (fd, buf, count, the_file.offset);
      if (bytes_read > 0)
	the_file.offset += bytes_read;
      return bytes_read;
    }

  if (real_read == NULL)
    {
      /* Get the address of the real read() function.  */
      real_read = (read_func_type) dlsym (RTLD_NEXT, "read");
      if (real_read == NULL)
	error ("[LD_PRELOAD] dlsym() error for 'read': %s\n", dlerror ());
    }

  return real_read (fd, buf, count);
}
