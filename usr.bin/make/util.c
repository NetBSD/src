/*	$NetBSD: util.c,v 1.31 2002/03/12 20:15:15 christos Exp $	*/

/*
 * Missing stuff from OS's
 */

#ifdef MAKE_BOOTSTRAP
static char rcsid[] = "$NetBSD: util.c,v 1.31 2002/03/12 20:15:15 christos Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: util.c,v 1.31 2002/03/12 20:15:15 christos Exp $");
#endif
#endif

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "make.h"
#include <sys/param.h>

#ifndef __STDC__
# ifndef const
#  define const
# endif
#endif

#if defined(MAKE_BOOTSTRAP) && !defined(HAVE_STRERROR)
extern int errno, sys_nerr;
extern char *sys_errlist[];

char *
strerror(e)
    int e;
{
    static char buf[100];
    if (e < 0 || e >= sys_nerr) {
	snprintf(buf, sizeof(buf), "Unknown error %d", e);
	return buf;
    }
    else
	return sys_errlist[e];
}
#endif

#if defined(MAKE_BOOTSTRAP) && !defined(HAVE_STRDUP)
#include <string.h>

/* strdup
 *
 * Make a duplicate of a string.
 * For systems which lack this function.
 */
char *
strdup(str)
    const char *str;
{
    size_t len;
    char *p;

    if (str == NULL)
	return NULL;
    len = strlen(str) + 1;
    p = emalloc(len);

    return memcpy(p, str, len);
}
#endif

#if defined(MAKE_BOOTSTRAP) && !defined(HAVE_SETENV)
int
setenv(name, value, dum)
    const char *name;
    const char *value;
    int dum;
{
    register char *p;
    int len = strlen(name) + strlen(value) + 2; /* = \0 */
    char *ptr = (char*) emalloc(len);

    (void) dum;

    if (ptr == NULL)
	return -1;

    p = ptr;

    while (*name)
	*p++ = *name++;

    *p++ = '=';

    while (*value)
	*p++ = *value++;

    *p = '\0';

    len = putenv(ptr);
/*    free(ptr); */
    return len;
}
#endif

#if defined(__hpux__) || defined(__hpux)
/* strrcpy():
 *	Like strcpy, going backwards and returning the new pointer
 */
static char *
strrcpy(ptr, str)
    register char *ptr, *str;
{
    register int len = strlen(str);

    while (len)
	*--ptr = str[--len];

    return (ptr);
} /* end strrcpy */

char    *sys_siglist[] = {
        "Signal 0",
        "Hangup",                       /* SIGHUP    */
        "Interrupt",                    /* SIGINT    */
        "Quit",                         /* SIGQUIT   */
        "Illegal instruction",          /* SIGILL    */
        "Trace/BPT trap",               /* SIGTRAP   */
        "IOT trap",                     /* SIGIOT    */
        "EMT trap",                     /* SIGEMT    */
        "Floating point exception",     /* SIGFPE    */
        "Killed",                       /* SIGKILL   */
        "Bus error",                    /* SIGBUS    */
        "Segmentation fault",           /* SIGSEGV   */
        "Bad system call",              /* SIGSYS    */
        "Broken pipe",                  /* SIGPIPE   */
        "Alarm clock",                  /* SIGALRM   */
        "Terminated",                   /* SIGTERM   */
        "User defined signal 1",        /* SIGUSR1   */
        "User defined signal 2",        /* SIGUSR2   */
        "Child exited",                 /* SIGCLD    */
        "Power-fail restart",           /* SIGPWR    */
        "Virtual timer expired",        /* SIGVTALRM */
        "Profiling timer expired",      /* SIGPROF   */
        "I/O possible",                 /* SIGIO     */
        "Window size changes",          /* SIGWINDOW */
        "Stopped (signal)",             /* SIGSTOP   */
        "Stopped",                      /* SIGTSTP   */
        "Continued",                    /* SIGCONT   */
        "Stopped (tty input)",          /* SIGTTIN   */
        "Stopped (tty output)",         /* SIGTTOU   */
        "Urgent I/O condition",         /* SIGURG    */
        "Remote lock lost (NFS)",       /* SIGLOST   */
        "Signal 31",                    /* reserved  */
        "DIL signal"                    /* SIGDIL    */
};
#endif /* __hpux__ || __hpux */

#ifdef __hpux
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syscall.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int
killpg(pid, sig)
    int pid, sig;
{
    return kill(-pid, sig);
}

