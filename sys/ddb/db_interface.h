/*	$NetBSD: db_interface.h,v 1.19.6.1 2007/07/11 20:04:55 mjf Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DDB_DB_INTERFACE_H_
#define _DDB_DB_INTERFACE_H_

/* arch/<arch>/<arch>/db_disasm.c */
db_addr_t	db_disasm(db_addr_t, bool);

/* arch/<arch>/<arch>/db_trace.c */
/* arch/vax/vax/db_machdep.c */
void		db_stack_trace_print(db_expr_t, bool, db_expr_t, const char *,
		    void (*)(const char *, ...));

/* ddb/db_xxx.c */
void		db_kgdb_cmd(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_proc.c */
void		db_kill_proc(db_expr_t, bool, db_expr_t, const char *);
void		db_show_all_procs(db_expr_t, bool, db_expr_t, const char *);
void		db_show_all_pools(db_expr_t, bool, db_expr_t, const char *);
void		db_show_sched_qs(db_expr_t, bool, db_expr_t, const char *);

/* kern/kern_clock.c */
void		db_show_callout(db_expr_t, bool, db_expr_t, const char *);

/* kern/subr_log.c */
void		db_dmesg(db_expr_t, bool, db_expr_t, const char *);

/* netinet/if_arp.c */
void		db_show_arptab(db_expr_t, bool, db_expr_t, const char *);

/* kern/vfs_aio.c */
void		db_show_aio_jobs(db_expr_t, bool, db_expr_t, const char *);

/*
 * This is used in several places to determine which printf format
 * string is appropriate for displaying a variable of type db_expr_t.
 */
#define	DB_EXPR_T_IS_QUAD (/* CONSTCOND */ sizeof(db_expr_t) > sizeof(long))

#endif /* _DDB_DB_INTERFACE_H_ */
