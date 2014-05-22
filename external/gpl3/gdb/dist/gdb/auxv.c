/* Auxiliary vector support for GDB, the GNU debugger.

   Copyright (C) 2004-2013 Free Software Foundation, Inc.

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

#include "defs.h"
#include "target.h"
#include "gdbtypes.h"
#include "command.h"
#include "inferior.h"
#include "valprint.h"
#include "gdb_assert.h"
#include "gdbcore.h"
#include "observer.h"

#include "auxv.h"
#include "elf/common.h"

#include <unistd.h>
#include <fcntl.h>


/* This function handles access via /proc/PID/auxv, which is a common
   method for native targets.  */

static LONGEST
procfs_xfer_auxv (gdb_byte *readbuf,
		  const gdb_byte *writebuf,
		  ULONGEST offset,
		  LONGEST len)
{
  char *pathname;
  int fd;
  LONGEST n;

  pathname = xstrprintf ("/proc/%d/auxv", PIDGET (inferior_ptid));
  fd = open (pathname, writebuf != NULL ? O_WRONLY : O_RDONLY);
  xfree (pathname);
  if (fd < 0)
    return -1;

  if (offset != (ULONGEST) 0
      && lseek (fd, (off_t) offset, SEEK_SET) != (off_t) offset)
    n = -1;
  else if (readbuf != NULL)
    n = read (fd, readbuf, len);
  else
    n = write (fd, writebuf, len);

  (void) close (fd);

  return n;
}

/* This function handles access via ld.so's symbol `_dl_auxv'.  */

static LONGEST
ld_so_xfer_auxv (gdb_byte *readbuf,
		 const gdb_byte *writebuf,
		 ULONGEST offset,
		 LONGEST len)
{
  struct minimal_symbol *msym;
  CORE_ADDR data_address, pointer_address;
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  size_t ptr_size = TYPE_LENGTH (ptr_type);
  size_t auxv_pair_size = 2 * ptr_size;
  gdb_byte *ptr_buf = alloca (ptr_size);
  LONGEST retval;
  size_t block;

  msym = lookup_minimal_symbol ("_dl_auxv", NULL, NULL);
  if (msym == NULL)
    return -1;

  if (MSYMBOL_SIZE (msym) != ptr_size)
    return -1;

  /* POINTER_ADDRESS is a location where the `_dl_auxv' variable
     resides.  DATA_ADDRESS is the inferior value present in
     `_dl_auxv', therefore the real inferior AUXV address.  */

  pointer_address = SYMBOL_VALUE_ADDRESS (msym);

  /* The location of the _dl_auxv symbol may no longer be correct if
     ld.so runs at a different address than the one present in the
     file.  This is very common case - for unprelinked ld.so or with a
     PIE executable.  PIE executable forces random address even for
     libraries already being prelinked to some address.  PIE
     executables themselves are never prelinked even on prelinked
     systems.  Prelinking of a PIE executable would block their
     purpose of randomizing load of everything including the
     executable.

     If the memory read fails, return -1 to fallback on another
     mechanism for retrieving the AUXV.

     In most cases of a PIE running under valgrind there is no way to
     find out the base addresses of any of ld.so, executable or AUXV
     as everything is randomized and /proc information is not relevant
     for the virtual executable running under valgrind.  We think that
     we might need a valgrind extension to make it work.  This is PR
     11440.  */

  if (target_read_memory (pointer_address, ptr_buf, ptr_size) != 0)
    return -1;

  data_address = extract_typed_address (ptr_buf, ptr_type);

  /* Possibly still not initialized such as during an inferior
     startup.  */
  if (data_address == 0)
    return -1;

  data_address += offset;

  if (writebuf != NULL)
    {
      if (target_write_memory (data_address, writebuf, len) == 0)
	return len;
      else
	return -1;
    }

  /* Stop if trying to read past the existing AUXV block.  The final
     AT_NULL was already returned before.  */

  if (offset >= auxv_pair_size)
    {
      if (target_read_memory (data_address - auxv_pair_size, ptr_buf,
			      ptr_size) != 0)
	return -1;

      if (extract_typed_address (ptr_buf, ptr_type) == AT_NULL)
	return 0;
    }

  retval = 0;
  block = 0x400;
  gdb_assert (block % auxv_pair_size == 0);

  while (len > 0)
    {
      if (block > len)
	block = len;

      /* Reading sizes smaller than AUXV_PAIR_SIZE is not supported.
	 Tails unaligned to AUXV_PAIR_SIZE will not be read during a
	 call (they should be completed during next read with
	 new/extended buffer).  */

      block &= -auxv_pair_size;
      if (block == 0)
	return retval;

      if (target_read_memory (data_address, readbuf, block) != 0)
	{
	  if (block <= auxv_pair_size)
	    return retval;

	  block = auxv_pair_size;
	  continue;
	}

      data_address += block;
      len -= block;

      /* Check terminal AT_NULL.  This function is being called
         indefinitely being extended its READBUF until it returns EOF
         (0).  */

      while (block >= auxv_pair_size)
	{
	  retval += auxv_pair_size;

	  if (extract_typed_address (readbuf, ptr_type) == AT_NULL)
	    return retval;

	  readbuf += auxv_pair_size;
	  block -= auxv_pair_size;
	}
    }

  return retval;
}

