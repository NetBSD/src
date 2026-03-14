/* Target-dependent code for GNU/Linux on RISC-V processors.
   Copyright (C) 2018-2025 Free Software Foundation, Inc.

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

#include "riscv-tdep.h"
#include "osabi.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "solib-svr4-linux.h"
#include "svr4-tls-tdep.h"
#include "solib-svr4.h"
#include "regset.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "gdbarch.h"
#include "record-full.h"
#include "linux-record.h"
#include "riscv-linux-tdep.h"
#include "inferior.h"

extern unsigned int record_debug;

/* The following value is derived from __NR_rt_sigreturn in
   <include/uapi/asm-generic/unistd.h> from the Linux source tree.  */

#define RISCV_NR_rt_sigreturn 139

/* Define the general register mapping.  The kernel puts the PC at offset 0,
   gdb puts it at offset 32.  Register x0 is always 0 and can be ignored.
   Registers x1 to x31 are in the same place.  */

static const struct regcache_map_entry riscv_linux_gregmap[] =
{
  { 1,  RISCV_PC_REGNUM, 0 },
  { 31, RISCV_RA_REGNUM, 0 }, /* x1 to x31 */
  { 0 }
};

/* Define the FP register mapping.  The kernel puts the 32 FP regs first, and
   then FCSR.  */

static const struct regcache_map_entry riscv_linux_fregmap[] =
{
  { 32, RISCV_FIRST_FP_REGNUM, 0 },
  { 1, RISCV_CSR_FCSR_REGNUM, 0 },
  { 0 }
};

/* Define the general register regset.  */

static const struct regset riscv_linux_gregset =
{
  riscv_linux_gregmap, riscv_supply_regset, regcache_collect_regset
};

/* Define the FP register regset.  */

static const struct regset riscv_linux_fregset =
{
  riscv_linux_fregmap, riscv_supply_regset, regcache_collect_regset
};

/* Define hook for core file support.  */

static void
riscv_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					  iterate_over_regset_sections_cb *cb,
					  void *cb_data,
					  const struct regcache *regcache)
{
  cb (".reg", (32 * riscv_isa_xlen (gdbarch)), (32 * riscv_isa_xlen (gdbarch)),
      &riscv_linux_gregset, NULL, cb_data);
  /* The kernel is adding 8 bytes for FCSR.  */
  cb (".reg2", (32 * riscv_isa_flen (gdbarch)) + 8,
      (32 * riscv_isa_flen (gdbarch)) + 8,
      &riscv_linux_fregset, NULL, cb_data);
}

/* Signal trampoline support.  */

static void riscv_linux_sigframe_init (const struct tramp_frame *self,
				       const frame_info_ptr &this_frame,
				       struct trad_frame_cache *this_cache,
				       CORE_ADDR func);

#define RISCV_INST_LI_A7_SIGRETURN	0x08b00893
#define RISCV_INST_ECALL		0x00000073

static const struct tramp_frame riscv_linux_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { RISCV_INST_LI_A7_SIGRETURN, ULONGEST_MAX },
    { RISCV_INST_ECALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  riscv_linux_sigframe_init,
  NULL
};

/* Runtime signal frames look like this:
   struct rt_sigframe {
     struct siginfo info;
     struct ucontext uc;
   };

   struct ucontext {
     unsigned long __uc_flags;
     struct ucontext *uclink;
     stack_t uc_stack;
     sigset_t uc_sigmask;
     char __glibc_reserved[1024 / 8 - sizeof (sigset_t)];
     mcontext_t uc_mcontext;
   }; */

#define SIGFRAME_SIGINFO_SIZE		128
#define UCONTEXT_MCONTEXT_OFFSET	176

