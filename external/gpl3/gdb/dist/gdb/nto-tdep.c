/* nto-tdep.c - general QNX Neutrino target functionality.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

   Contributed by QNX Software Systems Ltd.

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
#include <sys/stat.h>
#include <string.h>
#include "nto-tdep.h"
#include "top.h"
#include "inferior.h"
#include "gdbarch.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "solib-svr4.h"
#include "gdbcore.h"
#include "objfiles.h"

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

#ifdef __CYGWIN__
static char default_nto_target[] = "C:\\QNXsdk\\target\\qnx6";
#elif defined(__sun__) || defined(linux)
static char default_nto_target[] = "/opt/QNXsdk/target/qnx6";
#else
static char default_nto_target[] = "";
#endif

struct nto_target_ops current_nto_target;

static char *
nto_target (void)
{
  char *p = getenv ("QNX_TARGET");

#ifdef __CYGWIN__
  static char buf[PATH_MAX];
  if (p)
    cygwin_conv_path (CCP_WIN_A_TO_POSIX, p, buf, PATH_MAX);
  else
    cygwin_conv_path (CCP_WIN_A_TO_POSIX, default_nto_target, buf, PATH_MAX);
  return buf;
#else
  return p ? p : default_nto_target;
#endif
}

/* Take a string such as i386, rs6000, etc. and map it onto CPUTYPE_X86,
   CPUTYPE_PPC, etc. as defined in nto-share/dsmsgs.h.  */
int
nto_map_arch_to_cputype (const char *arch)
{
  if (!strcmp (arch, "i386") || !strcmp (arch, "x86"))
    return CPUTYPE_X86;
  if (!strcmp (arch, "rs6000") || !strcmp (arch, "powerpc"))
    return CPUTYPE_PPC;
  if (!strcmp (arch, "mips"))
    return CPUTYPE_MIPS;
  if (!strcmp (arch, "arm"))
    return CPUTYPE_ARM;
  if (!strcmp (arch, "sh"))
    return CPUTYPE_SH;
  return CPUTYPE_UNKNOWN;
}

int
nto_find_and_open_solib (char *solib, unsigned o_flags, char **temp_pathname)
{
  char *buf, *arch_path, *nto_root, *endian;
  const char *base;
  const char *arch;
  int arch_len, len, ret;
#define PATH_FMT \
  "%s/lib:%s/usr/lib:%s/usr/photon/lib:%s/usr/photon/dll:%s/lib/dll"

  nto_root = nto_target ();
  if (strcmp (gdbarch_bfd_arch_info (target_gdbarch ())->arch_name, "i386") == 0)
    {
      arch = "x86";
      endian = "";
    }
  else if (strcmp (gdbarch_bfd_arch_info (target_gdbarch ())->arch_name,
		   "rs6000") == 0
	   || strcmp (gdbarch_bfd_arch_info (target_gdbarch ())->arch_name,
		   "powerpc") == 0)
    {
      arch = "ppc";
      endian = "be";
    }
  else
    {
      arch = gdbarch_bfd_arch_info (target_gdbarch ())->arch_name;
      endian = gdbarch_byte_order (target_gdbarch ())
	       == BFD_ENDIAN_BIG ? "be" : "le";
    }

  /* In case nto_root is short, add strlen(solib)
     so we can reuse arch_path below.  */

  arch_len = (strlen (nto_root) + strlen (arch) + strlen (endian) + 2
	      + strlen (solib));
  arch_path = alloca (arch_len);
  xsnprintf (arch_path, arch_len, "%s/%s%s", nto_root, arch, endian);

  len = strlen (PATH_FMT) + strlen (arch_path) * 5 + 1;
  buf = alloca (len);
  xsnprintf (buf, len, PATH_FMT, arch_path, arch_path, arch_path, arch_path,
	     arch_path);

  base = lbasename (solib);
  ret = openp (buf, OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH, base, o_flags,
	       temp_pathname);
  if (ret < 0 && base != solib)
    {
      xsnprintf (arch_path, arch_len, "/%s", solib);
      ret = open (arch_path, o_flags, 0);
      if (temp_pathname)
	{
	  if (ret >= 0)
	    *temp_pathname = gdb_realpath (arch_path);
	  else
	    *temp_pathname = NULL;
	}
    }
  return ret;
}

