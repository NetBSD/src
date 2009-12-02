/*	$NetBSD: lvm2cmd.h,v 1.1.1.2 2009/12/02 00:25:51 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_CMDLIB_H
#define _LVM_CMDLIB_H

#ifndef _LVM_LOG_H
typedef void (*lvm2_log_fn_t) (int level, const char *file, int line,
                               int dm_errno, const char *message);

#endif

#define LVM2_LOG_SUPPRESS	0

/* Logging levels */
#define LVM2_LOG_FATAL		2
#define LVM2_LOG_ERROR		3
#define LVM2_LOG_PRINT		4
#define LVM2_LOG_VERBOSE 	5
#define LVM2_LOG_VERY_VERBOSE	6
#define LVM2_LOG_DEBUG		7

/*
 * Define external function to replace the built-in logging function.
 * It receives output line-by-line.
 *
 * level is the logging level (see above)
 * file & line refer to the source code where the message originates.
 */
void lvm2_log_fn(lvm2_log_fn_t log_fn);

/*
 * Initialise library.
 * Returns a handle so repeated use of lvm2_run is more efficient.
 */ 
void *lvm2_init(void);

/*
 * Set log level (as above) if using built-in logging function. 
 * Default is LVM2_LOG_PRINT.  Use LVM2_LOG_SUPPRESS to suppress output.
 */
void lvm2_log_level(void *handle, int level);

/*
 * Run an LVM2 command. 
 * Use NULL handle if the call is a one-off and you don't want to bother 
 * calling lvm2_init/lvm2_exit.
 */
int lvm2_run(void *handle, const char *cmdline);

/* Release handle */
void lvm2_exit(void *handle);

#endif
