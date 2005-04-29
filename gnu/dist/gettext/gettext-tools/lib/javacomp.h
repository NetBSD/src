/* Compile a Java program.
   Copyright (C) 2001-2002 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _JAVACOMP_H
#define _JAVACOMP_H

#include <stdbool.h>

/* Compile a Java source file to bytecode.
   java_sources is an array of source file names.
   classpaths is a list of pathnames to be prepended to the CLASSPATH.
   directory is the target directory. The .class file for class X.Y.Z is
   written at directory/X/Y/Z.class. If directory is NULL, the .class
   file is written in the source's directory.
   use_minimal_classpath = true means to ignore the user's CLASSPATH and
   use a minimal one. This is likely to reduce possible problems if the
   user's CLASSPATH contains garbage or a classes.zip file of the wrong
   Java version.
   If verbose, the command to be executed will be printed.
   Return false if OK, true on error.  */
extern bool compile_java_class (const char * const *java_sources,
				unsigned int java_sources_count,
				const char * const *classpaths,
				unsigned int classpaths_count,
				const char *directory,
				bool optimize, bool debug,
				bool use_minimal_classpath,
				bool verbose);

#endif /* _JAVACOMP_H */
