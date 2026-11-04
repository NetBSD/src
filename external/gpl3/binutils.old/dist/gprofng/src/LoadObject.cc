/* Copyright (C) 2021-2025 Free Software Foundation, Inc.
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

#include "config.h"
#include <errno.h>
#include <libgen.h>

#include "util.h"
#include "StringBuilder.h"
#include "Application.h"
#include "DbeSession.h"
#include "Experiment.h"
#include "Exp_Layout.h"
#include "DataObject.h"
#include "Elf.h"
#include "Function.h"
#include "Module.h"
#include "ClassFile.h"
#include "Stabs.h"
#include "LoadObject.h"
#include "dbe_types.h"
#include "DbeFile.h"
#include "ExpGroup.h"

enum
{
  LO_InstHTableSize     = 4096,
  HTableSize            = 1024
};

LoadObject *
LoadObject::create_item (const char *nm, int64_t chksum)
{
  LoadObject *lo = new LoadObject (nm);
  lo->checksum = chksum;
  dbeSession->append (lo);
  return lo;
}

LoadObject *
LoadObject::create_item (const char *nm, const char *_runTimePath, DbeFile *df)
{
  LoadObject *lo = new LoadObject (nm);
  lo->runTimePath = dbe_strdup (_runTimePath);
  lo->dbeFile->orig_location = dbe_strdup (_runTimePath);
  if (df)
    {
      if ((df->filetype & DbeFile::F_JAR_FILE) != 0)
	{
	  if (lo->dbeFile->find_in_jar_file (nm, df->get_jar_file ()))
	    {
	      lo->dbeFile->inArchive = df->inArchive;
	      lo->dbeFile->container = df;
	    }
	}
      else
	{
	  lo->dbeFile->set_location (df->get_location ());
	  lo->dbeFile->sbuf = df->sbuf;
	  lo->dbeFile->inArchive = df->inArchive;
	}
    }
  dbeSession->append (lo);
  return lo;
}

LoadObject::LoadObject (const char *loname)
{
  flags = 0;
  size = 0;
  type = SEG_UNKNOWN;
  isReadStabs = false;
  instHTable = new DbeInstr*[LO_InstHTableSize];
  for (int i = 0; i < LO_InstHTableSize; i++)
    instHTable[i] = NULL;

  functions = new Vector<Function*>;
  funcHTable = new Function*[HTableSize];
  for (int i = 0; i < HTableSize; i++)
    funcHTable[i] = NULL;

  seg_modules = new Vector<Module*>;
  modules = new HashMap<char*, Module*>;
  platform = Unknown;
  noname = dbeSession->createUnknownModule (this);
  modules->put (noname->get_name (), noname);
  pathname = NULL;
  runTimePath = NULL;
  objStabs = NULL;
  firstExp = NULL;
  seg_modules_map = NULL;
  comp_funcs = NULL;
  warnq = new Emsgqueue (NTXT ("lo_warnq"));
  commentq = new Emsgqueue (NTXT ("lo_commentq"));
  elf_lo = NULL;
  elf_inited = false;
  checksum = 0;
  isUsed = false;
  h_function = NULL;
  h_instr = NULL;

  char *nm = (char *) loname;
  if (strncmp (nm, NTXT ("./"), 2) == 0)
    nm += 2;
  set_name (nm);
  dbeFile = new DbeFile (nm);
  dbeFile->filetype |= DbeFile::F_LOADOBJ | DbeFile::F_FILE;
}

LoadObject::~LoadObject ()
{
  delete seg_modules_map;
  delete functions;
  delete[] instHTable;
  delete[] funcHTable;
  delete seg_modules;
  delete modules;
  delete elf_lo;
  free (pathname);
  free (runTimePath);
  delete objStabs;
  delete warnq;
  delete commentq;
  delete h_instr;
}

Elf *
LoadObject::get_elf ()
{
  if (elf_lo == NULL)
    {
      if (dbeFile->get_need_refind ())
	elf_inited = false;
      if (elf_inited)
	return NULL;
      elf_inited = true;
      char *fnm = dbeFile->get_location ();
      if (fnm == NULL)
	{
	  append_msg (CMSG_ERROR, GTXT ("Cannot find file: `%s'"),
		      dbeFile->get_name ());
	  return NULL;
	}
      Elf::Elf_status st = Elf::ELF_ERR_CANT_OPEN_FILE;
      elf_lo = Elf::elf_begin (fnm, &st);
      if (elf_lo == NULL)
	switch (st)
	  {
	  case Elf::ELF_ERR_CANT_OPEN_FILE:
	    append_msg (CMSG_ERROR, GTXT ("Cannot open ELF file `%s'"), fnm);
	    return NULL;
	  case Elf::ELF_ERR_BAD_ELF_FORMAT:
	  default:
	    append_msg (CMSG_ERROR, GTXT ("Cannot read ELF header of `%s'"),
			fnm);
	    return NULL;
	  }
      if (dbeFile->inArchive)
	{
	  // Try to find gnu_debug and gnu debug_alt files in archive
	  char *nm = dbe_sprintf ("%s_debug", fnm);
          elf_lo->gnu_debug_file = Elf::elf_begin (nm);
	  free (nm);
	  if (elf_lo->gnu_debug_file)
	    {
	      nm = dbe_sprintf ("%s_debug_alt", fnm);
	      elf_lo->gnu_debug_file->gnu_debugalt_file = Elf::elf_begin (nm);
	      free (nm);
	    }
	  nm = dbe_sprintf ("%s_alt", fnm);
	  elf_lo->gnu_debugalt_file = Elf::elf_begin (nm);
	  free (nm);
	}
      else if (checksum != 0 && elf_lo->elf_checksum () != 0 &&
	  checksum != elf_lo->elf_checksum ())
	{
	  char *msg = dbe_sprintf (GTXT ("%s has an unexpected checksum value;"
				  "perhaps it was rebuilt. File ignored"),
				  dbeFile->get_location ());
	  commentq->append (new Emsg (CMSG_ERROR, msg));
	  delete msg;
	  delete elf_lo;
	  elf_lo = NULL;
	  return NULL;
	}
      elf_lo->find_gnu_debug_files ();
      elf_lo->find_ancillary_files (get_pathname ());
    }
  return elf_lo;
}

Stabs *
LoadObject::openDebugInfo (Stabs::Stab_status *stp)
{
  if (objStabs == NULL)
    {
      Stabs::Stab_status st = Stabs::DBGD_ERR_BAD_ELF_LIB;
      Elf *elf = get_elf ();
      if (elf)
	{
	  objStabs = new Stabs (elf, get_pathname ());
	  st = objStabs->get_status ();
	  if (st != Stabs::DBGD_ERR_NONE)
	    {
	      delete objStabs;
	      objStabs = NULL;
	    }
	}
      if (stp)
	*stp = st;
    }
  return objStabs;
}

uint64_t
LoadObject::get_addr ()
{
  return MAKE_ADDRESS (seg_idx, 0);
}

bool
LoadObject::compare (const char *_path, int64_t _checksum)
{
  return _checksum == checksum && dbe_strcmp (_path, get_pathname ()) == 0;
}

int
LoadObject::compare (const char *_path, const char *_runTimePath, DbeFile *df)
{
  int ret = 0;
  if (dbe_strcmp (_path, get_pathname ()) != 0)
    return ret;
  ret |= CMP_PATH;
  if (_runTimePath)
    {
      if (dbe_strcmp (_runTimePath, runTimePath) != 0)
	return ret;
      ret |= CMP_RUNTIMEPATH;
    }
  if (df && dbeFile->compare (df))
    ret |= CMP_CHKSUM;
  return ret;
}

void
LoadObject::set_platform (Platform_t pltf, int wsz)
{
  switch (pltf)
    {
    case Sparc:
    case Sparcv9:
    case Sparcv8plus:
      platform = (wsz == W64) ? Sparcv9 : Sparc;
      break;
    case Intel:
    case Amd64:
      platform = (wsz == W64) ? Amd64 : Intel;
      break;
    default:
      platform = pltf;
      break;
    }
};

void
LoadObject::set_name (char *string)
{
  char *p;
  pathname = dbe_strdup (string);

  p = get_basename (pathname);
  if (p[0] == '<')
    name = dbe_strdup (p);
  else    // set a short name  to "<basename>"
    name = dbe_sprintf (NTXT ("<%s>"), p);
}

void
LoadObject::dump_functions (FILE *out)
{
  int index;
  Function *fitem;
  char *sname, *mname;
  if (platform == Java)
    {
      JMethod *jmthd;
      Vector<JMethod*> *jmethods = (Vector<JMethod*>*)functions;
      Vec_loop (JMethod*, jmethods, index, jmthd)
      {
	fprintf (out, "id %6llu, @0x%llx sz-%lld %s (module = %s)\n",
		 (unsigned long long) jmthd->id, (long long) jmthd->get_mid (),
		 (long long) jmthd->size, jmthd->get_name (),
		 jmthd->module ? jmthd->module->file_name : noname->file_name);
      }
    }
  else
    {
      Vec_loop (Function*, functions, index, fitem)
      {
	if (fitem->alias && fitem->alias != fitem)
	  fprintf (out, "id %6llu, @0x%llx -        %s == alias of '%s'\n",
		   (ull_t) fitem->id, (ull_t) fitem->img_offset,
		   fitem->get_mangled_name (), fitem->alias->get_mangled_name ());
	else
	  {
	    mname = fitem->module ? fitem->module->file_name : noname->file_name;
	    sname = fitem->getDefSrcName ();
	    fprintf (out, "id %6llu, @0x%llx-0x%llx sz-%lld", (ull_t) fitem->id,
		    (ull_t) fitem->img_offset,
		    (ull_t) (fitem->img_offset + fitem->size),
		    (ll_t) fitem->size);
	    if (fitem->save_addr != 0)
	      fprintf (out, " [save 0x%llx]", (ull_t) fitem->save_addr);
	    if (strcmp (fitem->get_mangled_name (), fitem->get_name ()) != 0)
	      fprintf (out, " [%s]", fitem->get_mangled_name ());
	    fprintf (out, " %s (module = %s)", fitem->get_name (), mname);
	    if (sname && strcmp (basename (sname), basename (mname)) != 0)
	      fprintf (out, " (Source = %s)", sname);
	    fprintf (out, "\n");
	  }
      }
    }
}

int
LoadObject::get_index (Function *func)
{
  Function *fp;
  uint64_t offset;
  int x;
  int left = 0;
  int right = functions->size () - 1;
  offset = func->img_offset;
  while (left <= right)
    {
      x = (left + right) / 2;
      fp = functions->fetch (x);

      if (left == right)
	{
	  if (offset >= fp->img_offset + fp->size)
	    return -1;
	  if (offset >= fp->img_offset)
	    return x;
	  return -1;
	}
      if (offset < fp->img_offset)
	right = x - 1;
      else if (offset >= fp->img_offset + fp->size)
	left = x + 1;
      else
	return x;
    }
  return -1;
}

char *
LoadObject::get_alias (Function *func)
{
  Function *fp, *alias;
  int index, nsize;
  static char buf[1024];
  if (func->img_offset == 0 || func->alias == NULL)
    return NULL;
  int fid = get_index (func);
  if (fid == -1)
    return NULL;

  nsize = functions->size ();
  alias = func->alias;
  for (index = fid; index < nsize; index++)
    {
      fp = functions->fetch (index);
      if (fp->alias != alias)
	{
	  fid = index;
	  break;
	}
    }

  *buf = '\0';
  for (index--; index >= 0; index--)
    {
      fp = functions->fetch (index);
      if (fp->alias != alias)
	break;
      if (fp != alias)
	{
	  size_t len = strlen (buf);
	  if (*buf != '\0')
	    {
	      snprintf (buf + len, sizeof (buf) - len, NTXT (", "));
	      len = strlen (buf);
	    }
	  snprintf (buf + len, sizeof (buf) - len, "%s", fp->get_name ());
	}
    }
  return buf;
}

DbeInstr*
LoadObject::find_dbeinstr (uint64_t file_off)
{
  int hash = (((int) file_off) >> 2) & (LO_InstHTableSize - 1);
  DbeInstr *instr = instHTable[hash];
  if (instr && instr->img_offset == file_off)
    return instr;
  Function *fp = find_function (file_off);
  if (fp == NULL)
    fp = dbeSession->get_Unknown_Function ();
  uint64_t func_off = file_off - fp->img_offset;
  instr = fp->find_dbeinstr (0, func_off);
  instHTable[hash] = instr;
  return instr;
}

Function *
LoadObject::find_function (uint64_t foff)
{
  // Look up in the hash table
  int hash = (((int) foff) >> 6) & (HTableSize - 1);
  Function *func = funcHTable[hash];
  if (func && foff >= func->img_offset && foff < func->img_offset + func->size)
    return func->alias ? func->alias : func;

  // Use binary search
  func = NULL;
  int left = 0;
  int right = functions->size () - 1;
  while (left <= right)
    {
      int x = (left + right) / 2;
      Function *fp = functions->fetch (x);
      assert (fp != NULL);

      if (foff < fp->img_offset)
	right = x - 1;
      else if (foff >= fp->img_offset + fp->size)
	left = x + 1;
      else
	{
	  func = fp;
	  break;
	}
    }

  // Plug the hole with a static function
  char *func_name = NULL;
  Size low_bound = 0, high_bound = 0;
  if (func == NULL)
    {
      int last = functions->size () - 1;
      uint64_t usize = size < 0 ? 0 : (uint64_t) size;
      if (last < 0)
	high_bound = foff >= usize ? foff : usize;
      else if (left == 0)
	high_bound = functions->fetch (left)->img_offset;
      else if (left < last)
	{
	  Function *fp = functions->fetch (left - 1);
	  low_bound = fp->img_offset + fp->size;
	  high_bound = functions->fetch (left)->img_offset;
	}
      else
	{
	  Function *fp = functions->fetch (last);
	  if (fp->flags & FUNC_FLAG_SIMULATED)
	    {
	      // Function is already created
	      func = fp;
	      uint64_t sz = func->size < 0 ? 0 : func->size;
	      if (sz + func->img_offset < foff)
		func->size = foff - func->img_offset;
	    }
	  else
	    {
	      low_bound = fp->img_offset + fp->size;
	      high_bound = foff > usize ? foff : usize;
	    }
	}
    }

  if (func == NULL)
    {
      func = dbeSession->createFunction ();
      func->flags |= FUNC_FLAG_SIMULATED;
      func->size = (unsigned) (high_bound - low_bound);
      func->module = noname;
      func->img_fname = get_pathname ();
      func->img_offset = (off_t) low_bound;
      noname->functions->append (func); // unordered
      if (func_name == NULL)
	func_name = dbe_sprintf (GTXT ("<static>@0x%llx (%s)"), low_bound,
				     name);
      func->set_name (func_name);
      free (func_name);

      // now insert the function
      functions->insert (left, func);
    }

  // Update the hash table
  funcHTable[hash] = func;
  return func->alias ? func->alias : func;
}

static void
fixFuncAlias (Vector<Function*> *SymLst)
{
  int ind, i, k;
  int64_t len, bestLen, maxSize;
  Function *sym, *bestAlias;

  // XXXX it is a clone of Stabs::fixSymtabAlias()
  ind = SymLst->size () - 1;
  for (i = 0; i < ind; i++)
    {
      bestAlias = SymLst->fetch (i);
      if (bestAlias->img_offset == 0) // Ignore this bad symbol
	continue;
      sym = SymLst->fetch (i + 1);
      if (bestAlias->img_offset != sym->img_offset)
	{
	  if (bestAlias->size == 0
	      || sym->img_offset < bestAlias->img_offset + bestAlias->size)
	    bestAlias->size = (int) (sym->img_offset - bestAlias->img_offset);
	  continue;
	}

      // Find a "best" alias
      bestLen = strlen (bestAlias->get_name ());
      maxSize = bestAlias->size;
      for (k = i + 1; k <= ind; k++)
	{
	  sym = SymLst->fetch (k);
	  if (bestAlias->img_offset != sym->img_offset)
	    { // no more aliases
	      if ((maxSize == 0) ||
		  (sym->img_offset < bestAlias->img_offset + maxSize))
		maxSize = sym->img_offset - bestAlias->img_offset;
	      break;
	    }
	  if (maxSize < sym->size)
	    maxSize = sym->size;
	  len = strlen (sym->get_name ());
	  if (len < bestLen)
	    {
	      bestAlias = sym;
	      bestLen = len;
	    }
	}
      for (; i < k; i++)
	{
	  sym = SymLst->fetch (i);
	  sym->alias = bestAlias;
	  sym->size = maxSize;
	}
      i--;
    }
}

void
LoadObject::post_process_functions ()
{
  if ((flags & SEG_FLAG_DYNAMIC) != 0 || platform == Java)
    return;

  char *msg = GTXT ("Processing Load Object Data");
  if (dbeSession->is_interactive ())
    theApplication->set_progress (1, msg);

  // First sort the functions
  functions->sort (func_compare);
  fixFuncAlias (functions);

  Module *mitem;
  int index;
  Vec_loop (Module*, seg_modules, index, mitem)
  {
    mitem->functions->sort (func_compare);
  }

  // Find any derived functions, and set their derivedNode
  Function *fitem;
  Vec_loop (Function*, functions, index, fitem)
  {
    if (dbeSession->is_interactive () && index % 5000 == 0)
      {
	int percent = (int) (100.0 * index / functions->size ());
	theApplication->set_progress (percent, (percent != 0) ? NULL : msg);
      }
    fitem->findDerivedFunctions ();
  }

  // 4987698: get the alias name for MAIN_
  fitem = find_function (NTXT ("MAIN_"));
  if (fitem)
    fitem->module->read_stabs ();
  fitem = find_function (NTXT ("@plt"));
  if (fitem)
    fitem->flags |= FUNC_FLAG_PLT;
  if (dbeSession->is_interactive ())
    theApplication->set_progress (0, NTXT (""));
}

int
LoadObject::func_compare (const void *p1, const void *p2)
{
  Function *f1 = *(Function **) p1;
  Function *f2 = *(Function **) p2;
  if (f1->img_offset != f2->img_offset)
    return f1->img_offset > f2->img_offset ? 1 : -1;

  // annotated source not available for weak symbols.
  if ((f1->module->flags & MOD_FLAG_UNKNOWN) != 0)
    {
      if ((f2->module->flags & MOD_FLAG_UNKNOWN) == 0)
	return -1;
    }
  else if ((f2->module->flags & MOD_FLAG_UNKNOWN) != 0)
    return 1;
  return strcoll (f1->get_name (), f2->get_name ());
}

Function *
LoadObject::find_function (char *fname)
{
  Function *fitem;
  int index;
  Vec_loop (Function*, functions, index, fitem)
  {
    if (strcmp (fitem->get_name (), fname) == 0)
      return fitem;
  }
  return (Function *) NULL;
}

Function *
LoadObject::find_function (char *fname, unsigned int chksum)
{
  Function *fitem;
  int index;
  Vec_loop (Function*, functions, index, fitem)
  {
    if (fitem->chksum == chksum && strcmp (fitem->get_name (), fname) == 0)
      return fitem;
  }
  return (Function *) NULL;
}

Module *
LoadObject::find_module (char *mname)
{
  for (int i = 0, sz = seg_modules ? seg_modules->size () : 0; i < sz; i++)
    {
      Module *module = seg_modules->fetch (i);
      if (strcmp (module->get_name (), mname) == 0)
	return module;
    }
  return (Module *) NULL;
}

LoadObject::Arch_status
LoadObject::sync_read_stabs ()
{
  Arch_status st = ARCHIVE_SUCCESS;
  if (!isReadStabs)
    {
      aquireLock ();
      if (!isReadStabs)
	{
	  st = read_stabs ();
	  post_process_functions ();
	  isReadStabs = true;
	}
      releaseLock ();
    }
  return st;
}

LoadObject::Arch_status
LoadObject::read_stabs ()
{
  if ((dbeFile->filetype & DbeFile::F_FICTION) != 0)
    return ARCHIVE_SUCCESS;
  Arch_status stabs_status = ARCHIVE_ERR_OPEN;
  if (platform == Java)
    {
      Module *cf = NULL;
      for (int i = 0, sz = seg_modules ? seg_modules->size () : 0; i < sz; i++)
	{
	  Module *mod = seg_modules->fetch (i);
	  if (mod->dbeFile
	      && (mod->dbeFile->filetype & DbeFile::F_JAVACLASS) != 0)
	    {
	      cf = mod;
	      break;
	    }
	}
      if (cf)
	{
	  int status = cf->readFile ();
	  switch (status)
	    {
	    case Module::AE_OK:
	      stabs_status = ARCHIVE_SUCCESS;
	      break;
	    case Module::AE_NOSTABS:
	      stabs_status = ARCHIVE_NO_STABS;
	      break;
	    case Module::AE_NOTREAD:
	    default:
	      stabs_status = ARCHIVE_ERR_OPEN;
	      break;
	    }
	}
    }
  else if (strchr (pathname, '`'))
    return ARCHIVE_SUCCESS;
  else
    {
      Elf *elf = get_elf ();
      if (elf == NULL)
	{
	  char *msg = dbe_sprintf (GTXT ("Can't open file: %s"),
				   dbeFile->get_name ());
	  warnq->append (new Emsg (CMSG_ERROR, msg));
	  delete msg;
	  return ARCHIVE_ERR_OPEN;
	}
      Stabs::Stab_status status = Stabs::DBGD_ERR_CANT_OPEN_FILE;
      if (openDebugInfo (&status))
	{
	  status = objStabs->read_archive (this);
	  isRelocatable = objStabs->is_relocatable ();
	  size = objStabs->get_textsz ();
	  platform = objStabs->get_platform ();
	  wsize = objStabs->get_class ();
	}

      switch (status)
	{
	case Stabs::DBGD_ERR_NONE:
	  stabs_status = ARCHIVE_SUCCESS;
	  break;
	case Stabs::DBGD_ERR_CANT_OPEN_FILE:
	  stabs_status = ARCHIVE_ERR_OPEN;
	  break;
	case Stabs::DBGD_ERR_BAD_ELF_LIB:
	case Stabs::DBGD_ERR_BAD_ELF_FORMAT:
	  stabs_status = ARCHIVE_BAD_STABS;
	  break;
	case Stabs::DBGD_ERR_NO_STABS:
	  stabs_status = ARCHIVE_NO_STABS;
	  break;
	case Stabs::DBGD_ERR_NO_DWARF:
	  stabs_status = ARCHIVE_NO_DWARF;
	  break;
	default:
	  stabs_status = ARCHIVE_BAD_STABS;
	  break;
	}
    }
  return stabs_status;
}

char *
LoadObject::status_str (Arch_status rv, char */*arg*/)
{
  switch (rv)
    {
    case ARCHIVE_SUCCESS:
    case ARCHIVE_EXIST:
      return NULL;
    case ARCHIVE_BAD_STABS:
      return dbe_sprintf (GTXT ("Error: unable to read symbol table of %s"),
			  name);
    case ARCHIVE_ERR_SEG:
      return dbe_sprintf (GTXT ("Error: unable to read load object file %s"),
			  pathname);
    case ARCHIVE_ERR_OPEN:
      return dbe_sprintf (GTXT ("Error: unable to open file %s"),
			  pathname);
    case ARCHIVE_ERR_MAP:
      return dbe_sprintf (GTXT ("Error: unable to map file %s"),
			  pathname);
    case ARCHIVE_WARN_CHECKSUM:
      return dbe_sprintf (GTXT ("Note: checksum differs from that recorded in experiment for %s"),
			  name);
    case ARCHIVE_WARN_MTIME:
      return dbe_sprintf (GTXT ("Warning: last-modified time differs from that recorded in experiment for %s"),
			  name);
    case ARCHIVE_WARN_HOST:
      return dbe_sprintf (GTXT ("Try running er_archive -F on the experiment, on the host where it was recorded"));
    case ARCHIVE_ERR_VERSION:
      return dbe_sprintf (GTXT ("Error: Wrong version of archive for %s"),
			  pathname);
    case ARCHIVE_NO_STABS:
      return dbe_sprintf (GTXT ("Note: no stabs or dwarf information in %s"),
			  name);
    case ARCHIVE_WRONG_ARCH:
#if ARCH(SPARC)
      return dbe_sprintf (GTXT ("Error: file %s is built for Intel, and can't be read on SPARC"),
			  name);
#else
      return dbe_sprintf (GTXT ("Error: file %s is built for SPARC, and can't be read on Intel"),
			  name);
#endif
    case ARCHIVE_NO_LIBDWARF:
      return dbe_strdup (GTXT ("Warning: no libdwarf found to read DWARF symbol tables"));
    case ARCHIVE_NO_DWARF:
      return dbe_sprintf (GTXT ("Note: no DWARF symbol table in %s"), name);
    default:
      return dbe_sprintf (GTXT ("Warning: unexpected archive error %d"),
			  (int) rv);
    }
}