static void
riscv_linux_sigframe_init (const struct tramp_frame *self,
			   const frame_info_ptr &this_frame,
			   struct trad_frame_cache *this_cache,
			   CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int xlen = riscv_isa_xlen (gdbarch);
  int flen = riscv_isa_flen (gdbarch);
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR mcontext_base;
  CORE_ADDR regs_base;

  mcontext_base = frame_sp + SIGFRAME_SIGINFO_SIZE + UCONTEXT_MCONTEXT_OFFSET;

  /* Handle the integer registers.  The first one is PC, followed by x1
     through x31.  */
  regs_base = mcontext_base;
  trad_frame_set_reg_addr (this_cache, RISCV_PC_REGNUM, regs_base);
  for (int i = 1; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, RISCV_ZERO_REGNUM + i,
			     regs_base + (i * xlen));

  /* Handle the FP registers.  First comes the 32 FP registers, followed by
     fcsr.  */
  regs_base += 32 * xlen;
  for (int i = 0; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, RISCV_FIRST_FP_REGNUM + i,
			     regs_base + (i * flen));
  regs_base += 32 * flen;
  trad_frame_set_reg_addr (this_cache, RISCV_CSR_FCSR_REGNUM, regs_base);

  /* Choice of the bottom of the sigframe is somewhat arbitrary.  */
  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* When FRAME is at a syscall instruction (ECALL), return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
riscv_linux_syscall_next_pc (const frame_info_ptr &frame)
{
  const CORE_ADDR pc = get_frame_pc (frame);
  const ULONGEST a7 = get_frame_register_unsigned (frame, RISCV_A7_REGNUM);

  if (a7 == RISCV_NR_rt_sigreturn)
    return frame_unwind_caller_pc (frame);

  return pc + 4 /* Length of the ECALL insn.  */;
}

/* RISC-V process record-replay constructs: syscall, signal etc.  */

static linux_record_tdep riscv_linux_record_tdep;

using regnum_type = int;

/* Record registers from first to last for process-record.  */

static bool
save_registers (struct regcache *regcache, regnum_type first, regnum_type last)
{
  gdb_assert (regcache != nullptr);

  for (regnum_type i = first; i != last; ++i)
    if (record_full_arch_list_add_reg (regcache, i))
      return false;
  return true;
};

/* Record all registers but PC register for process-record.  */

static bool
riscv_all_but_pc_registers_record (struct regcache *regcache)
{
  gdb_assert (regcache != nullptr);

  struct gdbarch *gdbarch = regcache->arch ();
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  const struct riscv_gdbarch_features &features = tdep->isa_features;

  if (!save_registers (regcache, RISCV_ZERO_REGNUM + 1, RISCV_PC_REGNUM))
    return false;

  if (features.flen
      && !save_registers (regcache, RISCV_FIRST_FP_REGNUM,
			  RISCV_LAST_FP_REGNUM + 1))
    return false;

  return true;
}

/* Handler for riscv system call instruction recording.  */

static int
riscv_linux_syscall_record (struct regcache *regcache,
			    unsigned long svc_number)
{
  gdb_assert (regcache != nullptr);

  enum gdb_syscall syscall_gdb = riscv64_canonicalize_syscall (svc_number);

  if (record_debug > 1)
    gdb_printf (gdb_stdlog, "Made syscall %s.\n", plongest (svc_number));

  if (syscall_gdb == gdb_sys_no_syscall)
    {
      warning (_("Process record and replay target doesn't "
		 "support syscall number %s\n"), plongest (svc_number));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn || syscall_gdb == gdb_sys_rt_sigreturn)
    {
      if (!riscv_all_but_pc_registers_record (regcache))
	return -1;
      return 0;
    }

  int ret = record_linux_system_call (syscall_gdb, regcache,
				      &riscv_linux_record_tdep);
  if (ret != 0)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, RISCV_A0_REGNUM))
    return -1;

  return 0;
}

/* Initialize the riscv64_linux_record_tdep.  */

