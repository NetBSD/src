#	$NetBSD: Makefile,v 1.12 2003/08/01 17:04:07 lukem Exp $

.include "../Makefile.inc"
.PATH:		${BIND_DIST_DIR}/bin/named \
		${BIND_DIST_DIR}/bin/named-bootconf \
		${BIND_DIST_DIR}/man

PROG=		named
HDRS=		db_defs.h db_glob.h ns_defs.h ns_glob.h named.h pathnames.h
SRCS=		db_dump.c db_load.c db_lookup.c db_save.c db_update.c \
		db_glue.c db_ixfr.c db_sec.c db_tsig.c \
		ns_parser.y ns_lexer.c ns_parseutil.c ns_ctl.c \
		ns_forw.c ns_init.c ns_main.c ns_maint.c ns_req.c \
		ns_resp.c ns_stats.c ns_ncache.c ns_xfr.c ns_glue.c \
		ns_udp.c ns_config.c ns_update.c ns_ixfr.c ns_signal.c \
		ns_sort.c ns_notify.c tmp_version.c
CPPFLAGS+=	${INCLUDE} -I${BIND_DIST_DIR}/bin/named
LDADD=		${LIBRARY}
CLEANFILES+=	y.tab.h named-bootconf tmp_version.c pathnames.h
MAN=		${PROG}.8 named.conf.5
YHEADER=	yes

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		named.conf
FILESDIR=	/usr/share/examples/named
.endif

HOSTNAMECMD=	hostname

tmp_version.c: ${BIND_DIST_DIR}/bin/named/version.c Makefile ../Makefile \
		${BIND_DIST_DIR}/Version
	(u=$${USER-root} d=`pwd` h=`${HOSTNAMECMD}` t=`date`; \
	sed -e "s|%WHEN%|$${t}|" -e "s|%VERSION%|"${VER}"|" \
	    -e "s|%WHOANDWHERE%|$${u}@$${h}:$${d}|" \
	    < ${BIND_DIST_DIR}/bin/named/version.c > tmp_version.c)

pathnames.h: ${BIND_DIST_DIR}/bin/named/pathtemplate.h
	sed -e "s|%DESTETC%|/etc|" -e "s|%DESTSBIN%|/usr/sbin|" \
	    -e "s|%DESTRUN%|/var/run|" -e "s|%DESTEXEC%|/usr/libexec|" \
	    < ${BIND_DIST_DIR}/bin/named/pathtemplate.h > pathnames.h

DPSRCS+=	pathnames.h
CLEANFILES+=	pathnames.h

.include "../../Makefile.inc"
.include <bsd.prog.mk>
