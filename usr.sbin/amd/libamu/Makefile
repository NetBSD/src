#	$NetBSD: Makefile,v 1.2 1997/07/28 00:47:01 christos Exp $

LIB= amu
SRCS=	mtabutil.c tranputil.c umount_fs.c xutil.c xdr_mountres3.c wire.c \
util.c nfs_prot_xdr.c mtab.c mount_fs.c misc_rpc.c hasmntopt.c 

config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} > ${.TARGET}

CLEANFILES+= config_local.h

.include <bsd.lib.mk>
LDADD=
DPADD=
${OBJS}: config_local.h
