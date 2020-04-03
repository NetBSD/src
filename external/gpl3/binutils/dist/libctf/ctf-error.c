/* Error table.
   Copyright (C) 2019-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <ctf-impl.h>

static const char *const _ctf_errlist[] = {
  "File is not in CTF or ELF format",		     /* ECTF_FMT */
  "BFD error",					     /* ECTF_BFDERR */
  "File uses more recent CTF version than libctf",   /* ECTF_CTFVERS */
  "Ambiguous BFD target",			     /* ECTF_BFD_AMBIGUOUS */
  "Symbol table uses invalid entry size",	     /* ECTF_SYMTAB */
  "Symbol table data buffer is not valid",	     /* ECTF_SYMBAD */
  "String table data buffer is not valid",	     /* ECTF_STRBAD */
  "File data structure corruption detected",	     /* ECTF_CORRUPT */
  "File does not contain CTF data",		     /* ECTF_NOCTFDATA */
  "Buffer does not contain CTF data",		     /* ECTF_NOCTFBUF */
  "Symbol table information is not available",	     /* ECTF_NOSYMTAB */
  "Type information is in parent and unavailable",   /* ECTF_NOPARENT */
  "Cannot import types with different data model",   /* ECTF_DMODEL */
  "File added to link too late",		     /* ECTF_LINKADDEDLATE */
  "Failed to allocate (de)compression buffer",	     /* ECTF_ZALLOC */
  "Failed to decompress CTF data",		     /* ECTF_DECOMPRESS */
  "External string table is not available",	     /* ECTF_STRTAB */
  "String name offset is corrupt",		     /* ECTF_BADNAME */
  "Invalid type identifier",			     /* ECTF_BADID */
  "Type is not a struct or union",		     /* ECTF_NOTSOU */
  "Type is not an enum",			     /* ECTF_NOTENUM */
  "Type is not a struct, union, or enum",	     /* ECTF_NOTSUE */
  "Type is not an integer, float, or enum",	     /* ECTF_NOTINTFP */
  "Type is not an array",			     /* ECTF_NOTARRAY */
  "Type does not reference another type",	     /* ECTF_NOTREF */
  "Input buffer is too small for type name",	     /* ECTF_NAMELEN */
  "No type information available for that name",     /* ECTF_NOTYPE */
  "Syntax error in type name",			     /* ECTF_SYNTAX */
  "Symbol table entry or type is not a function",    /* ECTF_NOTFUNC */
  "No function information available for symbol",    /* ECTF_NOFUNCDAT */
  "Symbol table entry is not a data object",	     /* ECTF_NOTDATA */
  "No type information available for symbol",	     /* ECTF_NOTYPEDAT */
  "No label information available for that name",    /* ECTF_NOLABEL */
  "File does not contain any labels",		     /* ECTF_NOLABELDATA */
  "Feature not supported",			     /* ECTF_NOTSUP */
  "Invalid enum element name",			     /* ECTF_NOENUMNAM */
  "Invalid member name",			     /* ECTF_NOMEMBNAM */
  "CTF container is read-only",			     /* ECTF_RDONLY */
  "Limit on number of dynamic type members reached", /* ECTF_DTFULL */
  "Limit on number of dynamic types reached",	     /* ECTF_FULL */
  "Duplicate member or variable name",		     /* ECTF_DUPLICATE */
  "Conflicting type is already defined",	     /* ECTF_CONFLICT */
  "Attempt to roll back past a ctf_update",	     /* ECTF_OVERROLLBACK */
  "Failed to compress CTF data",		     /* ECTF_COMPRESS */
  "Failed to create CTF archive",		     /* ECTF_ARCREATE */
  "Name not found in CTF archive",		     /* ECTF_ARNNAME */
  "Overflow of type bitness or offset in slice",     /* ECTF_SLICEOVERFLOW */
  "Unknown section number in dump",		     /* ECTF_DUMPSECTUNKNOWN */
  "Section changed in middle of dump",		     /* ECTF_DUMPSECTCHANGED */
  "Feature not yet implemented",		     /* ECTF_NOTYET */
  "Internal error in link",			     /* ECTF_INTERNAL */
  "Type not representable in CTF"		     /* ECTF_NONREPRESENTABLE */
};

static const int _ctf_nerr = sizeof (_ctf_errlist) / sizeof (_ctf_errlist[0]);

const char *
ctf_errmsg (int error)
{
  const char *str;

  if (error >= ECTF_BASE && (error - ECTF_BASE) < _ctf_nerr)
    str = _ctf_errlist[error - ECTF_BASE];
  else
    str = ctf_strerror (error);

  return (str ? str : "Unknown error");
}

int
ctf_errno (ctf_file_t * fp)
{
  return fp->ctf_errno;
}