/* This function is called like a to_xfer_partial hook, but must be
   called with TARGET_OBJECT_AUXV.  It handles access to AUXV.  */

LONGEST
memory_xfer_auxv (struct target_ops *ops,
		  enum target_object object,
		  const char *annex,
		  gdb_byte *readbuf,
		  const gdb_byte *writebuf,
		  ULONGEST offset,
		  LONGEST len)
{
  gdb_assert (object == TARGET_OBJECT_AUXV);
  gdb_assert (readbuf || writebuf);

   /* ld_so_xfer_auxv is the only function safe for virtual
      executables being executed by valgrind's memcheck.  Using
      ld_so_xfer_auxv during inferior startup is problematic, because
      ld.so symbol tables have not yet been relocated.  So GDB uses
      this function only when attaching to a process.
      */

  if (current_inferior ()->attach_flag != 0)
    {
      LONGEST retval;

      retval = ld_so_xfer_auxv (readbuf, writebuf, offset, len);
      if (retval != -1)
	return retval;
    }

  return procfs_xfer_auxv (readbuf, writebuf, offset, len);
}

/* Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
   Return 0 if *READPTR is already at the end of the buffer.
   Return -1 if there is insufficient buffer for a whole entry.
   Return 1 if an entry was read into *TYPEP and *VALP.  */
