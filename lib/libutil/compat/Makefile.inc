#	$NetBSD: Makefile.inc,v 1.1.2.1 2008/12/28 01:14:32 christos Exp $

.PATH: ${.CURDIR}/compat

CPPFLAGS+=-I${.CURDIR}/../libc -I${.CURDIR}/../../sys
SRCS+=compat_passwd.c compat_loginx.c compat_login.c compat_parsedate.c \
    compat_login_cap.c
