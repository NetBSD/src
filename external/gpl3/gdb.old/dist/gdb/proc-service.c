/* <proc_service.h> implementation.

   Copyright (C) 1999-2015 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "regcache.h"
#include "objfiles.h"

#include "gdb_proc_service.h"

#include <sys/procfs.h>

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"


/* Fix-up some broken systems.  */

/* The prototypes in <proc_service.h> are slightly different on older
   systems.  Compensate for the discrepancies.  */

#ifdef PROC_SERVICE_IS_OLD
typedef const struct ps_prochandle *gdb_ps_prochandle_t;
typedef char *gdb_ps_read_buf_t;
typedef char *gdb_ps_write_buf_t;
typedef int gdb_ps_size_t;
#else
typedef struct ps_prochandle *gdb_ps_prochandle_t;
typedef void *gdb_ps_read_buf_t;
typedef const void *gdb_ps_write_buf_t;
typedef size_t gdb_ps_size_t;
#endif


/* Helper functions.  */

/* Convert a psaddr_t to a CORE_ADDR.  */

static CORE_ADDR
ps_addr_to_core_addr (psaddr_t addr)
{
  if (exec_bfd && bfd_get_sign_extend_vma (exec_bfd))
    return (intptr_t) addr;
  else
    return (uintptr_t) addr;
}

/* Convert a CORE_ADDR to a psaddr_t.  */

static psaddr_t
core_addr_to_ps_addr (CORE_ADDR addr)
{
  if (exec_bfd && bfd_get_sign_extend_vma (exec_bfd))
    return (psaddr_t) (intptr_t) addr;
  else
    return (psaddr_t) (uintptr_t) addr;
}

/* Transfer LEN bytes of memory between BUF and address ADDR in the
   process specified by PH.  If WRITE, transfer them to the process,
   else transfer them from the process.  Returns PS_OK for success,
   PS_ERR on failure.

   This is a helper function for ps_pdread and ps_pdwrite.  */

static ps_err_e
ps_xfer_memory (const struct ps_prochandle *ph, psaddr_t addr,
		gdb_byte *buf, size_t len, int write)
{
  struct cleanup *old_chain = save_inferior_ptid ();
  int ret;
  CORE_ADDR core_addr = ps_addr_to_core_addr (addr);

  inferior_ptid = ph->ptid;

  if (write)
    ret = target_write_memory (core_addr, buf, len);
  else
    ret = target_read_memory (core_addr, buf, len);

  do_cleanups (old_chain);

  return (ret == 0 ? PS_OK : PS_ERR);
}


/* Search for the symbol named NAME within the object named OBJ within
   the target process PH.  If the symbol is found the address of the
   symbol is stored in SYM_ADDR.  */

ps_err_e
ps_pglobal_lookup (gdb_ps_prochandle_t ph, const char *obj,
		   const char *name, psaddr_t *sym_addr)
{
  struct bound_minimal_symbol ms;
  struct cleanup *old_chain = save_current_program_space ();
  struct inferior *inf = find_inferior_ptid (ph->ptid);
  ps_err_e result;

  set_current_program_space (inf->pspace);

  /* FIXME: kettenis/2000-09-03: What should we do with OBJ?  */
  ms = lookup_minimal_symbol (name, NULL, NULL);
  if (ms.minsym == NULL)
    result = PS_NOSYM;
  else
    {
      *sym_addr = core_addr_to_ps_addr (BMSYMBOL_VALUE_ADDRESS (ms));
      result = PS_OK;
    }

  do_cleanups (old_chain);
  return result;
}

/* Read SIZE bytes from the target process PH at address ADDR and copy
   them into BUF.  */

ps_err_e
ps_pdread (gdb_ps_prochandle_t ph, psaddr_t addr,
	   gdb_ps_read_buf_t buf, gdb_ps_size_t size)
{
  return ps_xfer_memory (ph, addr, buf, size, 0);
}

/* Write SIZE bytes from BUF into the target process PH at address ADDR.  */

ps_err_e
ps_pdwrite (gdb_ps_prochandle_t ph, psaddr_t addr,
	    gdb_ps_write_buf_t buf, gdb_ps_size_t size)
{
  return ps_xfer_memory (ph, addr, (gdb_byte *) buf, size, 1);
}

/* Get the general registers of LWP LWPID within the target process PH
   and store them in GREGSET.  */

ps_err_e
ps_lgetregs (gdb_ps_prochandle_t ph, lwpid_t lwpid, prgregset_t gregset)
{
  struct cleanup *old_chain = save_inferior_ptid ();
  struct regcache *regcache;

  inferior_ptid = ptid_build (ptid_get_pid (ph->ptid), lwpid, 0);
  regcache = get_thread_arch_regcache (inferior_ptid, target_gdbarch ());

  target_fetch_registers (regcache, -1);
  fill_gregset (regcache, (gdb_gregset_t *) gregset, -1);

  do_cleanups (old_chain);
  return PS_OK;
}

/* Set the general registers of LWP LWPID within the target process PH
   from GREGSET.  */

ps_err_e
ps_lsetregs (gdb_ps_prochandle_t ph, lwpid_t lwpid, const prgregset_t gregset)
{
  struct cleanup *old_chain = save_inferior_ptid ();
  struct regcache *regcache;

  inferior_ptid = ptid_build (ptid_get_pid (ph->ptid), lwpid, 0);
  regcache = get_thread_arch_regcache (inferior_ptid, target_gdbarch ());

  supply_gregset (regcache, (const gdb_gregset_t *) gregset);
  target_store_registers (regcache, -1);

  do_cleanups (old_chain);
  return PS_OK;
}

/* Get the floating-point registers of LWP LWPID within the target
   process PH and store them in FPREGSET.  */

ps_err_e
ps_lgetfpregs (gdb_ps_prochandle_t ph, lwpid_t lwpid,
	       gdb_prfpregset_t *fpregset)
{
  struct cleanup *old_chain = save_inferior_ptid ();
  struct regcache *regcache;

  inferior_ptid = ptid_build (ptid_get_pid (ph->ptid), lwpid, 0);
  regcache = get_thread_arch_regcache (inferior_ptid, target_gdbarch ());

  target_fetch_registers (regcache, -1);
  fill_fpregset (regcache, (gdb_fpregset_t *) fpregset, -1);

  do_cleanups (old_chain);
  return PS_OK;
}

/* Set the floating-point registers of LWP LWPID within the target
   process PH from FPREGSET.  */

ps_err_e
ps_lsetfpregs (gdb_ps_prochandle_t ph, lwpid_t lwpid,
	       const gdb_prfpregset_t *fpregset)
{
  struct cleanup *old_chain = save_inferior_ptid ();
  struct regcache *regcache;

  inferior_ptid = ptid_build (ptid_get_pid (ph->ptid), lwpid, 0);
  regcache = get_thread_arch_regcache (inferior_ptid, target_gdbarch ());

  supply_fpregset (regcache, (const gdb_fpregset_t *) fpregset);
  target_store_registers (regcache, -1);

  do_cleanups (old_chain);
  return PS_OK;
}

/* Return overall process id of the target PH.  Special for GNU/Linux
   -- not used on Solaris.  */

pid_t
ps_getpid (gdb_ps_prochandle_t ph)
{
  return ptid_get_pid (ph->ptid);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_proc_service;

void
_initialize_proc_service (void)
{
  /* This function solely exists to make sure this module is linked
     into the final binary.  */
}