static int
default_auxv_parse (struct target_ops *ops, gdb_byte **readptr,
		   gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
{
  const int sizeof_auxv_field = gdbarch_ptr_bit (target_gdbarch ())
				/ TARGET_CHAR_BIT;
  const enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  gdb_byte *ptr = *readptr;

  if (endptr == ptr)
    return 0;

  if (endptr - ptr < sizeof_auxv_field * 2)
    return -1;

  *typep = extract_unsigned_integer (ptr, sizeof_auxv_field, byte_order);
  ptr += sizeof_auxv_field;
  *valp = extract_unsigned_integer (ptr, sizeof_auxv_field, byte_order);
  ptr += sizeof_auxv_field;

  *readptr = ptr;
  return 1;
}

/* Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
   Return 0 if *READPTR is already at the end of the buffer.
   Return -1 if there is insufficient buffer for a whole entry.
   Return 1 if an entry was read into *TYPEP and *VALP.  */
int
target_auxv_parse (struct target_ops *ops, gdb_byte **readptr,
                  gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
{
  struct target_ops *t;

  for (t = ops; t != NULL; t = t->beneath)
    if (t->to_auxv_parse != NULL)
      return t->to_auxv_parse (t, readptr, endptr, typep, valp);
  
  return default_auxv_parse (ops, readptr, endptr, typep, valp);
}


/* Per-inferior data key for auxv.  */
static const struct inferior_data *auxv_inferior_data;

/*  Auxiliary Vector information structure.  This is used by GDB
    for caching purposes for each inferior.  This helps reduce the
    overhead of transfering data from a remote target to the local host.  */
struct auxv_info
{
  LONGEST length;
  gdb_byte *data;
};

/* Handles the cleanup of the auxv cache for inferior INF.  ARG is ignored.
   Frees whatever allocated space there is to be freed and sets INF's auxv cache
   data pointer to NULL.

   This function is called when the following events occur: inferior_appeared,
   inferior_exit and executable_changed.  */

static void
auxv_inferior_data_cleanup (struct inferior *inf, void *arg)
{
  struct auxv_info *info;

  info = inferior_data (inf, auxv_inferior_data);
  if (info != NULL)
    {
      xfree (info->data);
      xfree (info);
      set_inferior_data (inf, auxv_inferior_data, NULL);
    }
}

/* Invalidate INF's auxv cache.  */

static void
invalidate_auxv_cache_inf (struct inferior *inf)
{
  auxv_inferior_data_cleanup (inf, NULL);
}

/* Invalidate current inferior's auxv cache.  */

static void
invalidate_auxv_cache (void)
{
  invalidate_auxv_cache_inf (current_inferior ());
}

/* Fetch the auxv object from inferior INF.  If auxv is cached already,
   return a pointer to the cache.  If not, fetch the auxv object from the
   target and cache it.  This function always returns a valid INFO pointer.  */

static struct auxv_info *
get_auxv_inferior_data (struct target_ops *ops)
{
  struct auxv_info *info;
  struct inferior *inf = current_inferior ();

  info = inferior_data (inf, auxv_inferior_data);
  if (info == NULL)
    {
      info = XZALLOC (struct auxv_info);
      info->length = target_read_alloc (ops, TARGET_OBJECT_AUXV,
					NULL, &info->data);
      set_inferior_data (inf, auxv_inferior_data, info);
    }

  return info;
}

/* Extract the auxiliary vector entry with a_type matching MATCH.
   Return zero if no such entry was found, or -1 if there was
   an error getting the information.  On success, return 1 after
   storing the entry's value field in *VALP.  */
int
target_auxv_search (struct target_ops *ops, CORE_ADDR match, CORE_ADDR *valp)
{
  CORE_ADDR type, val;
  gdb_byte *data;
  gdb_byte *ptr;
  struct auxv_info *info;

  info = get_auxv_inferior_data (ops);

  data = info->data;
  ptr = data;

  if (info->length <= 0)
    return info->length;

  while (1)
    switch (target_auxv_parse (ops, &ptr, data + info->length, &type, &val))
      {
      case 1:			/* Here's an entry, check it.  */
	if (type == match)
	  {
	    *valp = val;
	    return 1;
	  }
	break;
      case 0:			/* End of the vector.  */
	return 0;
      default:			/* Bogosity.  */
	return -1;
      }

  /*NOTREACHED*/
}


/* Print the contents of the target's AUXV on the specified file.  */
int
fprint_target_auxv (struct ui_file *file, struct target_ops *ops)
{
  CORE_ADDR type, val;
  gdb_byte *data;
  gdb_byte *ptr;
  struct auxv_info *info;
  int ents = 0;

  info = get_auxv_inferior_data (ops);

  data = info->data;
  ptr = data;
  if (info->length <= 0)
    return info->length;

  while (target_auxv_parse (ops, &ptr, data + info->length, &type, &val) > 0)
    {
      const char *name = "???";
      const char *description = "";
      enum { dec, hex, str } flavor = hex;

      switch (type)
	{
#define TAG(tag, text, kind) \
	case tag: name = #tag; description = text; flavor = kind; break
	  TAG (AT_NULL, _("End of vector"), hex);
	  TAG (AT_IGNORE, _("Entry should be ignored"), hex);
	  TAG (AT_EXECFD, _("File descriptor of program"), dec);
	  TAG (AT_PHDR, _("Program headers for program"), hex);
	  TAG (AT_PHENT, _("Size of program header entry"), dec);
	  TAG (AT_PHNUM, _("Number of program headers"), dec);
	  TAG (AT_PAGESZ, _("System page size"), dec);
	  TAG (AT_BASE, _("Base address of interpreter"), hex);
	  TAG (AT_FLAGS, _("Flags"), hex);
	  TAG (AT_ENTRY, _("Entry point of program"), hex);
	  TAG (AT_NOTELF, _("Program is not ELF"), dec);
	  TAG (AT_UID, _("Real user ID"), dec);
	  TAG (AT_EUID, _("Effective user ID"), dec);
	  TAG (AT_GID, _("Real group ID"), dec);
	  TAG (AT_EGID, _("Effective group ID"), dec);
	  TAG (AT_CLKTCK, _("Frequency of times()"), dec);
	  TAG (AT_PLATFORM, _("String identifying platform"), str);
	  TAG (AT_HWCAP, _("Machine-dependent CPU capability hints"), hex);
	  TAG (AT_FPUCW, _("Used FPU control word"), dec);
	  TAG (AT_DCACHEBSIZE, _("Data cache block size"), dec);
	  TAG (AT_ICACHEBSIZE, _("Instruction cache block size"), dec);
	  TAG (AT_UCACHEBSIZE, _("Unified cache block size"), dec);
	  TAG (AT_IGNOREPPC, _("Entry should be ignored"), dec);
	  TAG (AT_BASE_PLATFORM, _("String identifying base platform"), str);
	  TAG (AT_RANDOM, _("Address of 16 random bytes"), hex);
	  TAG (AT_EXECFN, _("File name of executable"), str);
	  TAG (AT_SECURE, _("Boolean, was exec setuid-like?"), dec);
	  TAG (AT_SYSINFO, _("Special system info/entry points"), hex);
	  TAG (AT_SYSINFO_EHDR, _("System-supplied DSO's ELF header"), hex);
	  TAG (AT_L1I_CACHESHAPE, _("L1 Instruction cache information"), hex);
	  TAG (AT_L1D_CACHESHAPE, _("L1 Data cache information"), hex);
	  TAG (AT_L2_CACHESHAPE, _("L2 cache information"), hex);
	  TAG (AT_L3_CACHESHAPE, _("L3 cache information"), hex);
	  TAG (AT_SUN_UID, _("Effective user ID"), dec);
	  TAG (AT_SUN_RUID, _("Real user ID"), dec);
	  TAG (AT_SUN_GID, _("Effective group ID"), dec);
	  TAG (AT_SUN_RGID, _("Real group ID"), dec);
	  TAG (AT_SUN_LDELF, _("Dynamic linker's ELF header"), hex);
	  TAG (AT_SUN_LDSHDR, _("Dynamic linker's section headers"), hex);
	  TAG (AT_SUN_LDNAME, _("String giving name of dynamic linker"), str);
	  TAG (AT_SUN_LPAGESZ, _("Large pagesize"), dec);
	  TAG (AT_SUN_PLATFORM, _("Platform name string"), str);
	  TAG (AT_SUN_HWCAP, _("Machine-dependent CPU capability hints"), hex);
	  TAG (AT_SUN_IFLUSH, _("Should flush icache?"), dec);
	  TAG (AT_SUN_CPU, _("CPU name string"), str);
	  TAG (AT_SUN_EMUL_ENTRY, _("COFF entry point address"), hex);
	  TAG (AT_SUN_EMUL_EXECFD, _("COFF executable file descriptor"), dec);
	  TAG (AT_SUN_EXECNAME,
	       _("Canonicalized file name given to execve"), str);
	  TAG (AT_SUN_MMU, _("String for name of MMU module"), str);
	  TAG (AT_SUN_LDDATA, _("Dynamic linker's data segment address"), hex);
	  TAG (AT_SUN_AUXFLAGS,
	       _("AF_SUN_ flags passed from the kernel"), hex);
	}

      fprintf_filtered (file, "%-4s %-20s %-30s ",
			plongest (type), name, description);
      switch (flavor)
	{
	case dec:
	  fprintf_filtered (file, "%s\n", plongest (val));
	  break;
	case hex:
	  fprintf_filtered (file, "%s\n", paddress (target_gdbarch (), val));
	  break;
	case str:
	  {
	    struct value_print_options opts;

	    get_user_print_options (&opts);
	    if (opts.addressprint)
	      fprintf_filtered (file, "%s ", paddress (target_gdbarch (), val));
	    val_print_string (builtin_type (target_gdbarch ())->builtin_char,
			      NULL, val, -1, file, &opts);
	    fprintf_filtered (file, "\n");
	  }
	  break;
	}
      ++ents;
      if (type == AT_NULL)
	break;
    }

  return ents;
}

static void
info_auxv_command (char *cmd, int from_tty)
{
  if (! target_has_stack)
    error (_("The program has no auxiliary information now."));
  else
    {
      int ents = fprint_target_auxv (gdb_stdout, &current_target);

      if (ents < 0)
	error (_("No auxiliary vector found, or failed reading it."));
      else if (ents == 0)
	error (_("Auxiliary vector is empty."));
    }
}


extern initialize_file_ftype _initialize_auxv; /* -Wmissing-prototypes; */

void
_initialize_auxv (void)
{
  add_info ("auxv", info_auxv_command,
	    _("Display the inferior's auxiliary vector.\n\
This is information provided by the operating system at program startup."));

  /* Set an auxv cache per-inferior.  */
  auxv_inferior_data
    = register_inferior_data_with_cleanup (NULL, auxv_inferior_data_cleanup);

  /* Observers used to invalidate the auxv cache when needed.  */
  observer_attach_inferior_exit (invalidate_auxv_cache_inf);
  observer_attach_inferior_appeared (invalidate_auxv_cache_inf);
  observer_attach_executable_changed (invalidate_auxv_cache);
}
