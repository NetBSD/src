/* Target-dependent code for GNU/Linux, architecture independent.

   Copyright (C) 2025 Free Software Foundation, Inc.

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

#ifndef GDB_SVR4_TLS_TDEP_H
#define GDB_SVR4_TLS_TDEP_H

/* C library variants for TLS lookup.  */

enum svr4_tls_libc
{
  svr4_tls_libc_unknown,
  svr4_tls_libc_musl,
  svr4_tls_libc_glibc
};

/* Function type for "get_tls_dtv_addr" method.  */

typedef CORE_ADDR (get_tls_dtv_addr_ftype) (struct gdbarch *gdbarch,
					    ptid_t ptid,
					    enum svr4_tls_libc libc);

/* Function type for "get_tls_dtp_offset" method.  */

typedef CORE_ADDR (get_tls_dtp_offset_ftype) (struct gdbarch *gdbarch,
					      ptid_t ptid,
					      enum svr4_tls_libc libc);

/* Register architecture specific methods for fetching the TLS DTV
   and TLS DTP, used by linux_get_thread_local_address.  */

extern void svr4_tls_register_tls_methods
  (struct gdbarch_info info, struct gdbarch *gdbarch,
   get_tls_dtv_addr_ftype *get_tls_dtv_addr,
   get_tls_dtp_offset_ftype *get_tls_dtp_offset = nullptr);

/* Used as a gdbarch method for get_thread_local_address when the tdep
   file also defines a suitable  method for obtaining the TLS DTV.
   See linux_init_abi(), above.  */
CORE_ADDR
svr4_tls_get_thread_local_address (struct gdbarch *gdbarch, ptid_t ptid,
				   CORE_ADDR lm_addr, CORE_ADDR offset);

#endif /* GDB_SVR4_TLS_TDEP_H */
