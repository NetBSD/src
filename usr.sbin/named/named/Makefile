#	$NetBSD: Makefile,v 1.4 1997/05/06 20:46:31 gwr Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:45 vixie Exp 

.PATH.c: ${.CURDIR}/../man

PROG=	named
SRCS=	version.c db_dump.c db_glue.c db_load.c db_lookup.c db_reload.c \
	db_save.c db_secure.c db_update.c ns_forw.c \
	ns_init.c ns_main.c ns_maint.c ns_ncache.c ns_req.c ns_resp.c \
	ns_sort.c ns_stats.c ns_udp.c ns_validate.c storage.c tree.c
MAN=	named.8

makeversionc:
	(sed -e "s|%VERSION%|${VER}|" \
	    < ${.CURDIR}/Version.c > ${.CURDIR}/version.c)

.include "../../Makefile.inc"
.include <bsd.prog.mk>
