/* Copyright (C) 2021 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _DBE_JAR_FILE_H
#define _DBE_JAR_FILE_H

#include "Emsg.h"

class Data_window;
class ZipEntry;
template <class ITEM> class Vector;

class DbeJarFile : public DbeMessages
{
public:
  DbeJarFile (const char *jarName);
  ~DbeJarFile ();

  int get_entry (const char *fname);
  long long copy (char *toFileNname, int fromEntryNum);

private:
  void get_entries ();
  int get_EndCentDir (struct EndCentDir *endCentDir);
  char *name;
  Vector<ZipEntry *> *fnames;
  Data_window *dwin;
};
#endif /* _DBE_JAR_FILE_H */
