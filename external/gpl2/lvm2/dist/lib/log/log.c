/*	$NetBSD: log.c,v 1.1.1.2 2009/12/02 00:26:22 haad Exp $	*/

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

static int _syslog = 0;
static int _log_to_file = 0;
static int _log_direct = 0;
static int _log_while_suspended = 0;
static int _indent = 1;
static int _log_suppress = 0;
static char _msg_prefix[30] = "  ";
static int _already_logging = 0;

static lvm2_log_fn_t _lvm2_log_fn = NULL;

static int _lvm_errno = 0;
static int _store_errmsg = 0;
static char *_lvm_errmsg = NULL;

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

void init_msg_prefix(const char *prefix)
{
	strncpy(_msg_prefix, prefix, sizeof(_msg_prefix));
	_msg_prefix[sizeof(_msg_prefix) - 1] = '\0';
}

void init_indent(int indent)
{
	_indent = indent;
}

void reset_lvm_errno(int store_errmsg)
{
	_lvm_errno = 0;

	if (_lvm_errmsg) {
		dm_free(_lvm_errmsg);
		_lvm_errmsg = NULL;
	}

	_store_errmsg = store_errmsg;
}

int stored_errno(void)
{
	return _lvm_errno;
}

const char *stored_errmsg(void)
{
	return _lvm_errmsg ? : "";
}

void print_log(int level, const char *file, int line, int dm_errno,
	       const char *format, ...)
{
	va_list ap;
	char buf[1024], buf2[4096], locn[4096];
	int bufused, n;
	const char *message;
	const char *trformat;		/* Translated format string */
	char *newbuf;
	int use_stderr = level & _LOG_STDERR;

	level &= ~_LOG_STDERR;

	if (_log_suppress == 2)
		return;

	if (level <= _LOG_ERR)
		init_error_message_produced(1);

	trformat = _(format);

	if (dm_errno && !_lvm_errno)
		_lvm_errno = dm_errno;

	if (_lvm2_log_fn || (_store_errmsg && (level <= _LOG_ERR))) {
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
	}

	if (_store_errmsg && (level <= _LOG_ERR)) {
		if (!_lvm_errmsg)
			_lvm_errmsg = dm_strdup(message);
		else if ((newbuf = dm_realloc(_lvm_errmsg,
 					      strlen(_lvm_errmsg) +
					      strlen(message) + 2))) {
			_lvm_errmsg = strcat(newbuf, "\n");
			_lvm_errmsg = strcat(newbuf, message);
		}
	}

	if (_lvm2_log_fn) {
		_lvm2_log_fn(level, file, line, 0, message);

		return;
	}

      log_it:
	if (!_log_suppress) {
		if (verbose_level() > _LOG_DEBUG)
			dm_snprintf(locn, sizeof(locn), "#%s:%d ",
				     file, line);
		else
			locn[0] = '\0';

		va_start(ap, format);
		switch (level) {
		case _LOG_DEBUG:
			if (!strcmp("<backtrace>", format) &&
			    verbose_level() <= _LOG_DEBUG)
				break;
			if (verbose_level() >= _LOG_DEBUG) {
				fprintf(stderr, "%s%s%s", locn, log_command_name(),
					_msg_prefix);
				if (_indent)
					fprintf(stderr, "      ");
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;

		case _LOG_INFO:
			if (verbose_level() >= _LOG_INFO) {
				fprintf(stderr, "%s%s%s", locn, log_command_name(),
					_msg_prefix);
				if (_indent)
					fprintf(stderr, "    ");
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		case _LOG_NOTICE:
			if (verbose_level() >= _LOG_NOTICE) {
				fprintf(stderr, "%s%s%s", locn, log_command_name(),
					_msg_prefix);
				if (_indent)
					fprintf(stderr, "  ");
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		case _LOG_WARN:
			if (verbose_level() >= _LOG_WARN) {
				fprintf(use_stderr ? stderr : stdout, "%s%s",
					log_command_name(), _msg_prefix);
				vfprintf(use_stderr ? stderr : stdout, trformat, ap);
				fputc('\n', use_stderr ? stderr : stdout);
			}
			break;
		case _LOG_ERR:
			if (verbose_level() >= _LOG_ERR) {
				fprintf(stderr, "%s%s%s", locn, log_command_name(),
					_msg_prefix);
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		case _LOG_FATAL:
		default:
			if (verbose_level() >= _LOG_FATAL) {
				fprintf(stderr, "%s%s%s", locn, log_command_name(),
					_msg_prefix);
				vfprintf(stderr, trformat, ap);
				fputc('\n', stderr);
			}
			break;
		}
		va_end(ap);
	}

	if (level > debug_level())
		return;

	if (_log_to_file && (_log_while_suspended || !memlock())) {
		fprintf(_log_file, "%s:%d %s%s", file, line, log_command_name(),
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
				      "%s:%d %s%s", file, line, log_command_name(),
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
