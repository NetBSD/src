#	$NetBSD: Makefile,v 1.14 2001/03/21 11:02:24 tsutsui Exp $

SA_PROG= netboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= dev_net.c bootparam.c globals.c nfs.c rpc.c udp.c

PROG=	${SA_PROG}
MKMAN= no

.include <bsd.prog.mk>

.PATH: ${S}/lib/libsa
DEFS+=  -DSUN_BOOTPARAMS
