#ifdef __cplusplus
#error "Do not use C++.  See the INSTALL file."
#endif

#ifndef MANDOC_CONFIG_H
#define MANDOC_CONFIG_H

#if defined(__linux__) || defined(__MINT__)
#define _GNU_SOURCE	/* See test-*.c what needs this. */
#endif

#include <sys/types.h>

#define MAN_CONF_FILE "/etc/man.conf"
#define MANPATH_DEFAULT "/usr/share/man:/usr/X11R6/man:/usr/local/man"
#define HAVE_DIRENT_NAMLEN 1
#define HAVE_ERR 1
#define HAVE_FTS 1
#define HAVE_GETLINE 1
#define HAVE_GETSUBOPT 0
#define HAVE_ISBLANK 1
#define HAVE_MKDTEMP 1
#define HAVE_MMAP 1
#define HAVE_PLEDGE 0
#define HAVE_PROGNAME 1
#define HAVE_REALLOCARRAY 0
#define HAVE_REWB_BSD 1
#define HAVE_REWB_SYSV 0
#define HAVE_SANDBOX_INIT 0
#define HAVE_STRCASESTR 1
#define HAVE_STRINGLIST 0
#define HAVE_STRLCAT 1
#define HAVE_STRLCPY 1
#define HAVE_STRPTIME 1
#define HAVE_STRSEP 1
#define HAVE_STRTONUM 0
#define HAVE_VASPRINTF 1
#define HAVE_WCHAR 1
#if 0
#define HAVE_SQLITE3 1
#define HAVE_SQLITE3_ERRSTR 1
#endif
#define HAVE_OHASH 0
#define HAVE_MANPATH 0

#define BINM_APROPOS "apropos"
#define BINM_MAKEWHATIS "makewhatis"
#define BINM_MAN "man"
#define BINM_SOELIM "soelim"
#define BINM_WHATIS "whatis"

extern	int	  getsubopt(char **, char * const *, char **);
extern	void	 *reallocarray(void *, size_t, size_t);
extern	long long strtonum(const char *, long long, long long, const char **);

#endif /* MANDOC_CONFIG_H */