static void
riscv64_linux_record_tdep_init (struct gdbarch *gdbarch,
				struct linux_record_tdep &
				riscv_linux_record_tdep)
{
  gdb_assert (gdbarch != nullptr);

  /* These values are the size of the type that
     will be used in a system call.  */
  riscv_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  riscv_linux_record_tdep.size__old_kernel_stat = 48;
  riscv_linux_record_tdep.size_tms = 32;
  riscv_linux_record_tdep.size_loff_t = 8;
  riscv_linux_record_tdep.size_flock = 32;
  riscv_linux_record_tdep.size_oldold_utsname = 45;
  riscv_linux_record_tdep.size_ustat = 32;
  riscv_linux_record_tdep.size_old_sigaction = 32;
  riscv_linux_record_tdep.size_old_sigset_t = 8;
  riscv_linux_record_tdep.size_rlimit = 16;
  riscv_linux_record_tdep.size_rusage = 144;
  riscv_linux_record_tdep.size_timeval = 8;
  riscv_linux_record_tdep.size_timezone = 8;
  riscv_linux_record_tdep.size_old_gid_t = 2;
  riscv_linux_record_tdep.size_old_uid_t = 2;
  riscv_linux_record_tdep.size_fd_set = 128;
  riscv_linux_record_tdep.size_old_dirent = 268;
  riscv_linux_record_tdep.size_statfs = 120;
  riscv_linux_record_tdep.size_statfs64 = 120;
  riscv_linux_record_tdep.size_sockaddr = 16;
  riscv_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  riscv_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  riscv_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  riscv_linux_record_tdep.size_msghdr = 104;
  riscv_linux_record_tdep.size_itimerval = 16;
  riscv_linux_record_tdep.size_stat = 128;
  riscv_linux_record_tdep.size_old_utsname = 325;
  riscv_linux_record_tdep.size_sysinfo = 112;
  riscv_linux_record_tdep.size_msqid_ds = 104;
  riscv_linux_record_tdep.size_shmid_ds = 88;
  riscv_linux_record_tdep.size_new_utsname = 390;
  riscv_linux_record_tdep.size_timex = 188;
  riscv_linux_record_tdep.size_mem_dqinfo = 72;
  riscv_linux_record_tdep.size_if_dqblk = 68;
  riscv_linux_record_tdep.size_fs_quota_stat = 64;
  riscv_linux_record_tdep.size_timespec = 16;
  riscv_linux_record_tdep.size_pollfd = 8;
  riscv_linux_record_tdep.size_NFS_FHSIZE = 32;
  riscv_linux_record_tdep.size_knfsd_fh = 36;
  riscv_linux_record_tdep.size_TASK_COMM_LEN = 4;
  riscv_linux_record_tdep.size_sigaction = 24;
  riscv_linux_record_tdep.size_sigset_t = 8;
  riscv_linux_record_tdep.size_siginfo_t = 128;
  riscv_linux_record_tdep.size_cap_user_data_t = 8;
  riscv_linux_record_tdep.size_stack_t = 24;
  riscv_linux_record_tdep.size_off_t = riscv_linux_record_tdep.size_long;
  riscv_linux_record_tdep.size_stat64 = 136;
  riscv_linux_record_tdep.size_gid_t = 4;
  riscv_linux_record_tdep.size_uid_t = 4;
  riscv_linux_record_tdep.size_PAGE_SIZE = 4096;
  riscv_linux_record_tdep.size_flock64 = 32;
  riscv_linux_record_tdep.size_user_desc = 37;
  riscv_linux_record_tdep.size_io_event = 32;
  riscv_linux_record_tdep.size_iocb = 64;
  riscv_linux_record_tdep.size_epoll_event = 16;
  riscv_linux_record_tdep.size_itimerspec
    = riscv_linux_record_tdep.size_timespec * 2;
  riscv_linux_record_tdep.size_mq_attr = 64;
  riscv_linux_record_tdep.size_termios = 36;
  riscv_linux_record_tdep.size_termios2 = 44;
  riscv_linux_record_tdep.size_pid_t = 4;
  riscv_linux_record_tdep.size_winsize = 8;
  riscv_linux_record_tdep.size_serial_struct = 72;
  riscv_linux_record_tdep.size_serial_icounter_struct = 80;
  riscv_linux_record_tdep.size_hayes_esp_config = 12;
  riscv_linux_record_tdep.size_size_t = 8;
  riscv_linux_record_tdep.size_iovec = 16;
  riscv_linux_record_tdep.size_time_t = 8;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  riscv_linux_record_tdep.ioctl_TCGETS = 0x5401;
  riscv_linux_record_tdep.ioctl_TCSETS = 0x5402;
  riscv_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  riscv_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  riscv_linux_record_tdep.ioctl_TCGETA = 0x5405;
  riscv_linux_record_tdep.ioctl_TCSETA = 0x5406;
  riscv_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  riscv_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  riscv_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  riscv_linux_record_tdep.ioctl_TCXONC = 0x540a;
  riscv_linux_record_tdep.ioctl_TCFLSH = 0x540b;
  riscv_linux_record_tdep.ioctl_TIOCEXCL = 0x540c;
  riscv_linux_record_tdep.ioctl_TIOCNXCL = 0x540d;
  riscv_linux_record_tdep.ioctl_TIOCSCTTY = 0x540e;
  riscv_linux_record_tdep.ioctl_TIOCGPGRP = 0x540f;
  riscv_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  riscv_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  riscv_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  riscv_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  riscv_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  riscv_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  riscv_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  riscv_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  riscv_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  riscv_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  riscv_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541a;
  riscv_linux_record_tdep.ioctl_FIONREAD = 0x541b;
  riscv_linux_record_tdep.ioctl_TIOCINQ
    = riscv_linux_record_tdep.ioctl_FIONREAD;
  riscv_linux_record_tdep.ioctl_TIOCLINUX = 0x541c;
  riscv_linux_record_tdep.ioctl_TIOCCONS = 0x541d;
  riscv_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541e;
  riscv_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541f;
  riscv_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  riscv_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  riscv_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  riscv_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  riscv_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  riscv_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  riscv_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  riscv_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  riscv_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  riscv_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  riscv_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  riscv_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  riscv_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  riscv_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  riscv_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  riscv_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  riscv_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  riscv_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  riscv_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  riscv_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  riscv_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  riscv_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  riscv_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  riscv_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  riscv_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  riscv_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  riscv_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545a;
  riscv_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545b;
  riscv_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545c;
  riscv_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545d;
  riscv_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545e;
  riscv_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545f;
  riscv_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  riscv_linux_record_tdep.fcntl_F_GETLK = 5;
  riscv_linux_record_tdep.fcntl_F_GETLK64 = 12;
  riscv_linux_record_tdep.fcntl_F_SETLK64 = 13;
  riscv_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  riscv_linux_record_tdep.arg1 = RISCV_A0_REGNUM;
  riscv_linux_record_tdep.arg2 = RISCV_A1_REGNUM;
  riscv_linux_record_tdep.arg3 = RISCV_A2_REGNUM;
  riscv_linux_record_tdep.arg4 = RISCV_A3_REGNUM;
  riscv_linux_record_tdep.arg5 = RISCV_A4_REGNUM;
  riscv_linux_record_tdep.arg6 = RISCV_A5_REGNUM;
}