void
nto_init_solib_absolute_prefix (void)
{
  char buf[PATH_MAX * 2], arch_path[PATH_MAX];
  char *nto_root, *endian;
  const char *arch;

  nto_root = nto_target ();
  if (strcmp (gdbarch_bfd_arch_info (target_gdbarch ())->arch_name, "i386") == 0)
    {
      arch = "x86";
      endian = "";
    }
  else if (strcmp (gdbarch_bfd_arch_info (target_gdbarch ())->arch_name,
		   "rs6000") == 0
	   || strcmp (gdbarch_bfd_arch_info (target_gdbarch ())->arch_name,
		   "powerpc") == 0)
    {
      arch = "ppc";
      endian = "be";
    }
  else
    {
      arch = gdbarch_bfd_arch_info (target_gdbarch ())->arch_name;
      endian = gdbarch_byte_order (target_gdbarch ())
	       == BFD_ENDIAN_BIG ? "be" : "le";
    }

  xsnprintf (arch_path, sizeof (arch_path), "%s/%s%s", nto_root, arch, endian);

  xsnprintf (buf, sizeof (buf), "set solib-absolute-prefix %s", arch_path);
  execute_command (buf, 0);
}

char **
nto_parse_redirection (char *pargv[], const char **pin, const char **pout, 
		       const char **perr)
{
  char **argv;
  char *in, *out, *err, *p;
  int argc, i, n;

  for (n = 0; pargv[n]; n++);
  if (n == 0)
    return NULL;
  in = "";
  out = "";
  err = "";

  argv = xcalloc (n + 1, sizeof argv[0]);
  argc = n;
  for (i = 0, n = 0; n < argc; n++)
    {
      p = pargv[n];
      if (*p == '>')
	{
	  p++;
	  if (*p)
	    out = p;
	  else
	    out = pargv[++n];
	}
      else if (*p == '<')
	{
	  p++;
	  if (*p)
	    in = p;
	  else
	    in = pargv[++n];
	}
      else if (*p++ == '2' && *p++ == '>')
	{
	  if (*p == '&' && *(p + 1) == '1')
	    err = out;
	  else if (*p)
	    err = p;
	  else
	    err = pargv[++n];
	}
      else
	argv[i++] = pargv[n];
    }
  *pin = in;
  *pout = out;
  *perr = err;
  return argv;
}

/* The struct lm_info, lm_addr, and nto_truncate_ptr are copied from
   solib-svr4.c to support nto_relocate_section_addresses
   which is different from the svr4 version.  */

/* Link map info to include in an allocated so_list entry */

struct lm_info
  {
    /* Pointer to copy of link map from inferior.  The type is char *
       rather than void *, so that we may use byte offsets to find the
       various fields without the need for a cast.  */
    gdb_byte *lm;

    /* Amount by which addresses in the binary should be relocated to
       match the inferior.  This could most often be taken directly
       from lm, but when prelinking is involved and the prelink base
       address changes, we may need a different offset, we want to
       warn about the difference and compute it only once.  */
    CORE_ADDR l_addr;

    /* The target location of lm.  */
    CORE_ADDR lm_addr;
  };


static CORE_ADDR
lm_addr (struct so_list *so)
{
  if (so->lm_info->l_addr == (CORE_ADDR)-1)
    {
      struct link_map_offsets *lmo = nto_fetch_link_map_offsets ();
      struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;

      so->lm_info->l_addr =
	extract_typed_address (so->lm_info->lm + lmo->l_addr_offset, ptr_type);
    }
  return so->lm_info->l_addr;
}

static CORE_ADDR
nto_truncate_ptr (CORE_ADDR addr)
{
  if (gdbarch_ptr_bit (target_gdbarch ()) == sizeof (CORE_ADDR) * 8)
    /* We don't need to truncate anything, and the bit twiddling below
       will fail due to overflow problems.  */
    return addr;
  else
    return addr & (((CORE_ADDR) 1 << gdbarch_ptr_bit (target_gdbarch ())) - 1);
}

