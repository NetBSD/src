/* Main simulator entry points specific to the CRIS.
   Copyright (C) 2004-2015 Free Software Foundation, Inc.
   Contributed by Axis Communications.

This file is part of the GNU simulators.

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

/* Based on the fr30 file, mixing in bits from the i960 and pruning of
   dead code.  */

#include "config.h"
#include "libiberty.h"
#include "bfd.h"
#include "elf-bfd.h"

#include "sim-main.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>
#include "sim-options.h"
#include "dis-asm.h"

/* Apparently the autoconf bits are missing (though HAVE_ENVIRON is used
   in other dirs; also lacking there).  Patch around it for major systems.  */
#if defined (HAVE_ENVIRON) || defined (__GLIBC__)
extern char **environ;
#define GET_ENVIRON() environ
#else
char *missing_environ[] = { "SHELL=/bin/sh", "PATH=/bin:/usr/bin", NULL };
#define GET_ENVIRON() missing_environ
#endif

/* Used with get_progbounds to find out how much memory is needed for the
   program.  We don't want to allocate more, since that could mask
   invalid memory accesses program bugs.  */
struct progbounds {
  USI startmem;
  USI endmem;
  USI end_loadmem;
  USI start_nonloadmem;
};

static void free_state (SIM_DESC);
static void get_progbounds_iterator (bfd *, asection *, void *);
static SIM_RC cris_option_handler (SIM_DESC, sim_cpu *, int, char *, int);

/* Since we don't build the cgen-opcode table, we use the old
   disassembler.  */
static CGEN_DISASSEMBLER cris_disassemble_insn;

/* By default, we set up stack and environment variables like the Linux
   kernel.  */
static char cris_bare_iron = 0;

/* Whether 0x9000000xx have simulator-specific meanings.  */
char cris_have_900000xxif = 0;

/* Used to optionally override the default start address of the
   simulation.  */
static USI cris_start_address = 0xffffffffu;

/* Used to optionally add offsets to the loaded image and its start
   address.  (Not used for the interpreter of dynamically loaded
   programs or the DSO:s.)  */
static int cris_program_offset = 0;

/* What to do when we face a more or less unknown syscall.  */
enum cris_unknown_syscall_action_type cris_unknown_syscall_action
  = CRIS_USYSC_MSG_STOP;

/* Records simulator descriptor so utilities like cris_dump_regs can be
   called from gdb.  */
SIM_DESC current_state;

/* CRIS-specific options.  */
typedef enum {
  OPTION_CRIS_STATS = OPTION_START,
  OPTION_CRIS_TRACE,
  OPTION_CRIS_NAKED,
  OPTION_CRIS_PROGRAM_OFFSET,
  OPTION_CRIS_STARTADDR,
  OPTION_CRIS_900000XXIF,
  OPTION_CRIS_UNKNOWN_SYSCALL
} CRIS_OPTIONS;

