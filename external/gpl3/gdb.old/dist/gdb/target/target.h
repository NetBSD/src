/* Declarations for common target functions.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#ifndef TARGET_COMMON_H
#define TARGET_COMMON_H

#include "target/waitstatus.h"
/* This header is a stopgap until more code is shared.  */

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Return zero for success,
   nonzero if any error occurs.  This function must be provided by
   the client.  Implementations of this function may define and use
   their own error codes, but functions in the common, nat and target
   directories must treat the return code as opaque.  No guarantee is
   made about the contents of the data at MYADDR if any error
   occurs.  */

extern int target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr,
			       ssize_t len);

/* Read an unsigned 32-bit integer in the target's format from target
   memory at address MEMADDR, storing the result in GDB's format in
   GDB's memory at RESULT.  Return zero for success, nonzero if any
   error occurs.  This function must be provided by the client.
   Implementations of this function may define and use their own error
   codes, but functions in the common, nat and target directories must
   treat the return code as opaque.  No guarantee is made about the
   contents of the data at RESULT if any error occurs.  */

extern int target_read_uint32 (CORE_ADDR memaddr, uint32_t *result);

/* Write LEN bytes from MYADDR to target memory at address MEMADDR.
   Return zero for success, nonzero if any error occurs.  This
   function must be provided by the client.  Implementations of this
   function may define and use their own error codes, but functions
   in the common, nat and target directories must treat the return
   code as opaque.  No guarantee is made about the contents of the
   data at MEMADDR if any error occurs.  */

extern int target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr,
				ssize_t len);

/* Cause the target to stop in a continuable fashion--for instance,
   under Unix, this should act like SIGSTOP--and wait for the target
   to be stopped before returning.  This function must be provided by
   the client.  */

extern void target_stop_and_wait (ptid_t ptid);

/* Restart a target previously stopped by target_stop_and_wait.
   No signal is delivered to the target.  This function must be
   provided by the client.  */

extern void target_continue_no_signal (ptid_t ptid);

#endif /* TARGET_COMMON_H */