static Elf_Internal_Phdr *
find_load_phdr (bfd *abfd)
{
  Elf_Internal_Phdr *phdr;
  unsigned int i;

  if (!elf_tdata (abfd))
    return NULL;

  phdr = elf_tdata (abfd)->phdr;
  for (i = 0; i < elf_elfheader (abfd)->e_phnum; i++, phdr++)
    {
      if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_X))
	return phdr;
    }
  return NULL;
}

void
nto_relocate_section_addresses (struct so_list *so, struct target_section *sec)
{
  /* Neutrino treats the l_addr base address field in link.h as different than
     the base address in the System V ABI and so the offset needs to be
     calculated and applied to relocations.  */
  Elf_Internal_Phdr *phdr = find_load_phdr (sec->the_bfd_section->owner);
  unsigned vaddr = phdr ? phdr->p_vaddr : 0;

  sec->addr = nto_truncate_ptr (sec->addr + lm_addr (so) - vaddr);
  sec->endaddr = nto_truncate_ptr (sec->endaddr + lm_addr (so) - vaddr);
}

/* This is cheating a bit because our linker code is in libc.so.  If we
   ever implement lazy linking, this may need to be re-examined.  */
int
nto_in_dynsym_resolve_code (CORE_ADDR pc)
{
  if (in_plt_section (pc))
    return 1;
  return 0;
}

void
nto_dummy_supply_regset (struct regcache *regcache, char *regs)
{
  /* Do nothing.  */
}

enum gdb_osabi
nto_elf_osabi_sniffer (bfd *abfd)
{
  if (nto_is_nto_target)
    return nto_is_nto_target (abfd);
  return GDB_OSABI_UNKNOWN;
}

static const char *nto_thread_state_str[] =
{
  "DEAD",		/* 0  0x00 */
  "RUNNING",	/* 1  0x01 */
  "READY",	/* 2  0x02 */
  "STOPPED",	/* 3  0x03 */
  "SEND",		/* 4  0x04 */
  "RECEIVE",	/* 5  0x05 */
  "REPLY",	/* 6  0x06 */
  "STACK",	/* 7  0x07 */
  "WAITTHREAD",	/* 8  0x08 */
  "WAITPAGE",	/* 9  0x09 */
  "SIGSUSPEND",	/* 10 0x0a */
  "SIGWAITINFO",	/* 11 0x0b */
  "NANOSLEEP",	/* 12 0x0c */
  "MUTEX",	/* 13 0x0d */
  "CONDVAR",	/* 14 0x0e */
  "JOIN",		/* 15 0x0f */
  "INTR",		/* 16 0x10 */
  "SEM",		/* 17 0x11 */
  "WAITCTX",	/* 18 0x12 */
  "NET_SEND",	/* 19 0x13 */
  "NET_REPLY"	/* 20 0x14 */
};

char *
nto_extra_thread_info (struct thread_info *ti)
{
  if (ti && ti->private
      && ti->private->state < ARRAY_SIZE (nto_thread_state_str))
    return (char *)nto_thread_state_str [ti->private->state];
  return "";
}

void
nto_initialize_signals (void)
{
  /* We use SIG45 for pulses, or something, so nostop, noprint
     and pass them.  */
  signal_stop_update (gdb_signal_from_name ("SIG45"), 0);
  signal_print_update (gdb_signal_from_name ("SIG45"), 0);
  signal_pass_update (gdb_signal_from_name ("SIG45"), 1);

  /* By default we don't want to stop on these two, but we do want to pass.  */
#if defined(SIGSELECT)
  signal_stop_update (SIGSELECT, 0);
  signal_print_update (SIGSELECT, 0);
  signal_pass_update (SIGSELECT, 1);
#endif

#if defined(SIGPHOTON)
  signal_stop_update (SIGPHOTON, 0);
  signal_print_update (SIGPHOTON, 0);
  signal_pass_update (SIGPHOTON, 1);
#endif
}
