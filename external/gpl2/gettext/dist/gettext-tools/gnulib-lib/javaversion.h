/* Determine the Java version supported by javaexec.
   Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _JAVAVERSION_H
#define _JAVAVERSION_H


#ifdef __cplusplus
extern "C" {
#endif


/* Return information about the Java version used by execute_java_class().
   This is the value of System.getProperty("java.specification.version").
   Some possible values are: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6.  Return NULL if
   the Java version cannot be determined.  */
extern char * javaexec_version (void);


#ifdef __cplusplus
}
#endif


#endif /* _JAVAVERSION_H */