#ifndef __hpux__
void
srandom(seed)
    long seed;
{
    srand48(seed);
}

long
random()
{
    return lrand48();
}
#endif

/* turn into bsd signals */
void (*
signal(s, a)) __P((int))
    int     s;
    void (*a) __P((int));
{
    struct sigvec osv, sv;

    (void) sigvector(s, (struct sigvec *) 0, &osv);
    sv = osv;
    sv.sv_handler = a;
#ifdef SV_BSDSIG
    sv.sv_flags = SV_BSDSIG;
#endif

    if (sigvector(s, &sv, (struct sigvec *) 0) == -1)
        return (BADSIG);
    return (osv.sv_handler);
}

#ifndef __hpux__
int
utimes(file, tvp)
    char *file;
    struct timeval tvp[2];
{
    struct utimbuf t;

    t.actime  = tvp[0].tv_sec;
    t.modtime = tvp[1].tv_sec;
    return(utime(file, &t));
}
#endif

#if !defined(BSD) && !defined(d_fileno)
# define d_fileno d_ino
#endif

#ifndef DEV_DEV_COMPARE
# define DEV_DEV_COMPARE(a, b) ((a) == (b))
#endif
#define ISDOT(c) ((c)[0] == '.' && (((c)[1] == '\0') || ((c)[1] == '/')))
#define ISDOTDOT(c) ((c)[0] == '.' && ISDOT(&((c)[1])))

char   *
getwd(pathname)
    char   *pathname;
{
    DIR    *dp;
    struct dirent *d;
    extern int errno;

    struct stat st_root, st_cur, st_next, st_dotdot;
    char    pathbuf[MAXPATHLEN], nextpathbuf[MAXPATHLEN * 2];
    char   *pathptr, *nextpathptr, *cur_name_add;

    /* find the inode of root */
    if (stat("/", &st_root) == -1) {
	(void) snprintf(pathname, sizeof(pathname),
			"getwd: Cannot stat \"/\" (%s)", strerror(errno));
	return (NULL);
    }
    pathbuf[MAXPATHLEN - 1] = '\0';
    pathptr = &pathbuf[MAXPATHLEN - 1];
    nextpathbuf[MAXPATHLEN - 1] = '\0';
    cur_name_add = nextpathptr = &nextpathbuf[MAXPATHLEN - 1];

    /* find the inode of the current directory */
    if (lstat(".", &st_cur) == -1) {
	(void) snprintf(pathname, sizeof(pathname),
			"getwd: Cannot stat \".\" (%s)", strerror(errno));
	return (NULL);
    }
    nextpathptr = strrcpy(nextpathptr, "../");

    /* Descend to root */
    for (;;) {

	/* look if we found root yet */
	if (st_cur.st_ino == st_root.st_ino &&
	    DEV_DEV_COMPARE(st_cur.st_dev, st_root.st_dev)) {
	    (void) strcpy(pathname, *pathptr != '/' ? "/" : pathptr);
	    return (pathname);
	}

	/* open the parent directory */
	if (stat(nextpathptr, &st_dotdot) == -1) {
	    (void) snprintf(pathname, sizeof(pathname),
			    "getwd: Cannot stat directory \"%s\" (%s)",
			    nextpathptr, strerror(errno));
	    return (NULL);
	}
	if ((dp = opendir(nextpathptr)) == NULL) {
	    (void) snprintf(pathname, sizeof(pathname),
			    "getwd: Cannot open directory \"%s\" (%s)",
			    nextpathptr, strerror(errno));
	    return (NULL);
	}

	/* look in the parent for the entry with the same inode */
	if (DEV_DEV_COMPARE(st_dotdot.st_dev, st_cur.st_dev)) {
	    /* Parent has same device. No need to stat every member */
	    for (d = readdir(dp); d != NULL; d = readdir(dp))
		if (d->d_fileno == st_cur.st_ino)
		    break;
	}
	else {
	    /*
	     * Parent has a different device. This is a mount point so we
	     * need to stat every member
	     */
	    for (d = readdir(dp); d != NULL; d = readdir(dp)) {
		if (ISDOT(d->d_name) || ISDOTDOT(d->d_name))
		    continue;
		(void) strcpy(cur_name_add, d->d_name);
		if (lstat(nextpathptr, &st_next) == -1) {
		    (void) snprintf(pathname, sizeof(pathname),
			"getwd: Cannot stat \"%s\" (%s)",
			d->d_name, strerror(errno));
		    (void) closedir(dp);
		    return (NULL);
		}
		/* check if we found it yet */
		if (st_next.st_ino == st_cur.st_ino &&
		    DEV_DEV_COMPARE(st_next.st_dev, st_cur.st_dev))
		    break;
	    }
	}
	if (d == NULL) {
	    (void) snprintf(pathname, sizeof(pathname), 
		"getwd: Cannot find \".\" in \"..\"");
	    (void) closedir(dp);
	    return (NULL);
	}
	st_cur = st_dotdot;
	pathptr = strrcpy(pathptr, d->d_name);
	pathptr = strrcpy(pathptr, "/");
	nextpathptr = strrcpy(nextpathptr, "../");
	(void) closedir(dp);
	*cur_name_add = '\0';
    }
} /* end getwd */
#endif /* __hpux */

