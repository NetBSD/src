#	$NetBSD: Makefile,v 1.4 1997/01/11 01:50:56 cjs Exp $

LIB=	wrap 

SRCS=	hosts_access.c options.c shell_cmd.c rfc931.c eval.c hosts_ctl.c \
	refuse.c percent_x.c clean_exit.c fix_options.c socket.c \
	workarounds.c update.c misc.c diag.c
MAN=	hosts_access.3 hosts_access.5 hosts_options.5
MLINKS+=hosts_access.5 hosts.allow.5
MLINKS+=hosts_access.5 hosts.deny.5
MLINKS+=hosts_access.3 hosts_ctl.3
MLINKS+=hosts_access.3 request_init.3
MLINKS+=hosts_access.3 request_set.3

.include "Makefile.cflags"

includes:
	-cd ${.CURDIR}; cmp -s tcpd.h ${DESTDIR}/usr/include/tcpd.h > \
	    /dev/null 2>&1 || \
	    ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 tcpd.h \
	    ${DESTDIR}/usr/include

.include <bsd.lib.mk>
