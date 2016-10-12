/* SPU native-dependent code for GDB, the GNU debugger.
   Copyright (C) 2006-2016 Free Software Foundation, Inc.

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>.

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
#include "gdbcore.h"
#include "target.h"
#include "inferior.h"
#include "inf-child.h"
#include "inf-ptrace.h"
#include "regcache.h"
#include "symfile.h"
#include "gdb_wait.h"
#include "gdbthread.h"
#include "gdb_bfd.h"

#include "nat/gdb_ptrace.h"
#include <asm/ptrace.h>
#include <sys/types.h>

#include "spu-tdep.h"

/* PPU side system calls.  */
#define INSTR_SC	0x44000002
#define NR_spu_run	0x0116


/* Fetch PPU register REGNO.  */
static ULONGEST
fetch_ppc_register (int regno)
{
  PTRACE_TYPE_RET res;

  int tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid);

#ifndef __powerpc64__
  /* If running as a 32-bit process on a 64-bit system, we attempt
     to get the full 64-bit register content of the target process.
     If the PPC special ptrace call fails, we're on a 32-bit system;
     just fall through to the regular ptrace call in that case.  */
  {
    gdb_byte buf[8];

    errno = 0;
    ptrace (PPC_PTRACE_PEEKUSR_3264, tid,
	    (PTRACE_TYPE_ARG3) (regno * 8), buf);
    if (errno == 0)
      ptrace (PPC_PTRACE_PEEKUSR_3264, tid,
	      (PTRACE_TYPE_ARG3) (regno * 8 + 4), buf + 4);
    if (errno == 0)
      return (ULONGEST) *(uint64_t *)buf;
  }
#endif

  errno = 0;
  res = ptrace (PT_READ_U, tid,
	 	(PTRACE_TYPE_ARG3) (regno * sizeof (PTRACE_TYPE_RET)), 0);
  if (errno != 0)
    {
      char mess[128];
      xsnprintf (mess, sizeof mess, "reading PPC register #%d", regno);
      perror_with_name (_(mess));
    }

  return (ULONGEST) (unsigned long) res;
}

/* Fetch WORD from PPU memory at (aligned) MEMADDR in thread TID.  */
static int
fetch_ppc_memory_1 (int tid, ULONGEST memaddr, PTRACE_TYPE_RET *word)
{
  errno = 0;

#ifndef __powerpc64__
  if (memaddr >> 32)
    {
      uint64_t addr_8 = (uint64_t) memaddr;
      ptrace (PPC_PTRACE_PEEKTEXT_3264, tid, (PTRACE_TYPE_ARG3) &addr_8, word);
    }
  else
#endif
    *word = ptrace (PT_READ_I, tid, (PTRACE_TYPE_ARG3) (size_t) memaddr, 0);

  return errno;
}

/* Store WORD into PPU memory at (aligned) MEMADDR in thread TID.  */
static int
store_ppc_memory_1 (int tid, ULONGEST memaddr, PTRACE_TYPE_RET word)
{
  errno = 0;

#ifndef __powerpc64__
  if (memaddr >> 32)
    {
      uint64_t addr_8 = (uint64_t) memaddr;
      ptrace (PPC_PTRACE_POKEDATA_3264, tid, (PTRACE_TYPE_ARG3) &addr_8, word);
    }
  else
#endif
    ptrace (PT_WRITE_D, tid, (PTRACE_TYPE_ARG3) (size_t) memaddr, word);

  return errno;
}

/* Fetch LEN bytes of PPU memory at MEMADDR to MYADDR.  */
static int
fetch_ppc_memory (ULONGEST memaddr, gdb_byte *myaddr, int len)
{
  int i, ret;

  ULONGEST addr = memaddr & -(ULONGEST) sizeof (PTRACE_TYPE_RET);
  int count = ((((memaddr + len) - addr) + sizeof (PTRACE_TYPE_RET) - 1)
	       / sizeof (PTRACE_TYPE_RET));
  PTRACE_TYPE_RET *buffer;

  int tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid);

  buffer = (PTRACE_TYPE_RET *) alloca (count * sizeof (PTRACE_TYPE_RET));
  for (i = 0; i < count; i++, addr += sizeof (PTRACE_TYPE_RET))
    {
      ret = fetch_ppc_memory_1 (tid, addr, &buffer[i]);
      if (ret)
	return ret;
    }

  memcpy (myaddr,
	  (char *) buffer + (memaddr & (sizeof (PTRACE_TYPE_RET) - 1)),
	  len);

  return 0;
}

