#	$NetBSD: Makefile,v 1.33 2023/06/03 09:09:13 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

RUMPPRG=route
MAN=	route.8
SRCS=	route.c show.c keywords.c rtutil.c

.if (${MKRUMP} != "no")
CPPFLAGS+= -DRUMP_ACTION
LDADD.rump += -lrumpres
DPADD.rump += ${LIBRUMPRES}
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

LDADD+=-lutil
DPADD+=${LIBUTIL}

# The Makefile over in ../../distrib/utils/x_route
# would like keywords.[ch] to always exist here, so
# they are now checked in as sources.
#
# CPPFLAGS+=-I.
# CLEANFILES+= keywords.c keywords.h
# keywords.c keywords.h : keywords.sh
# 	${HOST_SH} keywords.sh

COPTS.rtutil.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