static const OPTION cris_options[] =
{
  { {"cris-cycles", required_argument, NULL, OPTION_CRIS_STATS},
      '\0', "basic|unaligned|schedulable|all",
    "Dump execution statistics",
      cris_option_handler, NULL },
  { {"cris-trace", required_argument, NULL, OPTION_CRIS_TRACE},
      '\0', "basic",
    "Emit trace information while running",
      cris_option_handler, NULL },
  { {"cris-naked", no_argument, NULL, OPTION_CRIS_NAKED},
     '\0', NULL, "Don't set up stack and environment",
     cris_option_handler, NULL },
  { {"cris-900000xx", no_argument, NULL, OPTION_CRIS_900000XXIF},
     '\0', NULL, "Define addresses at 0x900000xx with simulator semantics",
     cris_option_handler, NULL },
  { {"cris-unknown-syscall", required_argument, NULL,
     OPTION_CRIS_UNKNOWN_SYSCALL},
     '\0', "stop|enosys|enosys-quiet", "Action at an unknown system call",
     cris_option_handler, NULL },
  { {"cris-program-offset", required_argument, NULL,
     OPTION_CRIS_PROGRAM_OFFSET},
      '\0', "OFFSET",
    "Offset image addresses and default start address of a program",
      cris_option_handler },
  { {"cris-start-address", required_argument, NULL, OPTION_CRIS_STARTADDR},
      '\0', "ADDRESS", "Set start address",
      cris_option_handler },
  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

/* Add the CRIS-specific option list to the simulator.  */

SIM_RC
cris_option_install (SIM_DESC sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  if (sim_add_option_table (sd, NULL, cris_options) != SIM_RC_OK)
    return SIM_RC_FAIL;
  return SIM_RC_OK;
}

/* Handle CRIS-specific options.  */

static SIM_RC
cris_option_handler (SIM_DESC sd, sim_cpu *cpu ATTRIBUTE_UNUSED, int opt,
		     char *arg, int is_command ATTRIBUTE_UNUSED)
{
  /* The options are CRIS-specific, but cpu-specific option-handling is
     broken; required to being with "--cpu0-".  We store the flags in an
     unused field in the global state structure and move the flags over
     to the module-specific CPU data when we store things in the
     cpu-specific structure.  */
  char *tracefp = STATE_TRACE_FLAGS (sd);
  char *chp = arg;

  switch ((CRIS_OPTIONS) opt)
    {
      case OPTION_CRIS_STATS:
	if (strcmp (arg, "basic") == 0)
	  *tracefp = FLAG_CRIS_MISC_PROFILE_SIMPLE;
	else if (strcmp (arg, "unaligned") == 0)
	  *tracefp
	    = (FLAG_CRIS_MISC_PROFILE_UNALIGNED
	       | FLAG_CRIS_MISC_PROFILE_SIMPLE);
	else if (strcmp (arg, "schedulable") == 0)
	  *tracefp
	    = (FLAG_CRIS_MISC_PROFILE_SCHEDULABLE
	       | FLAG_CRIS_MISC_PROFILE_SIMPLE);
	else if (strcmp (arg, "all") == 0)
	  *tracefp = FLAG_CRIS_MISC_PROFILE_ALL;
	else
	  {
	    /* Beware; the framework does not handle the error case;
	       we have to do it ourselves.  */
	    sim_io_eprintf (sd, "Unknown option `--cris-cycles=%s'\n", arg);
	    return SIM_RC_FAIL;
	  }
	break;

      case OPTION_CRIS_TRACE:
	if (strcmp (arg, "basic") == 0)
	  *tracefp |= FLAG_CRIS_MISC_PROFILE_XSIM_TRACE;
	else
	  {
	    sim_io_eprintf (sd, "Unknown option `--cris-trace=%s'\n", arg);
	    return SIM_RC_FAIL;
	  }
	break;

      case OPTION_CRIS_NAKED:
	cris_bare_iron = 1;
	break;

      case OPTION_CRIS_900000XXIF:
	cris_have_900000xxif = 1;
	break;

      case OPTION_CRIS_STARTADDR:
	errno = 0;
	cris_start_address = (USI) strtoul (chp, &chp, 0);

	if (errno != 0 || *chp != 0)
	  {
	    sim_io_eprintf (sd, "Invalid option `--cris-start-address=%s'\n",
			    arg);
	    return SIM_RC_FAIL;
	  }
	break;

      case OPTION_CRIS_PROGRAM_OFFSET:
	errno = 0;
	cris_program_offset = (int) strtol (chp, &chp, 0);

	if (errno != 0 || *chp != 0)
	  {
	    sim_io_eprintf (sd, "Invalid option `--cris-program-offset=%s'\n",
			    arg);
	    return SIM_RC_FAIL;
	  }
	break;

      case OPTION_CRIS_UNKNOWN_SYSCALL:
	if (strcmp (arg, "enosys") == 0)
	  cris_unknown_syscall_action = CRIS_USYSC_MSG_ENOSYS;
	else if (strcmp (arg, "enosys-quiet") == 0)
	  cris_unknown_syscall_action = CRIS_USYSC_QUIET_ENOSYS;
	else if (strcmp (arg, "stop") == 0)
	  cris_unknown_syscall_action = CRIS_USYSC_MSG_STOP;
	else
	  {
	    sim_io_eprintf (sd, "Unknown option `--cris-unknown-syscall=%s'\n",
			    arg);
	    return SIM_RC_FAIL;
	  }
	break;

      default:
	/* We'll actually never get here; the caller handles the error
	   case.  */
	sim_io_eprintf (sd, "Unknown option `%s'\n", arg);
	return SIM_RC_FAIL;
    }

  /* Imply --profile-model=on.  */
  return sim_profile_set_option (sd, "-model", PROFILE_MODEL_IDX, "on");
}

/* FIXME: Remove these, globalize those in sim-load.c, move elsewhere.  */

static void
xprintf  (host_callback *callback, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);

  (*callback->vprintf_filtered) (callback, fmt, ap);

  va_end (ap);
}

static void
eprintf (host_callback *callback, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);

  (*callback->evprintf_filtered) (callback, fmt, ap);

  va_end (ap);
}

/* An ELF-specific simplified ../common/sim-load.c:sim_load_file,
   using the program headers, not sections, in order to make sure that
   the program headers themeselves are also loaded.  The caller is
   responsible for asserting that ABFD is an ELF file.  */

