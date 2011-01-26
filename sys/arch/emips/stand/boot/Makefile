#	$NetBSD: Makefile,v 1.1 2011/01/26 01:18:53 pooka Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

SECONDARY_PROG=	boot.emips
STRIPFLAG=

SRCS=		start.S boot.c
SRCS+=		printf.c putchar.c prom_iface.c
SRCS+=		bootinfo.c clock.c conf.c devopen.c dev_net.c
SRCS+=		enic.c ace.c raw.c init_board.c

CPPFLAGS+=	-DBOOT_TYPE_NAME='"Netboot"' \
		-DLIBSA_BOOTABLE_FILES_WILDCARD=\'?\' \
		-DBOOTNET -DSUPPORT_BOOTP -DSUPPORT_DHCP

#CPPFLAGS+=	-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG \
#		-DRPC_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID -DARP_DEBUG \
#		-DDEBUG_VAL=1

# Copy boot.emips to / as well.
afterinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${STRIPFLAG} ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/${PROG}

.include "../Makefile.booters"
