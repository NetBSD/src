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
#include <syslog.h>

static FILE *_log_file;
static struct device _log_dev;
static struct str_list _log_dev_alias;

static int _verbose_level = VERBOSE_BASE_LEVEL;
static int _test = 0;
static int _partial = 0;
static int _md_filtering = 0;
static int _pvmove = 0;
static int _full_scan_done = 0;	/* Restrict to one full scan during each cmd */
static int _trust_cache = 0; /* Don't scan when incomplete VGs encountered */
static int _debug_level = 0;
static int _syslog = 0;
static int _log_to_file = 0;
static int _log_direct = 0;
static int _log_while_suspended = 0;
static int _indent = 1;
static int _log_cmd_name = 0;
static int _log_suppress = 0;
static int _ignorelockingfailure = 0;
static int _lockingfailed = 0;
static int _security_level = SECURITY_LEVEL;
static char _cmd_name[30] = "";
static char _msg_prefix[30] = "  ";
static int _already_logging = 0;
static int _mirror_in_sync = 0;
static int _dmeventd_monitor = DEFAULT_DMEVENTD_MONITOR;
static int _ignore_suspended_devices = 0;
static int _error_message_produced = 0;

static lvm2_log_fn_t _lvm2_log_fn = NULL;

void init_log_fn(lvm2_log_fn_t log_fn)
{
	if (log_fn)
		_lvm2_log_fn = log_fn;
	else
		_lvm2_log_fn = NULL;
}

void init_log_file(const char *log_file, int append)
{
	const char *open_mode = append ? "a" : "w";

	if (!(_log_file = fopen(log_file, open_mode))) {
		log_sys_error("fopen", log_file);
		return;
	}

	_log_to_file = 1;
}

void init_log_direct(const char *log_file, int append)
{
	int open_flags = append ? 0 : O_TRUNC;

	dev_create_file(log_file, &_log_dev, &_log_dev_alias, 1);
	if (!dev_open_flags(&_log_dev, O_RDWR | O_CREAT | open_flags, 1, 0))
		return;

	_log_direct = 1;
}

void init_log_while_suspended(int log_while_suspended)
{
	_log_while_suspended = log_while_suspended;
}

void init_syslog(int facility)
{
	openlog("lvm", LOG_PID, facility);
	_syslog = 1;
}

int log_suppress(int suppress)
{
	int old_suppress = _log_suppress;

	_log_suppress = suppress;

	return old_suppress;
}

void release_log_memory(void)
{
	if (!_log_direct)
		return;

	dm_free((char *) _log_dev_alias.str);
	_log_dev_alias.str = "activate_log file";
}

void fin_log(void)
{
	if (_log_direct) {
		dev_close(&_log_dev);
		_log_direct = 0;
	}

	if (_log_to_file) {
		if (dm_fclose(_log_file)) {
			if (errno)
			      fprintf(stderr, "failed to write log file: %s\n",
				      strerror(errno));
			else
			      fprintf(stderr, "failed to write log file\n");

		}
		_log_to_file = 0;
	}
}

void fin_syslog()
{
	if (_syslog)
		closelog();
	_syslog = 0;
}

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

