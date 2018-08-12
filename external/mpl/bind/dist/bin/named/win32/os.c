/*	$NetBSD: os.c,v 1.1.1.1 2018/08/12 12:07:44 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <config.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <io.h>
#include <process.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <isc/print.h>
#include <isc/result.h>
#include <isc/strerror.h>
#include <isc/string.h>
#include <isc/ntpaths.h>
#include <isc/util.h>
#include <isc/win32os.h>

#include <named/main.h>
#include <named/log.h>
#include <named/os.h>
#include <named/globals.h>
#include <named/ntservice.h>


static char *lockfile = NULL;
static char *pidfile = NULL;
static int devnullfd = -1;
static int lockfilefd = -1;

static BOOL Initialized = FALSE;

static char *version_error =
	"named requires Windows 2000 Service Pack 2 or later to run correctly";

void
named_paths_init(void) {
	if (!Initialized)
		isc_ntpaths_init();

	named_g_conffile = isc_ntpaths_get(NAMED_CONF_PATH);
	named_g_defaultpidfile = isc_ntpaths_get(NAMED_PID_PATH);
	named_g_defaultlockfile = isc_ntpaths_get(NAMED_LOCK_PATH);
	named_g_keyfile = isc_ntpaths_get(RNDC_KEY_PATH);
	named_g_defaultsessionkeyfile = isc_ntpaths_get(SESSION_KEY_PATH);
	named_g_defaultdnstap = NULL;

	Initialized = TRUE;
}

/*
 * Due to Knowledge base article Q263823 we need to make sure that
 * Windows 2000 systems have Service Pack 2 or later installed and
 * warn when it isn't.
 */
static void
version_check(const char *progname) {

	if ((isc_win32os_versioncheck(4, 0, 0, 0) >= 0) &&
	    (isc_win32os_versioncheck(5, 0, 0, 0) < 0))
		return;	/* No problem with Version 4.0 */
	if (isc_win32os_versioncheck(5, 0, 2, 0) < 0)
		if (ntservice_isservice())
			NTReportError(progname, version_error);
		else
			fprintf(stderr, "%s\n", version_error);
}

static void
setup_syslog(const char *progname) {
	int options;

	options = LOG_PID;
#ifdef LOG_NDELAY
	options |= LOG_NDELAY;
#endif

	openlog(progname, options, LOG_DAEMON);
}

void
named_os_init(const char *progname) {
	named_paths_init();
	setup_syslog(progname);
	/*
	 * XXXMPA. We may need to split ntservice_init() in two and
	 * just mark as running in named_os_started().  If we do that
	 * this is where the first part of ntservice_init() should be
	 * called from.
	 *
	 * XXX970 Remove comment if no problems by 9.7.0.
	 *
	 * ntservice_init();
	 */
	version_check(progname);
}

void
named_os_daemonize(void) {
	/*
	 * Try to set stdin, stdout, and stderr to /dev/null, but press
	 * on even if it fails.
	 */
	if (devnullfd != -1) {
		if (devnullfd != _fileno(stdin)) {
			close(_fileno(stdin));
			(void)_dup2(devnullfd, _fileno(stdin));
		}
		if (devnullfd != _fileno(stdout)) {
			close(_fileno(stdout));
			(void)_dup2(devnullfd, _fileno(stdout));
		}
		if (devnullfd != _fileno(stderr)) {
			close(_fileno(stderr));
			(void)_dup2(devnullfd, _fileno(stderr));
		}
	}
}

void
named_os_opendevnull(void) {
	devnullfd = open("NUL", O_RDWR, 0);
}

void
named_os_closedevnull(void) {
	if (devnullfd != _fileno(stdin) &&
	    devnullfd != _fileno(stdout) &&
	    devnullfd != _fileno(stderr)) {
		close(devnullfd);
		devnullfd = -1;
	}
}

void
named_os_chroot(const char *root) {
	if (root != NULL)
		named_main_earlyfatal("chroot(): isn't supported by Win32 API");
}

void
named_os_inituserinfo(const char *username) {
}

void
named_os_changeuser(void) {
}

unsigned int
ns_os_uid(void) {
	return (0);
}

void
named_os_adjustnofile(void) {
}

void
named_os_minprivs(void) {
}

static int
safe_open(const char *filename, int mode, isc_boolean_t append) {
	int fd;
	struct stat sb;

	if (stat(filename, &sb) == -1) {
		if (errno != ENOENT)
			return (-1);
	} else if ((sb.st_mode & S_IFREG) == 0)
		return (-1);

	if (append)
		fd = open(filename, O_WRONLY|O_CREAT|O_APPEND, mode);
	else {
		(void)unlink(filename);
		fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, mode);
	}
	return (fd);
}

static void
cleanup_pidfile(void) {
	if (pidfile != NULL) {
		(void)unlink(pidfile);
		free(pidfile);
	}
	pidfile = NULL;
}

static void
cleanup_lockfile(void) {
	if (lockfilefd != -1) {
		close(lockfilefd);
		lockfilefd = -1;
	}

	if (lockfile != NULL) {
		int n = unlink(lockfile);
		if (n == -1 && errno != ENOENT)
			named_main_earlywarning("unlink '%s': failed",
						lockfile);
		free(lockfile);
		lockfile = NULL;
	}
}

FILE *
named_os_openfile(const char *filename, int mode, isc_boolean_t switch_user) {
	char strbuf[ISC_STRERRORSIZE];
	FILE *fp;
	int fd;

	UNUSED(switch_user);
	fd = safe_open(filename, mode, ISC_FALSE);
	if (fd < 0) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		named_main_earlywarning("could not open file '%s': %s",
					filename, strbuf);
		return (NULL);
	}

	fp = fdopen(fd, "w");
	if (fp == NULL) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		named_main_earlywarning("could not fdopen() file '%s': %s",
					filename, strbuf);
		close(fd);
	}

	return (fp);
}

