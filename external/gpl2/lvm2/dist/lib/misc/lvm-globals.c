/*	$NetBSD: lvm-globals.c,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

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

#include "lib.h"
#include "device.h"
#include "memlock.h"
#include "lvm-string.h"
#include "lvm-file.h"
#include "defaults.h"

#include <stdarg.h>

static int _verbose_level = VERBOSE_BASE_LEVEL;
static int _test = 0;
static int _md_filtering = 0;
static int _pvmove = 0;
static int _full_scan_done = 0;	/* Restrict to one full scan during each cmd */
static int _trust_cache = 0; /* Don't scan when incomplete VGs encountered */
static int _debug_level = 0;
static int _log_cmd_name = 0;
static int _ignorelockingfailure = 0;
static int _lockingfailed = 0;
static int _security_level = SECURITY_LEVEL;
static char _cmd_name[30] = "";
static int _mirror_in_sync = 0;
static int _dmeventd_monitor = DEFAULT_DMEVENTD_MONITOR;
static int _ignore_suspended_devices = 0;
static int _error_message_produced = 0;
static unsigned _is_static = 0;

void init_verbose(int level)
{
	_verbose_level = level;
}

void init_test(int level)
{
	if (!_test && level)
		log_print("Test mode: Metadata will NOT be updated.");
	_test = level;
}

void init_md_filtering(int level)
{
	_md_filtering = level;
}

void init_pvmove(int level)
{
	_pvmove = level;
}

void init_full_scan_done(int level)
{
	_full_scan_done = level;
}

void init_trust_cache(int trustcache)
{
	_trust_cache = trustcache;
}

void init_ignorelockingfailure(int level)
{
	_ignorelockingfailure = level;
}

void init_lockingfailed(int level)
{
	_lockingfailed = level;
}

void init_security_level(int level)
{
	_security_level = level;
}

void init_mirror_in_sync(int in_sync)
{
	_mirror_in_sync = in_sync;
}

void init_dmeventd_monitor(int reg)
{
	_dmeventd_monitor = reg;
}

void init_ignore_suspended_devices(int ignore)
{
	_ignore_suspended_devices = ignore;
}

void init_cmd_name(int status)
{
	_log_cmd_name = status;
}

void init_is_static(unsigned value)
{
	_is_static = value;
}

void set_cmd_name(const char *cmd)
{
	strncpy(_cmd_name, cmd, sizeof(_cmd_name));
	_cmd_name[sizeof(_cmd_name) - 1] = '\0';
}

const char *log_command_name()
{
	if (!_log_cmd_name)
		return "";

	return _cmd_name;
}

void init_error_message_produced(int value)
{
	_error_message_produced = value;
}

int error_message_produced(void)
{
	return _error_message_produced;
}

int test_mode()
{
	return _test;
}

int md_filtering()
{
	return _md_filtering;
}

int pvmove_mode()
{
	return _pvmove;
}

int full_scan_done()
{
	return _full_scan_done;
}

int trust_cache()
{
	return _trust_cache;
}

int lockingfailed()
{
	return _lockingfailed;
}

int ignorelockingfailure()
{
	return _ignorelockingfailure;
}

int security_level()
{
	return _security_level;
}

int mirror_in_sync(void)
{
	return _mirror_in_sync;
}

int dmeventd_monitor_mode(void)
{
	return _dmeventd_monitor;
}

int ignore_suspended_devices(void)
{
	return _ignore_suspended_devices;
}

void init_debug(int level)
{
	_debug_level = level;
}

int verbose_level()
{
	return _verbose_level;
}

int debug_level()
{
	return _debug_level;
}

unsigned is_static(void)
{
	return _is_static;
}
