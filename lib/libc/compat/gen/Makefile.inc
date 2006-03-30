#	$NetBSD: Makefile.inc,v 1.4 2006/03/30 01:23:50 christos Exp $

.PATH: ${COMPATDIR}/gen
SRCS+=compat_errlist.c compat_fts.c compat___fts13.c compat___fts30.c \
    compat_getmntinfo.c compat_glob.c compat___glob13.c compat_opendir.c \
    compat_readdir.c compat_scandir.c compat_siglist.c compat_signame.c \
    compat_sigsetops.c compat_times.c compat_timezone.c compat_unvis.c \
    compat_utmpx.c compat__sys_errlist.c compat__sys_nerr.c \
    compat__sys_siglist.c
