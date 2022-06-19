#	$NetBSD: Makefile,v 1.27 2022/06/19 11:31:19 simonb Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c ptyfs.c tmpfs.c misc.c
.if (${MKZFS} != "no")
SRCS+=	zfs.c
.endif
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif
.if (${MKZFS} != "no")
CPPFLAGS+=-DHAVE_ZFS
.endif
CPPFLAGS+=-D_KMEMUSER

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include "../../compat/exec.mk"

COPTS.fstat.c+=		${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