/* Store LEN bytes from MYADDR to PPU memory at MEMADDR.  */
static int
store_ppc_memory (ULONGEST memaddr, const gdb_byte *myaddr, int len)
{
  int i, ret;

  ULONGEST addr = memaddr & -(ULONGEST) sizeof (PTRACE_TYPE_RET);
  int count = ((((memaddr + len) - addr) + sizeof (PTRACE_TYPE_RET) - 1)
	       / sizeof (PTRACE_TYPE_RET));
  PTRACE_TYPE_RET *buffer;

  int tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid);

  buffer = (PTRACE_TYPE_RET *) alloca (count * sizeof (PTRACE_TYPE_RET));

  if (addr != memaddr || len < (int) sizeof (PTRACE_TYPE_RET))
    {
      ret = fetch_ppc_memory_1 (tid, addr, &buffer[0]);
      if (ret)
	return ret;
    }

  if (count > 1)
    {
      ret = fetch_ppc_memory_1 (tid, addr + (count - 1)
					       * sizeof (PTRACE_TYPE_RET),
				&buffer[count - 1]);
      if (ret)
	return ret;
    }

  memcpy ((char *) buffer + (memaddr & (sizeof (PTRACE_TYPE_RET) - 1)),
          myaddr, len);

  for (i = 0; i < count; i++, addr += sizeof (PTRACE_TYPE_RET))
    {
      ret = store_ppc_memory_1 (tid, addr, buffer[i]);
      if (ret)
	return ret;
    }

  return 0;
}


/* If the PPU thread is currently stopped on a spu_run system call,
   return to FD and ADDR the file handle and NPC parameter address
   used with the system call.  Return non-zero if successful.  */
static int 
parse_spufs_run (int *fd, ULONGEST *addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  gdb_byte buf[4];
  ULONGEST pc = fetch_ppc_register (32);  /* nip */

  /* Fetch instruction preceding current NIP.  */
  if (fetch_ppc_memory (pc-4, buf, 4) != 0)
    return 0;
  /* It should be a "sc" instruction.  */
  if (extract_unsigned_integer (buf, 4, byte_order) != INSTR_SC)
    return 0;
  /* System call number should be NR_spu_run.  */
  if (fetch_ppc_register (0) != NR_spu_run)
    return 0;

  /* Register 3 contains fd, register 4 the NPC param pointer.  */
  *fd = fetch_ppc_register (34);  /* orig_gpr3 */
  *addr = fetch_ppc_register (4);
  return 1;
}


/* Implement the to_xfer_partial target_ops method for TARGET_OBJECT_SPU.
   Copy LEN bytes at OFFSET in spufs file ANNEX into/from READBUF or WRITEBUF,
   using the /proc file system.  */

static enum target_xfer_status
spu_proc_xfer_spu (const char *annex, gdb_byte *readbuf,
		   const gdb_byte *writebuf,
		   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  char buf[128];
  int fd = 0;
  int ret = -1;
  int pid = ptid_get_pid (inferior_ptid);

  if (!annex)
    return TARGET_XFER_EOF;

  xsnprintf (buf, sizeof buf, "/proc/%d/fd/%s", pid, annex);
  fd = open (buf, writebuf? O_WRONLY : O_RDONLY);
  if (fd <= 0)
    return TARGET_XFER_E_IO;

  if (offset != 0
      && lseek (fd, (off_t) offset, SEEK_SET) != (off_t) offset)
    {
      close (fd);
      return TARGET_XFER_EOF;
    }

  if (writebuf)
    ret = write (fd, writebuf, (size_t) len);
  else if (readbuf)
    ret = read (fd, readbuf, (size_t) len);

  close (fd);
  if (ret < 0)
    return TARGET_XFER_E_IO;
  else if (ret == 0)
    return TARGET_XFER_EOF;
  else
    {
      *xfered_len = (ULONGEST) ret;
      return TARGET_XFER_OK;
    }
}


