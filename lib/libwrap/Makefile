#	$NetBSD: Makefile,v 1.2 1996/11/28 04:32:32 mrg Exp $

LIB=	wrap 

SRCS=	hosts_access.c options.c shell_cmd.c rfc931.c eval.c hosts_ctl.c \
	refuse.c percent_x.c clean_exit.c fix_options.c socket.c \
	workarounds.c update.c misc.c diag.c
MAN=	hosts_access.3 hosts_access.5 hosts_options.5

CFLAGS+= -DFACILITY=LOG_AUTHPRIV -DSEVERITY=LOG_INFO -DPARANOID
CFLAGS+= -DREAL_DAEMON_DIR=\"/usr/libexec\" -DHOSTS_ACCESS -DDAEMON_UMASK=022
CFLAGS+= -DRFC931_TIMEOUT=10 -DALWAYS_HOSTNAME -DSYS_ERRLIST_DEFINED
CFLAGS+= -DHOSTS_ALLOW=\"/etc/hosts.allow\" -DHOSTS_DENY=\"/etc/hosts.deny\"

includes:
	-cd ${.CURDIR}; cmp -s tcpd.h ${DESTDIR}/usr/include/tcpd.h > \
	    /dev/null 2>&1 || \
	    ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 tcpd.h \
	    ${DESTDIR}/usr/include

.include <bsd.lib.mk>