static bfd_boolean
cris_load_elf_file (SIM_DESC sd, struct bfd *abfd, sim_write_fn do_write)
{
  Elf_Internal_Phdr *phdr;
  int n_hdrs;
  int i;
  bfd_boolean verbose = STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG;
  host_callback *callback = STATE_CALLBACK (sd);

  phdr = elf_tdata (abfd)->phdr;
  n_hdrs = elf_elfheader (abfd)->e_phnum;

  /* We're only interested in PT_LOAD; all necessary information
     should be covered by that.  */
  for (i = 0; i < n_hdrs; i++)
    {
      bfd_byte *buf;
      bfd_vma lma = STATE_LOAD_AT_LMA_P (sd)
	? phdr[i].p_paddr : phdr[i].p_vaddr;

      if (phdr[i].p_type != PT_LOAD)
	continue;

      buf = xmalloc (phdr[i].p_filesz);

      if (verbose)
	xprintf (callback, "Loading segment at 0x%lx, size 0x%lx\n",
		 lma, phdr[i].p_filesz);

      if (bfd_seek (abfd, phdr[i].p_offset, SEEK_SET) != 0
	  || (bfd_bread (buf, phdr[i].p_filesz, abfd) != phdr[i].p_filesz))
	{
	  eprintf (callback,
		   "%s: could not read segment at 0x%lx, size 0x%lx\n",
		   STATE_MY_NAME (sd), lma, phdr[i].p_filesz);
	  free (buf);
	  return FALSE;
	}

      if (do_write (sd, lma, buf, phdr[i].p_filesz) != phdr[i].p_filesz)
	{
	  eprintf (callback,
		   "%s: could not load segment at 0x%lx, size 0x%lx\n",
		   STATE_MY_NAME (sd), lma, phdr[i].p_filesz);
	  free (buf);
	  return FALSE;
	}

      free (buf);
    }

  return TRUE;
}

/* Helper for sim_load (needed just for ELF files): like sim_write,
   but offset load at cris_program_offset offset.  */

static int
cris_program_offset_write (SIM_DESC sd, SIM_ADDR mem, unsigned char *buf,
			   int length)
{
  return sim_write (sd, mem + cris_program_offset, buf, length);
}

/* Replacement for ../common/sim-hload.c:sim_load, so we can treat ELF
   files differently.  */

SIM_RC
sim_load (SIM_DESC sd, const char *prog_name, struct bfd *prog_bfd,
	  int from_tty ATTRIBUTE_UNUSED)
{
  bfd *result_bfd;

  if (bfd_get_flavour (prog_bfd) != bfd_target_elf_flavour)
    {
      SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
      if (sim_analyze_program (sd, prog_name, prog_bfd) != SIM_RC_OK)
	return SIM_RC_FAIL;
      SIM_ASSERT (STATE_PROG_BFD (sd) != NULL);

      result_bfd = sim_load_file (sd, STATE_MY_NAME (sd),
				  STATE_CALLBACK (sd),
				  prog_name,
				  STATE_PROG_BFD (sd),
				  STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG,
				  STATE_LOAD_AT_LMA_P (sd),
				  sim_write);
      if (result_bfd == NULL)
	{
	  bfd_close (STATE_PROG_BFD (sd));
	  STATE_PROG_BFD (sd) = NULL;
	  return SIM_RC_FAIL;
	}
      return SIM_RC_OK;
    }

  return cris_load_elf_file (sd, prog_bfd, cris_program_offset_write)
    ? SIM_RC_OK : SIM_RC_FAIL;
}

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Helper struct for cris_set_section_offset_iterator.  */

struct offsetinfo
{
  SIM_DESC sd;
  int offset;
};

/* BFD section iterator to offset the LMA and VMA.  */

static void
cris_set_section_offset_iterator (bfd *abfd, asection *s, void *vp)
{
  struct offsetinfo *p = (struct offsetinfo *) vp;
  SIM_DESC sd = p->sd;
  int offset = p->offset;

  if ((bfd_get_section_flags (abfd, s) & SEC_ALLOC))
    {
      bfd_vma vma = bfd_get_section_vma (abfd, s);
      
      bfd_set_section_vma (abfd, s, vma + offset);
    }

  /* This seems clumsy and inaccurate, but let's stick to doing it the
     same way as sim_analyze_program for consistency.  */
  if (strcmp (bfd_get_section_name (abfd, s), ".text") == 0)
    STATE_TEXT_START (sd) = bfd_get_section_vma (abfd, s);
}

/* Adjust the start-address, LMA and VMA of a SD.  Must be called
   after sim_analyze_program.  */