void init_partial(int level)
{
	_partial = level;
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

void set_cmd_name(const char *cmd)
{
	strncpy(_cmd_name, cmd, sizeof(_cmd_name));
	_cmd_name[sizeof(_cmd_name) - 1] = '\0';
}

static const char *_command_name()
{
	if (!_log_cmd_name)
		return "";

	return _cmd_name;
}

void init_msg_prefix(const char *prefix)
{
	strncpy(_msg_prefix, prefix, sizeof(_msg_prefix));
	_msg_prefix[sizeof(_msg_prefix) - 1] = '\0';
}

void init_indent(int indent)
{
	_indent = indent;
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

int partial_mode()
{
	return _partial;
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

int debug_level()
{
	return _debug_level;
}

void print_log(int level, const char *file, int line, const char *format, ...)
{
	va_list ap;
	char buf[1024], buf2[4096], locn[4096];
	int bufused, n;
	const char *message;
	const char *trformat;		/* Translated format string */
	int use_stderr = level & _LOG_STDERR;

	level &= ~_LOG_STDERR;

	if (_log_suppress == 2)
		return;

	if (level <= _LOG_ERR)
		_error_message_produced = 1;

	trformat = _(format);

	if (_lvm2_log_fn) {
		va_start(ap, format);
		n = vsnprintf(buf2, sizeof(buf2) - 1, trformat, ap);
		va_end(ap);

		if (n < 0) {
			fprintf(stderr, _("vsnprintf failed: skipping external "
					"logging function"));
			goto log_it;
		}

		buf2[sizeof(buf2) - 1] = '\0';
		message = &buf2[0];

		_lvm2_log_fn(level, file, line, message);

		return;
	}

      log_it:
	if (!_log_suppress) {
		if (_verbose_level > _LOG_DEBUG)
			dm_snprintf(locn, sizeof(locn), "#%s:%d ",
				     file, line);
		else
			locn[0] = '\0';

		va_start(ap, format);
		switch (level) {
		case _LOG_DEBUG:
			if (!strcmp("<backtrace>", format) &&
			    _verbose_level <= _LOG_DEBUG)
				break;
			if (_verbose_level >= _LOG_DEBUG) {
				fprintf(stderr, "%s%s%s", locn, _command_name(),
					_msg_prefix);
				if (_indent)
					fprintf(stderr, "      ");
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;

		case _LOG_INFO:
			if (_verbose_level >= _LOG_INFO) {
				fprintf(stderr, "%s%s%s", locn, _command_name(),
					_msg_prefix);
				if (_indent)
					fprintf(stderr, "    ");
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		case _LOG_NOTICE:
			if (_verbose_level >= _LOG_NOTICE) {
				fprintf(stderr, "%s%s%s", locn, _command_name(),
					_msg_prefix);
				if (_indent)
					fprintf(stderr, "  ");
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		case _LOG_WARN:
			if (_verbose_level >= _LOG_WARN) {
				fprintf(use_stderr ? stderr : stdout, "%s%s",
					_command_name(), _msg_prefix);
				vfprintf(use_stderr ? stderr : stdout, trformat, ap);
				fputc('\n', use_stderr ? stderr : stdout);
			}
			break;
		case _LOG_ERR:
			if (_verbose_level >= _LOG_ERR) {
				fprintf(stderr, "%s%s%s", locn, _command_name(),
					_msg_prefix);
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		case _LOG_FATAL:
		default:
			if (_verbose_level >= _LOG_FATAL) {
				fprintf(stderr, "%s%s%s", locn, _command_name(),
					_msg_prefix);
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		}
		va_end(ap);
	}

	if (level > _debug_level)
		return;

	if (_log_to_file && (_log_while_suspended || !memlock())) {
		fprintf(_log_file, "%s:%d %s%s", file, line, _command_name(),
			_msg_prefix);

		va_start(ap, format);
		vfprintf(_log_file, trformat, ap);
		va_end(ap);

		fprintf(_log_file, "\n");
		fflush(_log_file);
	}

	if (_syslog && (_log_while_suspended || !memlock())) {
		va_start(ap, format);
		vsyslog(level, trformat, ap);
		va_end(ap);
	}

	/* FIXME This code is unfinished - pre-extend & condense. */
	if (!_already_logging && _log_direct && memlock()) {
		_already_logging = 1;
		memset(&buf, ' ', sizeof(buf));
		bufused = 0;
		if ((n = dm_snprintf(buf, sizeof(buf) - bufused - 1,
				      "%s:%d %s%s", file, line, _command_name(),
				      _msg_prefix)) == -1)
			goto done;

		bufused += n;

		va_start(ap, format);
		n = vsnprintf(buf + bufused - 1, sizeof(buf) - bufused - 1,
			      trformat, ap);
		va_end(ap);
		bufused += n;

	      done:
		buf[bufused - 1] = '\n';
		buf[bufused] = '\n';
		buf[sizeof(buf) - 1] = '\n';
		/* FIXME real size bufused */
		dev_append(&_log_dev, sizeof(buf), buf);
		_already_logging = 0;
	}
}