/* Fetch and return the TLS DTV (dynamic thread vector) address for PTID.
   Throw a suitable TLS error if something goes wrong.  */

static CORE_ADDR
riscv_linux_get_tls_dtv_addr (struct gdbarch *gdbarch, ptid_t ptid,
			      svr4_tls_libc libc)
{
  /* On RISC-V, the thread pointer is found in TP.  */
  regcache *regcache
    = get_thread_arch_regcache (current_inferior (), ptid, gdbarch);
  int thread_pointer_regnum = RISCV_TP_REGNUM;
  target_fetch_registers (regcache, thread_pointer_regnum);
  ULONGEST thr_ptr;
  if (regcache->cooked_read (thread_pointer_regnum, &thr_ptr) != REG_VALID)
    throw_error (TLS_GENERIC_ERROR, _("Unable to fetch thread pointer"));

  CORE_ADDR dtv_ptr_addr;
  switch (libc)
    {
      case svr4_tls_libc_musl:
	/* MUSL: The DTV pointer is found at the very end of the pthread
	   struct which is located *before* the thread pointer.  I.e.
	   the thread pointer will be just beyond the end of the struct,
	   so the address of the DTV pointer is found one pointer-size
	   before the thread pointer.  */
	dtv_ptr_addr
	  = thr_ptr - (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
	break;
      case svr4_tls_libc_glibc:
	/* GLIBC:  The thread pointer (TP) points just beyond the end of
	   the TCB (thread control block).  On RISC-V, this struct
	   (tcbhead_t) is defined to contain two pointers.  The first is
	   a pointer to the DTV and the second is a pointer to private
	   data.  So the DTV pointer address is 16 bytes (i.e. the size of
	   two pointers) before thread pointer.  */

	dtv_ptr_addr
	  = thr_ptr - 2 * (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
	break;
      default:
	throw_error (TLS_GENERIC_ERROR, _("Unknown RISC-V C library"));
	break;
    }

  gdb::byte_vector buf (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
  if (target_read_memory (dtv_ptr_addr, buf.data (), buf.size ()) != 0)
    throw_error (TLS_GENERIC_ERROR, _("Unable to fetch DTV address"));

  const struct builtin_type *builtin = builtin_type (gdbarch);
  CORE_ADDR dtv_addr = gdbarch_pointer_to_address
			 (gdbarch, builtin->builtin_data_ptr, buf.data ());
  return dtv_addr;
}

/* For internal TLS lookup, return the DTP offset, which is the offset
   to subtract from a DTV entry, in order to obtain the address of the
   TLS block.  */

static ULONGEST
riscv_linux_get_tls_dtp_offset (struct gdbarch *gdbarch, ptid_t ptid,
				svr4_tls_libc libc)
{
  if (libc == svr4_tls_libc_musl)
    {
      /* This value is DTP_OFFSET in MUSL's arch/riscv64/pthread_arch.h.
	 It represents the value to subtract from the DTV entry, once
	 it has been loaded.  */
      return 0x800;
    }
  else
    return 0;
}

/* Initialize RISC-V Linux ABI info.  */

static void
riscv_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  set_gdbarch_get_next_pcs (gdbarch, riscv_software_single_step);

  set_solib_svr4_ops (gdbarch, (riscv_isa_xlen (gdbarch) == 4
				? make_linux_ilp32_svr4_solib_ops
				: make_linux_lp64_svr4_solib_ops));

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
  set_gdbarch_get_thread_local_address (gdbarch,
					svr4_tls_get_thread_local_address);
  svr4_tls_register_tls_methods (info, gdbarch, riscv_linux_get_tls_dtv_addr,
				 riscv_linux_get_tls_dtp_offset);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, riscv_linux_iterate_over_regset_sections);

  tramp_frame_prepend_unwinder (gdbarch, &riscv_linux_sigframe);

  tdep->syscall_next_pc = riscv_linux_syscall_next_pc;
  tdep->riscv_syscall_record = riscv_linux_syscall_record;

  riscv64_linux_record_tdep_init (gdbarch, riscv_linux_record_tdep);
}

/* Initialize RISC-V Linux target support.  */

INIT_GDB_FILE (riscv_linux_tdep)
{
  gdbarch_register_osabi (bfd_arch_riscv, 0, GDB_OSABI_LINUX,
			  riscv_linux_init_abi);
}
