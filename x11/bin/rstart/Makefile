#	$NetBSD: Makefile,v 1.4 2004/01/02 16:32:12 lukem Exp $

.include <bsd.own.mk>

PROG=			rstartd.real
SRCS=			auth.c server.c
RSTARTDBINDIR=		${X11ROOTDIR}/libexec

CPPFLAGS.server.c=	-DSERVERNAME=\"rstartd\" \
			-DDEFAULT_CONFIG=\"${X11LIBDIR}/rstart/config\"

MAN=			rstart.1 rstartd.1

BUILDSYMLINKS=		client.cpp rstart.cpp

CPPSCRIPTS=		config rstart rstartd
CPPSCRIPTFLAGS_config=	-DBINDIR=${X11BINDIR} -DLIBDIR=${X11LIBDIR}/rstart \
			-DPACKAGEname=rstart -DENVPREFIX=RSTART
CPPSCRIPTFLAGS_rstart=	-DRSHCMD=rsh -DSERVERNAME=rstartd
CPPSCRIPTFLAGS_rstartd=	-DLIBDIR=${X11LIBDIR}/rstart -DSERVERNAME=rstartd \
			-DBINDIR=${RSTARTDBINDIR}
SCRIPTS=		rstart rstartd
SCRIPTSDIR=		${X11BINDIR}
FILES=			config
FILESDIR=		${X11LIBDIR}/rstart

SUBDIR=			commands contexts

.PATH:			${X11SRCDIR.xc}/programs/rstart

.include <bsd.x11.mk>
BINDIR=			${RSTARTDBINDIR}
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