/* Inferior memory should contain an SPE executable image at location ADDR.
   Allocate a BFD representing that executable.  Return NULL on error.  */

static void *
spu_bfd_iovec_open (struct bfd *nbfd, void *open_closure)
{
  return open_closure;
}

static int
spu_bfd_iovec_close (struct bfd *nbfd, void *stream)
{
  xfree (stream);

  /* Zero means success.  */
  return 0;
}

static file_ptr
spu_bfd_iovec_pread (struct bfd *abfd, void *stream, void *buf,
	             file_ptr nbytes, file_ptr offset)
{
  ULONGEST addr = *(ULONGEST *)stream;

  if (fetch_ppc_memory (addr + offset, (gdb_byte *)buf, nbytes) != 0)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return -1;
    }

  return nbytes;
}

static int
spu_bfd_iovec_stat (struct bfd *abfd, void *stream, struct stat *sb)
{
  /* We don't have an easy way of finding the size of embedded spu
     images.  We could parse the in-memory ELF header and section
     table to find the extent of the last section but that seems
     pointless when the size is needed only for checks of other
     parsed values in dbxread.c.  */
  memset (sb, 0, sizeof (struct stat));
  sb->st_size = INT_MAX;
  return 0;
}

static bfd *
spu_bfd_open (ULONGEST addr)
{
  struct bfd *nbfd;
  asection *spu_name;

  ULONGEST *open_closure = XNEW (ULONGEST);
  *open_closure = addr;

  nbfd = gdb_bfd_openr_iovec ("<in-memory>", "elf32-spu",
			      spu_bfd_iovec_open, open_closure,
			      spu_bfd_iovec_pread, spu_bfd_iovec_close,
			      spu_bfd_iovec_stat);
  if (!nbfd)
    return NULL;

  if (!bfd_check_format (nbfd, bfd_object))
    {
      gdb_bfd_unref (nbfd);
      return NULL;
    }

  /* Retrieve SPU name note and update BFD name.  */
  spu_name = bfd_get_section_by_name (nbfd, ".note.spu_name");
  if (spu_name)
    {
      int sect_size = bfd_section_size (nbfd, spu_name);
      if (sect_size > 20)
	{
	  char *buf = (char *)alloca (sect_size - 20 + 1);
	  bfd_get_section_contents (nbfd, spu_name, buf, 20, sect_size - 20);
	  buf[sect_size - 20] = '\0';

	  xfree ((char *)nbfd->filename);
	  nbfd->filename = xstrdup (buf);
	}
    }

  return nbfd;
}

/* INFERIOR_FD is a file handle passed by the inferior to the
   spu_run system call.  Assuming the SPE context was allocated
   by the libspe library, try to retrieve the main SPE executable
   file from its copy within the target process.  */
static void
spu_symbol_file_add_from_memory (int inferior_fd)
{
  ULONGEST addr;
  struct bfd *nbfd;

  gdb_byte id[128];
  char annex[32];
  ULONGEST len;
  enum target_xfer_status status;

  /* Read object ID.  */
  xsnprintf (annex, sizeof annex, "%d/object-id", inferior_fd);
  status = spu_proc_xfer_spu (annex, id, NULL, 0, sizeof id, &len);
  if (status != TARGET_XFER_OK || len >= sizeof id)
    return;
  id[len] = 0;
  addr = strtoulst ((const char *) id, NULL, 16);
  if (!addr)
    return;

  /* Open BFD representing SPE executable and read its symbols.  */
  nbfd = spu_bfd_open (addr);
  if (nbfd)
    {
      struct cleanup *cleanup = make_cleanup_bfd_unref (nbfd);

      symbol_file_add_from_bfd (nbfd, bfd_get_filename (nbfd),
				SYMFILE_VERBOSE | SYMFILE_MAINLINE,
				NULL, 0, NULL);
      do_cleanups (cleanup);
    }
}


/* Override the post_startup_inferior routine to continue running
   the inferior until the first spu_run system call.  */
