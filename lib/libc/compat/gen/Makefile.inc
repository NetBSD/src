#	$NetBSD: Makefile.inc,v 1.1 2005/09/13 01:44:09 christos Exp $

.PATH: ${.CURDIR}/compat/gen
SRCS+=compat_errlist.c compat_fts.c compat___fts13.c compat_getmntinfo.c \
    compat_glob.c compat_opendir.c compat_readdir.c compat_scandir.c \
    compat_siglist.c compat_signame.c compat_sigsetops.c compat_times.c \
    compat_timezone.c compat_unvis.c compat_utmpx.c compat__sys_errlist.c \
    compat__sys_nerr.c compat__sys_siglist.c 
