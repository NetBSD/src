#	$NetBSD: Makefile,v 1.10.14.3 2001/03/12 13:29:39 bouyer Exp $

SA_PROG= netboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= dev_net.c bootparam.c globals.c nfs.c rpc.c udp.c

PROG=	${SA_PROG}
MKMAN= no
NOMAN= yes

.include <bsd.prog.mk>

.PATH: ${S}/lib/libsa
DEFS+=  -DSUN_BOOTPARAMS