static void
spu_child_post_startup_inferior (struct target_ops *self, ptid_t ptid)
{
  int fd;
  ULONGEST addr;

  int tid = ptid_get_lwp (ptid);
  if (tid == 0)
    tid = ptid_get_pid (ptid);
  
  while (!parse_spufs_run (&fd, &addr))
    {
      ptrace (PT_SYSCALL, tid, (PTRACE_TYPE_ARG3) 0, 0);
      waitpid (tid, NULL, __WALL | __WNOTHREAD);
    }
}

/* Override the post_attach routine to try load the SPE executable
   file image from its copy inside the target process.  */
static void
spu_child_post_attach (struct target_ops *self, int pid)
{
  int fd;
  ULONGEST addr;

  /* Like child_post_startup_inferior, if we happened to attach to
     the inferior while it wasn't currently in spu_run, continue 
     running it until we get back there.  */
  while (!parse_spufs_run (&fd, &addr))
    {
      ptrace (PT_SYSCALL, pid, (PTRACE_TYPE_ARG3) 0, 0);
      waitpid (pid, NULL, __WALL | __WNOTHREAD);
    }

  /* If the user has not provided an executable file, try to extract
     the image from inside the target process.  */
  if (!get_exec_file (0))
    spu_symbol_file_add_from_memory (fd);
}

/* Wait for child PTID to do something.  Return id of the child,
   minus_one_ptid in case of error; store status into *OURSTATUS.  */
static ptid_t
spu_child_wait (struct target_ops *ops,
		ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  int save_errno;
  int status;
  pid_t pid;

  do
    {
      set_sigint_trap ();	/* Causes SIGINT to be passed on to the
				   attached process.  */

      pid = waitpid (ptid_get_pid (ptid), &status, 0);
      if (pid == -1 && errno == ECHILD)
	/* Try again with __WCLONE to check cloned processes.  */
	pid = waitpid (ptid_get_pid (ptid), &status, __WCLONE);

      save_errno = errno;

      /* Make sure we don't report an event for the exit of the
         original program, if we've detached from it.  */
      if (pid != -1 && !WIFSTOPPED (status)
	  && pid != ptid_get_pid (inferior_ptid))
	{
	  pid = -1;
	  save_errno = EINTR;
	}

      clear_sigint_trap ();
    }
  while (pid == -1 && save_errno == EINTR);

  if (pid == -1)
    {
      warning (_("Child process unexpectedly missing: %s"),
	       safe_strerror (save_errno));

      /* Claim it exited with unknown signal.  */
      ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
      ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
      return inferior_ptid;
    }

  store_waitstatus (ourstatus, status);
  return pid_to_ptid (pid);
}

/* Override the fetch_inferior_register routine.  */
static void
spu_fetch_inferior_registers (struct target_ops *ops,
			      struct regcache *regcache, int regno)
{
  int fd;
  ULONGEST addr;

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (&fd, &addr))
    return;

  /* The ID register holds the spufs file handle.  */
  if (regno == -1 || regno == SPU_ID_REGNUM)
    {
      struct gdbarch *gdbarch = get_regcache_arch (regcache);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      gdb_byte buf[4];
      store_unsigned_integer (buf, 4, byte_order, fd);
      regcache_raw_supply (regcache, SPU_ID_REGNUM, buf);
    }

  /* The NPC register is found at ADDR.  */
  if (regno == -1 || regno == SPU_PC_REGNUM)
    {
      gdb_byte buf[4];
      if (fetch_ppc_memory (addr, buf, 4) == 0)
	regcache_raw_supply (regcache, SPU_PC_REGNUM, buf);
    }

  /* The GPRs are found in the "regs" spufs file.  */
  if (regno == -1 || (regno >= 0 && regno < SPU_NUM_GPRS))
    {
      gdb_byte buf[16 * SPU_NUM_GPRS];
      char annex[32];
      int i;
      ULONGEST len;

      xsnprintf (annex, sizeof annex, "%d/regs", fd);
      if ((spu_proc_xfer_spu (annex, buf, NULL, 0, sizeof buf, &len)
	   == TARGET_XFER_OK)
	  && len == sizeof buf)
	for (i = 0; i < SPU_NUM_GPRS; i++)
	  regcache_raw_supply (regcache, i, buf + i*16);
    }
}

