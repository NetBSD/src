#	$NetBSD: Makefile,v 1.10.14.2 2001/02/11 19:12:46 bouyer Exp $

SA_PROG= netboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= dev_net.c bootparam.c globals.c nfs.c rpc.c udp.c

PROG=	${SA_PROG} ${SA_PROG}.bin
MKMAN= no
NOMAN= yes

.include <bsd.prog.mk>

.PATH: ${S}/lib/libsa
DEFS+=  -DSUN_BOOTPARAMS