static void
cris_offset_sections (SIM_DESC sd, int offset)
{
  bfd_boolean ret;
  struct bfd *abfd = STATE_PROG_BFD (sd);
  asection *text;
  struct offsetinfo oi;

  /* Only happens for usage error.  */
  if (abfd == NULL)
    return;

  oi.sd = sd;
  oi.offset = offset;

  bfd_map_over_sections (abfd, cris_set_section_offset_iterator, &oi);
  ret = bfd_set_start_address (abfd, bfd_get_start_address (abfd) + offset);

  STATE_START_ADDR (sd) = bfd_get_start_address (abfd);
}

/* BFD section iterator to find the highest and lowest allocated and
   non-allocated section addresses (plus one).  */

static void
get_progbounds_iterator (bfd *abfd ATTRIBUTE_UNUSED, asection *s, void *vp)
{
  struct progbounds *pbp = (struct progbounds *) vp;

  if ((bfd_get_section_flags (abfd, s) & SEC_ALLOC))
    {
      bfd_size_type sec_size = bfd_get_section_size (s);
      bfd_size_type sec_start = bfd_get_section_vma (abfd, s);
      bfd_size_type sec_end = sec_start + sec_size;

      if (sec_end > pbp->endmem)
	pbp->endmem = sec_end;

      if (sec_start < pbp->startmem)
	pbp->startmem = sec_start;

      if ((bfd_get_section_flags (abfd, s) & SEC_LOAD))
	{
	  if (sec_end > pbp->end_loadmem)
	    pbp->end_loadmem = sec_end;
	}
      else if (sec_start < pbp->start_nonloadmem)
	pbp->start_nonloadmem = sec_start;
    }
}

/* Get the program boundaries.  Because not everything is covered by
   sections in ELF, notably the program headers, we use the program
   headers instead.  */

static void
cris_get_progbounds (struct bfd *abfd, struct progbounds *pbp)
{
  Elf_Internal_Phdr *phdr;
  int n_hdrs;
  int i;

  pbp->startmem = 0xffffffff;
  pbp->endmem = 0;
  pbp->end_loadmem = 0;
  pbp->start_nonloadmem = 0xffffffff;

  /* In case we're ever used for something other than ELF, use the
     generic method.  */
  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    {
      bfd_map_over_sections (abfd, get_progbounds_iterator, pbp);
      return;
    }

  phdr = elf_tdata (abfd)->phdr;
  n_hdrs = elf_elfheader (abfd)->e_phnum;

  /* We're only interested in PT_LOAD; all necessary information
     should be covered by that.  */
  for (i = 0; i < n_hdrs; i++)
    {
      if (phdr[i].p_type != PT_LOAD)
	continue;

      if (phdr[i].p_paddr < pbp->startmem)
	pbp->startmem = phdr[i].p_paddr;

      if (phdr[i].p_paddr + phdr[i].p_memsz > pbp->endmem)
	pbp->endmem = phdr[i].p_paddr + phdr[i].p_memsz;

      if (phdr[i].p_paddr + phdr[i].p_filesz > pbp->end_loadmem)
	pbp->end_loadmem = phdr[i].p_paddr + phdr[i].p_filesz;

      if (phdr[i].p_memsz > phdr[i].p_filesz
	  && phdr[i].p_paddr + phdr[i].p_filesz < pbp->start_nonloadmem)
	pbp->start_nonloadmem = phdr[i].p_paddr + phdr[i].p_filesz;
    }  
}

/* Parameter communication by static variables, hmm...  Oh well, for
   simplicity.  */
static bfd_vma exec_load_addr;
static bfd_vma interp_load_addr;
static bfd_vma interp_start_addr;

/* Supposed to mimic Linux' "NEW_AUX_ENT (AT_PHDR, load_addr + exec->e_phoff)".  */

static USI
aux_ent_phdr (struct bfd *ebfd)
{
  return elf_elfheader (ebfd)->e_phoff + exec_load_addr;
}

/* We just pass on the header info; we don't have our own idea of the
   program header entry size.  */

static USI
aux_ent_phent (struct bfd *ebfd)
{
  return elf_elfheader (ebfd)->e_phentsize;
}

/* Like "NEW_AUX_ENT(AT_PHNUM, exec->e_phnum)".  */

static USI
aux_ent_phnum (struct bfd *ebfd)
{
  return elf_elfheader (ebfd)->e_phnum;
}

/* Like "NEW_AUX_ENT(AT_BASE, interp_load_addr)".  */

static USI
aux_ent_base (struct bfd *ebfd)
{
  return interp_load_addr;
}

/* Like "NEW_AUX_ENT(AT_ENTRY, exec->e_entry)".  */

