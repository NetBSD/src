#	$NetBSD: Makefile,v 1.6 1999/07/19 12:04:03 tron Exp $

.PATH:		${.CURDIR}/../man

WARNS=		0
PROG=		named
HDRS=		db_defs.h db_glob.h ns_defs.h ns_glob.h named.h pathnames.h
SRCS=		db_dump.c db_load.c db_lookup.c db_save.c db_update.c \
		db_glue.c ns_parser.y ns_lexer.c ns_parseutil.c \
		ns_forw.c ns_init.c ns_main.c ns_maint.c ns_req.c \
		ns_resp.c ns_stats.c ns_ncache.c ns_xfr.c ns_glue.c \
		ns_udp.c ns_config.c ns_update.c tmp_version.c
CPPFLAGS+=	${INCLUDE}
LDADD=		${LIBRARY}
CLEANFILES+=	y.tab.h named-bootconf tmp_version.c
MAN=		${PROG}.8 named.conf.5 named-bootconf.8
SCRIPTS=	named-bootconf.sh

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		named.conf
FILESDIR=	/usr/share/examples/named
.endif

HOSTNAMECMD=	hostname

tmp_version.c: ${.CURDIR}/version.c Makefile ../Makefile
	(u=$${USER-root} d=`pwd` h=`${HOSTNAMECMD}` t=`date`; \
	sed -e "s|%WHEN%|$${t}|" -e "s|%VERSION%|"${VER}"|" \
	    -e "s|%WHOANDWHERE%|$${u}@$${h}:$${d}|" \
	    < ${.CURDIR}/version.c > tmp_version.c)

.include "../../Makefile.inc"
.include <bsd.prog.mk>