#if defined(sun) && defined(__svr4__)
#include <signal.h>

/* turn into bsd signals */
void (*
signal(s, a)) __P((int))
    int     s;
    void (*a) __P((int));
{
    struct sigaction sa, osa;

    sa.sa_handler = a;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(s, &sa, &osa) == -1)
	return SIG_ERR;
    else
	return osa.sa_handler;
}
#endif

#if defined(MAKE_BOOTSTRAP) && !defined(HAVE_VSNPRINTF)
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#if !defined(__osf__)
#ifdef _IOSTRG
#define STRFLAG	(_IOSTRG|_IOWRT)	/* no _IOWRT: avoid stdio bug */
#else
#if 0
#define STRFLAG	(_IOREAD)		/* XXX: Assume svr4 stdio */
#endif
#endif /* _IOSTRG */
#endif /* __osf__ */

int
vsnprintf(s, n, fmt, args)
	char *s;
	size_t n;
	const char *fmt;
	va_list args;
{
#ifdef STRFLAG
	FILE fakebuf;

	fakebuf._flag = STRFLAG;
	/*
	 * Some os's are char * _ptr, others are unsigned char *_ptr...
	 * We cast to void * to make everyone happy.
	 */
	fakebuf._ptr = (void *) s;
	fakebuf._cnt = n-1;
	fakebuf._file = -1;
	_doprnt(fmt, args, &fakebuf);
	fakebuf._cnt++;
	putc('\0', &fakebuf);
	if (fakebuf._cnt<0)
	    fakebuf._cnt = 0;
	return (n-fakebuf._cnt-1);
#else
	(void) vsprintf(s, fmt, args);
	return strlen(s);
#endif
}

int
#ifdef __STDC__
snprintf(char *s, size_t n, const char *fmt, ...)
#else
snprintf(va_alist)
	va_dcl
#endif
{
	va_list ap;
	int rv;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *s;
	size_t n;
	const char *fmt;

	va_start(ap);

	s = va_arg(ap, char *);
	n = va_arg(ap, size_t);
	fmt = va_arg(ap, const char *);
#endif
	rv = vsnprintf(s, n, fmt, ap);
	va_end(ap);
	return rv;
}

#if defined(MAKE_BOOTSTRAP) && !defined(HAVE_STRFTIME)
size_t
strftime(buf, len, fmt, tm)
	char *buf;
	size_t len;
	const char *fmt;
	const struct tm *tm;
{
	static char months[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	size_t s;
	char *b = buf;

	while (*fmt) {
		if (len == 0)
			return buf - b;
		if (*fmt != '%') {
			*buf++ = *fmt++;
			len--;
			continue;
		}
		switch (*fmt++) {
		case '%':
			*buf++ = '%';
			len--;
			if (len == 0) return buf - b;
			/*FALLTHROUGH*/
		case '\0':
			*buf = '%';
			s = 1;
			break;
		case 'k':
			s = snprintf(buf, len, "%d", tm->tm_hour);
			break;
		case 'M':
			s = snprintf(buf, len, "%02d", tm->tm_min);
			break;
		case 'S':
			s = snprintf(buf, len, "%02d", tm->tm_sec);
			break;
		case 'b':
			if (tm->tm_mon >= 12)
				return buf - b;
			s = snprintf(buf, len, "%s", months[tm->tm_mon]);
			break;
		case 'd':
			s = snprintf(buf, len, "%s", tm->tm_mday);
			break;
		case 'Y':
			s = snprintf(buf, len, "%s", 1900 + tm->tm_year);
			break;
		default:
			s = snprintf(buf, len, "Unsupported format %c",
			    fmt[-1]);
			break;
		}
		buf += s;
		len -= s;
	}
}
#endif
#endif
