/* debuginfod utilities for GDB.
   Copyright (C) 2020 Free Software Foundation, Inc.

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

#ifndef DEBUGINFOD_SUPPORT_H
#define DEBUGINFOD_SUPPORT_H

/* Query debuginfod servers for a source file associated with an
   executable with BUILD_ID.  BUILD_ID can be given as a binary blob or
   a null-terminated string.  If given as a binary blob, BUILD_ID_LEN
   should be the number of bytes.  If given as a null-terminated string,
   BUILD_ID_LEN should be 0.

   SRC_PATH should be the source file's absolute path that includes the
   compilation directory of the CU associated with the source file.
   For example if a CU's compilation directory is `/my/build` and the
   source file path is `/my/source/foo.c`, then SRC_PATH should be
   `/my/build/../source/foo.c`.

   If the file is successfully retrieved, its path on the local machine
   is stored in DESTNAME.  If GDB is not built with debuginfod, this
   function returns -ENOSYS.  */

extern scoped_fd
debuginfod_source_query (const unsigned char *build_id,
			 int build_id_len,
			 const char *src_path,
			 gdb::unique_xmalloc_ptr<char> *destname);

/* Query debuginfod servers for a debug info file with BUILD_ID.
   BUILD_ID can be given as a binary blob or a null-terminated string.
   If given as a binary blob, BUILD_ID_LEN should be the number of bytes.
   If given as a null-terminated string, BUILD_ID_LEN should be 0.

   FILENAME should be the name or path of the main binary associated with
   the separate debug info.  It is used for printing messages to the user.

   If the file is successfully retrieved, its path on the local machine
   is stored in DESTNAME.  If GDB is not built with debuginfod, this
   function returns -ENOSYS.  */

extern scoped_fd
debuginfod_debuginfo_query (const unsigned char *build_id,
			    int build_id_len,
			    const char *filename,
			    gdb::unique_xmalloc_ptr<char> *destname);

#endif /* DEBUGINFOD_SUPPORT_H */
