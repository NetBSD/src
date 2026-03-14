/* Target-dependent code for the GNU Hurd.
   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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
#include "extract-store-integer.h"
#include "gdbcore.h"
#include "osabi.h"
#include "solib-svr4.h"

#include "amd64-tdep.h"
#include "glibc-tdep.h"

/* Recognizing signal handler frames.  */

/* When the GNU/Hurd libc calls a signal handler, the return address points
   inside the trampoline assembly snippet.

   If the trampoline function name can not be identified, we resort to reading
   memory from the process in order to identify it.  */

static const gdb_byte gnu_sigtramp_code[] =
{
/* rpc_wait_trampoline: */
  0x48, 0xc7, 0xc0, 0xe7, 0xff, 0xff, 0xff,	/* mov    $-25,%rax */
  0x0f, 0x05,					/* syscall */
  0x49, 0x89, 0x04, 0x24,			/* mov    %rax,(%r12) */
  0x48, 0x89, 0xdc,				/* mov    %rbx,%rsp */

/* trampoline: */
  0x5f,			                        /* pop    %rdi */
  0x5e,						/* pop    %rsi */
  0x5a,						/* pop    %rdx */
  0x48, 0x83, 0xc4, 0x08,			/* add    $0x8,%rsp */
  0x41, 0xff, 0xd5,				/* call   *%r13 */

/* RA HERE */
  0x48, 0x8b, 0x7c, 0x24, 0x10,			/* mov    0x10(%rsp),%rdi */
  0xc3,						/* ret */

/* firewall: */
  0xf4,						/* hlt */
};

#define GNU_SIGTRAMP_LEN (sizeof gnu_sigtramp_code)
#define GNU_SIGTRAMP_TAIL 7			/* length of tail after RA */

/* If THIS_FRAME is a sigtramp routine, return the address of the
   start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
amd64_gnu_sigtramp_start (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[GNU_SIGTRAMP_LEN];

  if (!safe_frame_unwind_memory (this_frame,
				 pc + GNU_SIGTRAMP_TAIL - GNU_SIGTRAMP_LEN,
				 buf))
    return 0;

  if (memcmp (buf, gnu_sigtramp_code, GNU_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* Return whether THIS_FRAME corresponds to a Hurd sigtramp routine.  */

static int
amd64_gnu_sigtramp_p (const frame_info_ptr &this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If we have a NAME, we can check for the trampoline function */
  if (name != NULL && strcmp (name, "trampoline") == 0)
    return 1;

  return amd64_gnu_sigtramp_start (this_frame) != 0;
}

/* Offset to sc_i386_thread_state in sigcontext, from <bits/sigcontext.h>.  */
#define AMD64_GNU_SIGCONTEXT_THREAD_STATE_OFFSET 32

/* Assuming THIS_FRAME is a Hurd sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
amd64_gnu_sigcontext_addr (const frame_info_ptr &this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc;
  CORE_ADDR sp;
  gdb_byte buf[8];

  get_frame_register (this_frame, AMD64_RSP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 8, byte_order);

  pc = amd64_gnu_sigtramp_start (this_frame);
  if (pc)
    {
      CORE_ADDR sigcontext_addr;

      /* The sigcontext structure address is passed as the third argument
       * of the signal handler but %RDX is not saved across calls. Luckily,
       * the structured is saved underneath the &__sigreturn and a dummy word
       * to fill the slot for the address for __sigreturn to return to.
       */
      read_memory (sp + 16, buf, 8);
      sigcontext_addr = extract_unsigned_integer (buf, 8, byte_order);
      return sigcontext_addr + AMD64_GNU_SIGCONTEXT_THREAD_STATE_OFFSET;
    }

  error (_("Couldn't recognize signal trampoline."));
  return 0;
}

/* Mapping between the general-purpose registers in `struct
   sigcontext' format (starting at sc_i386_thread_state)
   and GDB's register cache layout.  */

/* From <bits/sigcontext.h>.  */
static int amd64_gnu_sc_reg_offset[] =
{
  15 * 8,			/* %rax */
  12 * 8,			/* %rbx */
  14 * 8,			/* %rcx */
  13 * 8,			/* %rdx */
  10 * 8,			/* %rsi */
  9 * 8,			/* %rdi */
  10 * 8,			/* %rbp */
  11 * 8,			/* %rsp */
  0 * 8,			/* %r8 ...  */
  8 * 8,
  7 * 8,
  6 * 8,
  3 * 8,
  2 * 8,
  1 * 8,
  0 * 8,			/* ... %r15 */
  16 * 8,			/* %rip */
  18 * 8,			/* %eflags */
  17 * 8,			/* %cs */
};

/* From <sys/ucontext.h>.  */
static int amd64_gnu_gregset_reg_offset[] =
{
  10 * 8,			/* %rax */
  5 * 8,			/* %rbx */
  11 * 8,			/* %rcx */
  12 * 8,			/* %rdx */
  13 * 8,			/* %rsi */
  14 * 8,			/* %rdi */
  4 * 8,			/* %rbp */
  19 * 8,			/* %rsp */
  9 * 8,			/* %r8 ...  */
  8 * 8,
  7 * 8,
  6 * 8,
  3 * 8,
  2 * 8,
  1 * 8,
  0 * 8,			/* ... %r15 */
  16 * 8,			/* %rip */
  18 * 8,			/* %eflags */
  17 * 8,			/* %cs */
  -1,				  /* %ss */
  -1,				  /* %ds */
  -1,				  /* %es */
  -1,				  /* %fs */
  -1,				  /* %gs */
};

static void
amd64_gnu_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  amd64_init_abi (info, gdbarch,
      amd64_target_description (X86_XSTATE_SSE_MASK, true));

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
      svr4_fetch_objfile_link_map);

  /* Hurd uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* Hurd uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  tdep->gregset_reg_offset = amd64_gnu_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (amd64_gnu_gregset_reg_offset);
  tdep->sizeof_gregset = 21 * 8; /* sizeof (struct i386_thread_state); */

  tdep->sigtramp_p = amd64_gnu_sigtramp_p;
  tdep->sigcontext_addr = amd64_gnu_sigcontext_addr;
  tdep->sc_reg_offset = amd64_gnu_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (amd64_gnu_sc_reg_offset);

  /* Hurd uses SVR4-style shared libraries.  */
  set_solib_svr4_ops (gdbarch, make_svr4_lp64_solib_ops);
}

INIT_GDB_FILE (amd64_gnu_tdep)
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_HURD, amd64_gnu_init_abi);
}