static USI
aux_ent_entry (struct bfd *ebfd)
{
  ASSERT (elf_elfheader (ebfd)->e_entry == bfd_get_start_address (ebfd));
  return elf_elfheader (ebfd)->e_entry;
}

/* Helper for cris_handle_interpreter: like sim_write, but load at
   interp_load_addr offset.  */

static int
cris_write_interp (SIM_DESC sd, SIM_ADDR mem, unsigned char *buf, int length)
{
  return sim_write (sd, mem + interp_load_addr, buf, length);
}

/* Cater to the presence of an interpreter: load it and set
   interp_start_addr.  Return FALSE if there was an error, TRUE if
   everything went fine, including an interpreter being absent and
   the program being in a non-ELF format.  */

static bfd_boolean
cris_handle_interpreter (SIM_DESC sd, struct bfd *abfd)
{
  int i, n_hdrs;
  bfd_vma phaddr;
  bfd_byte buf[4];
  char *interp = NULL;
  struct bfd *ibfd;
  bfd_boolean ok = FALSE;
  Elf_Internal_Phdr *phdr;

  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    return TRUE;

  phdr = elf_tdata (abfd)->phdr;
  n_hdrs = aux_ent_phnum (abfd);

  /* Check the program headers for presence of an interpreter.  */
  for (i = 0; i < n_hdrs; i++)
    {
      int interplen;
      bfd_size_type interpsiz, interp_filesiz;
      struct progbounds interp_bounds;

      if (phdr[i].p_type != PT_INTERP)
	continue;

      /* Get the name of the interpreter, prepended with the sysroot
	 (empty if absent).  */
      interplen = phdr[i].p_filesz;
      interp = xmalloc (interplen + strlen (simulator_sysroot));
      strcpy (interp, simulator_sysroot);

      /* Read in the name.  */
      if (bfd_seek (abfd, phdr[i].p_offset, SEEK_SET) != 0
	  || (bfd_bread (interp + strlen (simulator_sysroot), interplen, abfd)
	      != interplen))
	goto interpname_failed;

      /* Like Linux, require the string to be 0-terminated.  */
      if (interp[interplen + strlen (simulator_sysroot) - 1] != 0)
	goto interpname_failed;

      /* Inspect the interpreter.  */
      ibfd = bfd_openr (interp, STATE_TARGET (sd));
      if (ibfd == NULL)
	goto interpname_failed;

      /* The interpreter is at least something readable to BFD; make
	 sure it's an ELF non-archive file.  */
      if (!bfd_check_format (ibfd, bfd_object)
	  || bfd_get_flavour (ibfd) != bfd_target_elf_flavour)
	goto interp_failed;

      /* Check the layout of the interpreter.  */
      cris_get_progbounds (ibfd, &interp_bounds);

      /* Round down to pagesize the start page and up the endpage.
	 Don't round the *load and *nonload members.  */
      interp_bounds.startmem &= ~8191;
      interp_bounds.endmem = (interp_bounds.endmem + 8191) & ~8191;

      /* Until we need a more dynamic solution, assume we can put the
	 interpreter at this fixed location.  NB: this is not what
	 happens for Linux 2008-12-28, but it could and might and
	 perhaps should.  */
      interp_load_addr = 0x40000;
      interpsiz = interp_bounds.endmem - interp_bounds.startmem;
      interp_filesiz = interp_bounds.end_loadmem - interp_bounds.startmem;

      /* If we have a non-DSO or interpreter starting at the wrong
	 address, bail.  */
      if (interp_bounds.startmem != 0
	  || interpsiz + interp_load_addr >= exec_load_addr)
	goto interp_failed;

      /* We don't have the API to get the address of a simulator
	 memory area, so we go via a temporary area.  Luckily, the
	 interpreter is supposed to be small, less than 0x40000
	 bytes.  */
      sim_do_commandf (sd, "memory region 0x%lx,0x%lx",
		       interp_load_addr, interpsiz);

      /* Now that memory for the interpreter is defined, load it.  */
      if (!cris_load_elf_file (sd, ibfd, cris_write_interp))
	goto interp_failed;

      /* It's no use setting STATE_START_ADDR, because it gets
	 overwritten by a sim_analyze_program call in sim_load.  Let's
	 just store it locally.  */
      interp_start_addr
	= (bfd_get_start_address (ibfd)
	   - interp_bounds.startmem + interp_load_addr);

      /* Linux cares only about the first PT_INTERP, so let's ignore
	 the rest.  */
      goto all_done;
    }

  /* Register R10 should hold 0 at static start (no finifunc), but
     that's the default, so don't bother.  */
  return TRUE;

 all_done:
  ok = TRUE;

 interp_failed:
  bfd_close (ibfd);

 interpname_failed:
  if (!ok)
    sim_io_eprintf (sd,
		    "%s: could not load ELF interpreter `%s' for program `%s'\n",
		    STATE_MY_NAME (sd),
		    interp == NULL ? "(what's-its-name)" : interp,
		    bfd_get_filename (abfd));
  free (interp);
  return ok;
}

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback, struct bfd *abfd,
	  char **argv)
{
  char c;
  int i;
  USI startmem = 0;
  USI endmem = CRIS_DEFAULT_MEM_SIZE;
  USI endbrk = endmem;
  USI stack_low = 0;
  SIM_DESC sd = sim_state_alloc (kind, callback);

  static const struct auxv_entries_s
  {
    bfd_byte id;
    USI (*efn) (struct bfd *ebfd);
    USI val;
  } auxv_entries[] =
    {
#define AUX_ENT(a, b) {a, NULL, b}
#define AUX_ENTF(a, f) {a, f, 0}
      AUX_ENT (AT_HWCAP, 0),
      AUX_ENT (AT_PAGESZ, 8192),
      AUX_ENT (AT_CLKTCK, 100),
      AUX_ENTF (AT_PHDR, aux_ent_phdr),
      AUX_ENTF (AT_PHENT, aux_ent_phent),
      AUX_ENTF (AT_PHNUM, aux_ent_phnum),
      AUX_ENTF (AT_BASE, aux_ent_base),
      AUX_ENT (AT_FLAGS, 0),
      AUX_ENTF (AT_ENTRY, aux_ent_entry),

      /* Or is root better?  Maybe have it settable?  */
      AUX_ENT (AT_UID, 500),
      AUX_ENT (AT_EUID, 500),
      AUX_ENT (AT_GID, 500),
      AUX_ENT (AT_EGID, 500),
      AUX_ENT (AT_SECURE, 0),
      AUX_ENT (AT_NULL, 0)
    };

  /* Can't initialize to "" below.  It's either a GCC bug in old
     releases (up to and including 2.95.3 (.4 in debian) or a bug in the
     standard ;-) that the rest of the elements won't be initialized.  */
  bfd_byte sp_init[4] = {0, 0, 0, 0};

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 1, cgen_cpu_max_extra_bytes ()) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* getopt will print the error message so we just have to exit if this fails.
     FIXME: Hmmm...  in the case of gdb we need getopt to call
     print_filtered.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* If we have a binary program, endianness-setting would not be taken
     from elsewhere unfortunately, so set it here.  At the time of this
     writing, it isn't used until sim_config, but that might change so
     set it here before memory is defined or touched.  */
  current_target_byte_order = LITTLE_ENDIAN;

  /* check for/establish the reference program image */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL),
			   abfd) != SIM_RC_OK)
    {
      /* When there's an error, sim_analyze_program has already output
	 a message.  Let's just clarify it, as "not an object file"
	 perhaps doesn't ring a bell.  */
      sim_io_eprintf (sd, "(not a CRIS program)\n");
      free_state (sd);
      return 0;
    }

  /* We might get called with the caller expecting us to get hold of
     the bfd for ourselves, which would happen at the
     sim_analyze_program call above.  */
  if (abfd == NULL)
    abfd = STATE_PROG_BFD (sd);

  /* Adjust the addresses of the program at this point.  Unfortunately
     this does not affect ELF program headers, so we have to handle
     that separately.  */
  cris_offset_sections (sd, cris_program_offset);

  if (abfd != NULL && bfd_get_arch (abfd) == bfd_arch_unknown)
    {
      if (STATE_PROG_ARGV (sd) != NULL)
	sim_io_eprintf (sd, "%s: `%s' is not a CRIS program\n",
			STATE_MY_NAME (sd), *STATE_PROG_ARGV (sd));
      else
	sim_io_eprintf (sd, "%s: program to be run is not a CRIS program\n",
			STATE_MY_NAME (sd));
      free_state (sd);
      return 0;
    }

  /* For CRIS simulator-specific use, we need to find out the bounds of
     the program as well, which is not done by sim_analyze_program
     above.  */
  if (abfd != NULL)
    {
      struct progbounds pb;

      /* The sections should now be accessible using bfd functions.  */
      cris_get_progbounds (abfd, &pb);

      /* We align the area that the program uses to page boundaries.  */
      startmem = pb.startmem & ~8191;
      endbrk = pb.endmem;
      endmem = (endbrk + 8191) & ~8191;
    }

  /* Find out how much room is needed for the environment and argv, create
     that memory and fill it.  Only do this when there's a program
     specified.  */
  if (abfd != NULL && !cris_bare_iron)
    {
      char *name = bfd_get_filename (abfd);
      char **my_environ = GET_ENVIRON ();
      /* We use these maps to give the same behavior as the old xsim
	 simulator.  */
      USI envtop = 0x40000000;
      USI stacktop = 0x3e000000;
      USI envstart;
      int envc;
      int len = strlen (name) + 1;
      USI epp, epp0;
      USI stacklen;
      int i;
      char **prog_argv = STATE_PROG_ARGV (sd);
      int my_argc = 0;
      /* All CPU:s have the same memory map, apparently.  */
      SIM_CPU *cpu = STATE_CPU (sd, 0);
      USI csp;
      bfd_byte buf[4];

      /* Count in the environment as well. */
      for (envc = 0; my_environ[envc] != NULL; envc++)
	len += strlen (my_environ[envc]) + 1;

      for (i = 0; prog_argv[i] != NULL; my_argc++, i++)
	len += strlen (prog_argv[i]) + 1;

      envstart = (envtop - len) & ~8191;

      /* Create read-only block for the environment strings.  */
      sim_core_attach (sd, NULL, 0, access_read, 0,
		       envstart, (len + 8191) & ~8191,
		       0, NULL, NULL);

      /* This shouldn't happen.  */
      if (envstart < stacktop)
	stacktop = envstart - 64 * 8192;

      csp = stacktop;

      /* Note that the linux kernel does not correctly compute the storage
	 needs for the static-exe AUX vector.  */

      csp -= sizeof (auxv_entries) / sizeof (auxv_entries[0]) * 4 * 2;

      csp -= (envc + 1) * 4;
      csp -= (my_argc + 1) * 4;
      csp -= 4;

      /* Write the target representation of the start-up-value for the
	 stack-pointer suitable for register initialization below.  */
      bfd_putl32 (csp, sp_init);

      /* If we make this 1M higher; say 8192*1024, we have to take
	 special precautions for pthreads, because pthreads assumes that
	 the memory that low isn't mmapped, and that it can mmap it
	 without fallback in case of failure (and we fail ungracefully
	 long before *that*: the memory isn't accounted for in our mmap
	 list).  */
      stack_low = (csp - (7168*1024)) & ~8191;

      stacklen = stacktop - stack_low;

      /* Tee hee, we have an executable stack.  Well, it's necessary to
	 test GCC trampolines...  */
      sim_core_attach (sd, NULL, 0, access_read_write_exec, 0,
		       stack_low, stacklen,
		       0, NULL, NULL);

      epp = epp0 = envstart;

      /* Can't use sim_core_write_unaligned_4 without everything
	 initialized when tracing, and then these writes would get into
	 the trace.  */
#define write_dword(addr, data)						\
 do									\
   {									\
     USI data_ = data;							\
     USI addr_ = addr;							\
     bfd_putl32 (data_, buf);						\
     if (sim_core_write_buffer (sd, cpu, 0, buf, addr_, 4) != 4)	\
	goto abandon_chip;						\
   }									\
 while (0)

      write_dword (csp, my_argc);
      csp += 4;

      for (i = 0; i < my_argc; i++, csp += 4)
	{
	  size_t strln = strlen (prog_argv[i]) + 1;

	  if (sim_core_write_buffer (sd, cpu, 0, prog_argv[i], epp, strln)
	      != strln)
	  goto abandon_chip;

	  write_dword (csp, envstart + epp - epp0);
	  epp += strln;
	}

      write_dword (csp, 0);
      csp += 4;

      for (i = 0; i < envc; i++, csp += 4)
	{
	  unsigned int strln = strlen (my_environ[i]) + 1;

	  if (sim_core_write_buffer (sd, cpu, 0, my_environ[i], epp, strln)
	      != strln)
	    goto abandon_chip;

	  write_dword (csp, envstart + epp - epp0);
	  epp += strln;
	}

      write_dword (csp, 0);
      csp += 4;

      /* The load address of the executable could presumably be
	 different than the lowest used memory address, but let's
	 stick to simplicity until needed.  And
	 cris_handle_interpreter might change startmem and endmem, so
	 let's set it now.  */
      exec_load_addr = startmem;

      if (!cris_handle_interpreter (sd, abfd))
	goto abandon_chip;

      if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
	for (i = 0; i < sizeof (auxv_entries) / sizeof (auxv_entries[0]); i++)
	  {
	    write_dword (csp, auxv_entries[i].id);
	    write_dword (csp + 4,
			 auxv_entries[i].efn != NULL
			 ? (*auxv_entries[i].efn) (abfd)
			 : auxv_entries[i].val);
	    csp += 4 + 4;
	  }
    }

  /* Allocate core managed memory if none specified by user.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, startmem, 1) == 0)
    sim_do_commandf (sd, "memory region 0x%lx,0x%lx", startmem,
		     endmem - startmem);

  /* Allocate simulator I/O managed memory if none specified by user.  */
  if (cris_have_900000xxif)
    {
      if (sim_core_read_buffer (sd, NULL, read_map, &c, 0x90000000, 1) == 0)
	sim_core_attach (sd, NULL, 0, access_write, 0, 0x90000000, 0x100,
			 0, &cris_devices, NULL);
      else
	{
	  (*callback->
	   printf_filtered) (callback,
			     "Seeing --cris-900000xx with memory defined there\n");
	  goto abandon_chip;
	}
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
    abandon_chip:
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Open a copy of the cpu descriptor table.  */
  {
    CGEN_CPU_DESC cd = cris_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
					     CGEN_ENDIAN_LITTLE);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = cris_disassemble_insn;

	/* See cris_option_handler for the reason why this is needed.  */
	CPU_CRIS_MISC_PROFILE (cpu)->flags = STATE_TRACE_FLAGS (sd)[0];

	/* Set SP to the stack we allocated above.  */
	(* CPU_REG_STORE (cpu)) (cpu, H_GR_SP, (char *) sp_init, 4);

	/* Set the simulator environment data.  */
	cpu->highest_mmapped_page = NULL;
	cpu->endmem = endmem;
	cpu->endbrk = endbrk;
	cpu->stack_low = stack_low;
	cpu->syscalls = 0;
	cpu->m1threads = 0;
	cpu->threadno = 0;
	cpu->max_threadid = 0;
	cpu->thread_data = NULL;
	memset (cpu->sighandler, 0, sizeof (cpu->sighandler));
	cpu->make_thread_cpu_data = NULL;
	cpu->thread_cpu_data_size = 0;
#if WITH_HW
	cpu->deliver_interrupt = NULL;
#endif
      }