void
named_os_writepidfile(const char *filename, isc_boolean_t first_time) {
	FILE *pidlockfile;
	pid_t pid;
	char strbuf[ISC_STRERRORSIZE];
	void (*report)(const char *, ...);

	/*
	 * The caller must ensure any required synchronization.
	 */

	report = first_time ? named_main_earlyfatal : named_main_earlywarning;

	cleanup_pidfile();

	if (filename == NULL)
		return;

	pidfile = strdup(filename);
	if (pidfile == NULL) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		(*report)("couldn't strdup() '%s': %s", filename, strbuf);
		return;
	}

	pidlockfile = named_os_openfile(filename,
					S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
					ISC_FALSE);
	if (pidlockfile == NULL) {
		free(pidfile);
		pidfile = NULL;
		return;
	}

	pid = getpid();

	if (fprintf(pidlockfile, "%ld\n", (long)pid) < 0) {
		(*report)("fprintf() to pid file '%s' failed", filename);
		(void)fclose(pidlockfile);
		cleanup_pidfile();
		return;
	}
	if (fflush(pidlockfile) == EOF) {
		(*report)("fflush() to pid file '%s' failed", filename);
		(void)fclose(pidlockfile);
		cleanup_pidfile();
		return;
	}
	(void)fclose(pidlockfile);
}

isc_boolean_t
named_os_issingleton(const char *filename) {
	char strbuf[ISC_STRERRORSIZE];
	OVERLAPPED o;

	if (lockfilefd != -1)
		return (ISC_TRUE);

	if (strcasecmp(filename, "none") == 0)
		return (ISC_TRUE);

	lockfile = strdup(filename);
	if (lockfile == NULL) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		named_main_earlyfatal("couldn't allocate memory for '%s': %s",
				      filename, strbuf);
	}

	/*
	 * named_os_openfile() uses safeopen() which removes any existing
	 * files. We can't use that here.
	 */
	lockfilefd = open(filename, O_WRONLY | O_CREAT,
			  S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (lockfilefd == -1) {
		cleanup_lockfile();
		return (ISC_FALSE);
	}

	memset(&o, 0, sizeof(o));
	/* Expect ERROR_LOCK_VIOLATION if already locked */
	if (!LockFileEx((HANDLE) _get_osfhandle(lockfilefd),
			LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
			0, 0, 1, &o)) {
		cleanup_lockfile();
		return (ISC_FALSE);
	}

	return (ISC_TRUE);
}


void
named_os_shutdown(void) {
	closelog();
	cleanup_pidfile();

	if (lockfilefd != -1) {
		(void) UnlockFile((HANDLE) _get_osfhandle(lockfilefd),
				  0, 0, 0, 1);
		close(lockfilefd);
		lockfilefd = -1;
	}
	ntservice_shutdown();	/* This MUST be the last thing done */
}

isc_result_t
named_os_gethostname(char *buf, size_t len) {
	int n;

	n = gethostname(buf, (int)len);
	return ((n == 0) ? ISC_R_SUCCESS : ISC_R_FAILURE);
}

void
named_os_shutdownmsg(char *command, isc_buffer_t *text) {
	UNUSED(command);
	UNUSED(text);
}

void
named_os_tzset(void) {
#ifdef HAVE_TZSET
	tzset();
#endif
}

void
named_os_started(void) {
	ntservice_init();
}

static char unamebuf[BUFSIZ];
static char *unamep = NULL;

static void
getuname(void) {
	DWORD fvilen;
	char *fvi;
	VS_FIXEDFILEINFO *ffi;
	UINT ffilen;
	SYSTEM_INFO sysinfo;
	char *arch;

	fvi = NULL;
	fvilen = GetFileVersionInfoSize("kernel32.dll", 0);
	if (fvilen == 0) {
		goto err;
	}
	fvi = (char *)malloc(fvilen);
	if (fvi == NULL) {
		goto err;
	}
	memset(fvi, 0, fvilen);
	if (GetFileVersionInfo("kernel32.dll", 0, fvilen, fvi) == 0) {
		goto err;
	}
	ffi = NULL;
	ffilen = 0;
	if ((VerQueryValue(fvi, "\\", &ffi, &ffilen) == 0) ||
	    (ffi == NULL) || (ffilen == 0)) {
		goto err;
	}
	memset(&sysinfo, 0, sizeof(sysinfo));
	GetSystemInfo(&sysinfo);
	switch (sysinfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
		arch = "x86";
		break;
	case PROCESSOR_ARCHITECTURE_ARM:
		arch = "arm";
		break;
	case PROCESSOR_ARCHITECTURE_IA64:
		arch = "ia64";
		break;
	case PROCESSOR_ARCHITECTURE_AMD64:
		arch = "x64";
		break;
	default:
		arch = "unknown architecture";
		break;
	}

	snprintf(unamebuf, sizeof(unamebuf),
		 "Windows %d %d build %d %d for %s\n",
		 (ffi->dwProductVersionMS >> 16) & 0xffff,
		 ffi->dwProductVersionMS & 0xffff,
		 (ffi->dwProductVersionLS >> 16) & 0xffff,
		 ffi->dwProductVersionLS & 0xffff,
		 arch);

    err:
	if (fvi != NULL) {
		free(fvi);
	}
	unamep = unamebuf;
}

/*
 * GetVersionEx() returns 6.2 (aka Windows 8.1) since it was obsoleted
 * so we had to switch to the recommended way to get the Windows version.
 */
char *
named_os_uname(void) {
	if (unamep == NULL)
		getuname();
	return (unamep);
}
