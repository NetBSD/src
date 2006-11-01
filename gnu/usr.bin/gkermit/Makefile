#	$NetBSD: Makefile,v 1.1 2006/11/01 13:17:38 apb Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist/gkermit

PROG=		gkermit
SRCS=		gcmdline.c gkermit.c gproto.c gunixio.c
MAN=		gkermit.1

# Note that we do not use gwart.  Provided gproto.w is unchanged,
# the distributed gproto.c does not need to be updated.

BUILDSYMLINKS+=	gkermit.nr gkermit.1

CPPFLAGS+=	-DPOSIX
CPPFLAGS+=	-I${DIST}

.PATH: ${DIST}

.include <bsd.prog.mk>
