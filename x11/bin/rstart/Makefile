#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:09 lukem Exp $

.include <bsd.own.mk>

PROG=			rstartd.real
SRCS=			auth.c server.c
BINDIR=			${X11LIBDIR}/rstart

CPPFLAGS.server.c=	-DSERVERNAME=\"rstartd\" \
			-DDEFAULT_CONFIG=\"${X11LIBDIR}/rstart/config\"

MAN=			rstart.1 rstartd.1

LNFILES=		client.cpp rstart.cpp \
			server.cpp rstartd.cpp

CPPSCRIPTS=		config rstart rstartd
CPPSCRIPTFLAGS_config=	-DBINDIR=${X11BINDIR} -DLIBDIR=${X11LIBDIR}/rstart \
			-DPACKAGEname=rstart -DENVPREFIX=RSTART
CPPSCRIPTFLAGS_rstart=	-DRSHCMD=rsh -DSERVERNAME=rstartd
CPPSCRIPTFLAGS_rstartd=	-DLIBDIR=${X11LIBDIR}/rstart -DSERVERNAME=rstartd
SCRIPTS=		rstart rstartd
SCRIPTSDIR=		${X11BINDIR}
FILES=			config
FILESDIR=		${X11LIBDIR}/rstart

SUBDIR=			commands contexts

.PATH:			${X11SRCDIR.xc}/programs/rstart

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
