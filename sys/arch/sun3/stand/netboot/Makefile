#	$NetBSD: Makefile,v 1.10.14.1 2000/11/20 20:27:56 bouyer Exp $

SA_PROG= netboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= dev_net.c bootparam.c globals.c nfs.c rpc.c

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

.PATH: ${S}/lib/libsa
DEFS+=  -DSUN_BOOTPARAMS