uint32_t
LoadObject::get_checksum ()
{
  char *errmsg = NULL;
  uint32_t crcval = get_cksum (pathname, &errmsg);
  if (0 == crcval && errmsg)
    {
      warnq->append (new Emsg (CMSG_ERROR, errmsg));
      free (errmsg);
    }
  return crcval;
}

static char*
get_module_map_key (Module *mod)
{
  return mod->lang_code == Sp_lang_java ? mod->get_name () : mod->file_name;
}

Module *
LoadObject::get_comparable_Module (Module *mod)
{
  if (mod->loadobject == this)
    return mod;
  if (get_module_map_key (mod) == NULL)
    return NULL;
  if (seg_modules_map == NULL)
    {
      seg_modules_map = new HashMap<char*, Module*>;
      for (int i = 0; i < seg_modules->size (); i++)
	{
	  Module *m = seg_modules->fetch (i);
	  char *key = get_module_map_key (m);
	  if (key)
	    {
	      seg_modules_map->put (m->file_name, m);
	      char *bname = get_basename (key);
	      if (bname != key)
		seg_modules_map->put (bname, m);
	    }
	}
    }

  char *key = get_module_map_key (mod);
  Module *cmpMod = seg_modules_map->get (key);
  if (cmpMod && cmpMod->comparable_objs == NULL)
    return cmpMod;
  char *bname = get_basename (key);
  if (bname != key)
    {
      cmpMod = seg_modules_map->get (bname);
      if (cmpMod && cmpMod->comparable_objs == NULL)
	return cmpMod;
    }
  return NULL;
}