#if WITH_HW
    /* Always be cycle-accurate and call before/after functions if
       with-hardware.  */
    sim_profile_set_option (sd, "-model", PROFILE_MODEL_IDX, "on");
#endif
  }

  /* Initialize various cgen things not done by common framework.
     Must be done after cris_cgen_cpu_open.  */
  cgen_init (sd);

  /* Store in a global so things like cris_dump_regs can be invoked
     from the gdb command line.  */
  current_state = sd;

  cris_set_callbacks (callback);

  return sd;
}

void
sim_close (SIM_DESC sd, int quitting ATTRIBUTE_UNUSED)
{
  cris_cgen_cpu_close (CPU_CPU_DESC (STATE_CPU (sd, 0)));
  sim_module_uninstall (sd);
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char **argv ATTRIBUTE_UNUSED,
		     char **envp ATTRIBUTE_UNUSED)
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  SIM_ADDR addr;

  if (sd != NULL)
    addr = cris_start_address != (SIM_ADDR) -1
      ? cris_start_address
      : (interp_start_addr != 0
	 ? interp_start_addr
	 : bfd_get_start_address (abfd));
  else
    addr = 0;
  sim_pc_set (current_cpu, addr);

  /* Other simulators have #if 0:d code that says
      STATE_ARGV (sd) = sim_copy_argv (argv);
      STATE_ENVP (sd) = sim_copy_argv (envp);
     Enabling that gives you not-found link-errors for sim_copy_argv.
     FIXME: Do archaeology to find out more.  */

  return SIM_RC_OK;
}

/* Disassemble an instruction.  */

static void
cris_disassemble_insn (SIM_CPU *cpu,
		       const CGEN_INSN *insn ATTRIBUTE_UNUSED,
		       const ARGBUF *abuf ATTRIBUTE_UNUSED,
		       IADDR pc, char *buf)
{
  disassembler_ftype pinsn;
  struct disassemble_info disasm_info;
  SFILE sfile;
  SIM_DESC sd = CPU_STATE (cpu);

  sfile.buffer = sfile.current = buf;
  INIT_DISASSEMBLE_INFO (disasm_info, (FILE *) &sfile,
			 (fprintf_ftype) sim_disasm_sprintf);
  disasm_info.endian = BFD_ENDIAN_LITTLE;
  disasm_info.read_memory_func = sim_disasm_read_memory;
  disasm_info.memory_error_func = sim_disasm_perror_memory;
  disasm_info.application_data = (PTR) cpu;
  pinsn = cris_get_disassembler (STATE_PROG_BFD (sd));
  (*pinsn) (pc, &disasm_info);
}
