/*	$NetBSD: lvm-globals.h,v 1.1.1.1.2.1 2009/05/13 18:52:41 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_GLOBALS_H
#define _LVM_GLOBALS_H

#define VERBOSE_BASE_LEVEL _LOG_WARN
#define SECURITY_LEVEL 0

void init_verbose(int level);
void init_test(int level);
void init_md_filtering(int level);
void init_pvmove(int level);
void init_full_scan_done(int level);
void init_trust_cache(int trustcache);
void init_debug(int level);
void init_cmd_name(int status);
void init_ignorelockingfailure(int level);
void init_lockingfailed(int level);
void init_security_level(int level);
void init_mirror_in_sync(int in_sync);
void init_dmeventd_monitor(int reg);
void init_ignore_suspended_devices(int ignore);
void init_error_message_produced(int produced);
void init_is_static(unsigned value);

void set_cmd_name(const char *cmd_name);

int test_mode(void);
int md_filtering(void);
int pvmove_mode(void);
int full_scan_done(void);
int trust_cache(void);
int verbose_level(void);
int debug_level(void);
int ignorelockingfailure(void);
int lockingfailed(void);
int security_level(void);
int mirror_in_sync(void);
int ignore_suspended_devices(void);
const char *log_command_name(void);
unsigned is_static(void);

#define DMEVENTD_MONITOR_IGNORE -1
int dmeventd_monitor_mode(void);

#endif
