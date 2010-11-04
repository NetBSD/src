#	$NetBSD: Makefile,v 1.24 2010/11/04 23:36:10 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	route
MAN=	route.8
SRCS=	route.c show.c keywords.c

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

# The Makefile over in ../../distrib/utils/x_route
# would like keywords.[ch] to always exist here, so
# they are now checked in as sources.
#
# CPPFLAGS+=-I.
# CLEANFILES+= keywords.c keywords.h
# keywords.c keywords.h : keywords.sh
# 	${HOST_SH} keywords.sh

#
# Compile-time debug flag.  If compiled with "make RUMP_ACTION=1",
# make rump system calls.
#
.ifdef RUMP_ACTION
CPPFLAGS+=      -DRUMP_SYS_NETWORKING -DRUMP_SYS_READWRITE -DRUMP_SYS_CLOSE
CPPFLAGS+=      -DRUMP_ACTION -DSMALL -Dsysctl=rump_sys___sysctl
LDADD+=         -lrumpclient
DBG=            -g
.endif

.include <bsd.prog.mk>
