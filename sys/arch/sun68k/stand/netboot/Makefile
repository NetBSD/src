#	$NetBSD: Makefile,v 1.1.2.1 2002/01/10 19:49:56 thorpej Exp $

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
