#	$NetBSD: Makefile.inc,v 1.12 2009/08/16 19:33:39 christos Exp $

.PATH: ${COMPATDIR}/gen
SRCS+=compat_errlist.c compat_fts.c compat___fts13.c compat___fts30.c \
    compat___fts31.c compat_getmntinfo.c compat_glob.c compat___glob13.c \
    compat_opendir.c compat_readdir.c compat__readdir_unlocked30.c \
    compat_scandir.c compat_siglist.c compat_signame.c compat_sigsetops.c \
    compat_times.c compat_timezone.c compat_unvis.c compat_utmpx.c \
    compat__sys_errlist.c compat__sys_nerr.c compat__sys_siglist.c \
    compat_time.c compat_utime.c compat_devname.c compat_alphasort.c \
    compat_getpwent.c compat___fts32.c compat_utmp.c compat___fts50.c

LIBMINC=-I${LIBCDIR}/../libm/src -DUSE_LIBM
CPPFLAGS.compat_frexp_ieee754.c += ${LIBMINC}
CPPFLAGS.compat_ldexp_ieee754.c += ${LIBMINC}
CPPFLAGS.compat_modf_ieee754.c += ${LIBMINC}