/* Override the store_inferior_register routine.  */
static void
spu_store_inferior_registers (struct target_ops *ops,
			      struct regcache *regcache, int regno)
{
  int fd;
  ULONGEST addr;

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (&fd, &addr))
    return;

  /* The NPC register is found at ADDR.  */
  if (regno == -1 || regno == SPU_PC_REGNUM)
    {
      gdb_byte buf[4];
      regcache_raw_collect (regcache, SPU_PC_REGNUM, buf);
      store_ppc_memory (addr, buf, 4);
    }

  /* The GPRs are found in the "regs" spufs file.  */
  if (regno == -1 || (regno >= 0 && regno < SPU_NUM_GPRS))
    {
      gdb_byte buf[16 * SPU_NUM_GPRS];
      char annex[32];
      int i;
      ULONGEST len;

      for (i = 0; i < SPU_NUM_GPRS; i++)
	regcache_raw_collect (regcache, i, buf + i*16);

      xsnprintf (annex, sizeof annex, "%d/regs", fd);
      spu_proc_xfer_spu (annex, NULL, buf, 0, sizeof buf, &len);
    }
}

/* Override the to_xfer_partial routine.  */
static enum target_xfer_status
spu_xfer_partial (struct target_ops *ops,
		  enum target_object object, const char *annex,
		  gdb_byte *readbuf, const gdb_byte *writebuf,
		  ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  if (object == TARGET_OBJECT_SPU)
    return spu_proc_xfer_spu (annex, readbuf, writebuf, offset, len,
			      xfered_len);

  if (object == TARGET_OBJECT_MEMORY)
    {
      int fd;
      ULONGEST addr;
      char mem_annex[32], lslr_annex[32];
      gdb_byte buf[32];
      ULONGEST lslr;
      enum target_xfer_status ret;

      /* We must be stopped on a spu_run system call.  */
      if (!parse_spufs_run (&fd, &addr))
	return TARGET_XFER_EOF;

      /* Use the "mem" spufs file to access SPU local store.  */
      xsnprintf (mem_annex, sizeof mem_annex, "%d/mem", fd);
      ret = spu_proc_xfer_spu (mem_annex, readbuf, writebuf, offset, len,
			       xfered_len);
      if (ret == TARGET_XFER_OK)
	return ret;

      /* SPU local store access wraps the address around at the
	 local store limit.  We emulate this here.  To avoid needing
	 an extra access to retrieve the LSLR, we only do that after
	 trying the original address first, and getting end-of-file.  */
      xsnprintf (lslr_annex, sizeof lslr_annex, "%d/lslr", fd);
      memset (buf, 0, sizeof buf);
      if (spu_proc_xfer_spu (lslr_annex, buf, NULL, 0, sizeof buf, xfered_len)
	  != TARGET_XFER_OK)
	return ret;

      lslr = strtoulst ((const char *) buf, NULL, 16);
      return spu_proc_xfer_spu (mem_annex, readbuf, writebuf,
				offset & lslr, len, xfered_len);
    }

  return TARGET_XFER_E_IO;
}

/* Override the to_can_use_hw_breakpoint routine.  */
static int
spu_can_use_hw_breakpoint (struct target_ops *self,
			   enum bptype type, int cnt, int othertype)
{
  return 0;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_spu_nat;

/* Initialize SPU native target.  */
void 
_initialize_spu_nat (void)
{
  /* Generic ptrace methods.  */
  struct target_ops *t;
  t = inf_ptrace_target ();

  /* Add SPU methods.  */
  t->to_post_attach = spu_child_post_attach;  
  t->to_post_startup_inferior = spu_child_post_startup_inferior;
  t->to_wait = spu_child_wait;
  t->to_fetch_registers = spu_fetch_inferior_registers;
  t->to_store_registers = spu_store_inferior_registers;
  t->to_xfer_partial = spu_xfer_partial;
  t->to_can_use_hw_breakpoint = spu_can_use_hw_breakpoint;

  /* Register SPU target.  */
  add_target (t);
}