Vector<Histable*> *
LoadObject::get_comparable_objs ()
{
  update_comparable_objs ();
  if (comparable_objs || dbeSession->expGroups->size () <= 1)
    return comparable_objs;
  comparable_objs = new Vector<Histable*>(dbeSession->expGroups->size ());
  for (int i = 0, sz = dbeSession->expGroups->size (); i < sz; i++)
    {
      ExpGroup *gr = dbeSession->expGroups->fetch (i);
      Histable *h = gr->get_comparable_loadObject (this);
      comparable_objs->append (h);
      if (h)
	h->comparable_objs = comparable_objs;
    }
  dump_comparable_objs ();
  return comparable_objs;
}

void
LoadObject::append_module (Module *mod)
{
  seg_modules->append (mod);
  if (seg_modules_map == NULL)
    seg_modules_map = new HashMap<char*, Module*>;
  char *key = get_module_map_key (mod);
  if (key)
    {
      seg_modules_map->put (key, mod);
      char *bname = get_basename (key);
      if (bname != key)
	seg_modules_map->put (bname, mod);
    }
}

// LIBRARY_VISIBILITY
Function *
LoadObject::get_hide_function ()
{
  if (h_function == NULL)
    h_function = dbeSession->create_hide_function (this);
  return h_function;
}

DbeInstr *
LoadObject::get_hide_instr (DbeInstr *instr)
{
  if (h_instr == NULL)
    {
      Function *hf = get_hide_function ();
      h_instr = hf->create_hide_instr (instr);
    }
  return h_instr;
}
