#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:03 tv Exp $

SA_PROG= netboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= dev_net.c bootparam.c globals.c nfs.c rpc.c udp.c

PROG=	${SA_PROG}
NOMAN= # defined

.include <bsd.prog.mk>

.PATH: ${S}/lib/libsa
DEFS+=  -DSUN_BOOTPARAMS
