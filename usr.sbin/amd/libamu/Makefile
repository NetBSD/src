#	$NetBSD: Makefile,v 1.1.1.1 1997/07/24 21:20:11 christos Exp $

LIB= amu
SRCS=	mtabutil.c tranputil.c umount_fs.c xutil.c xdr_mountres3.c wire.c \
util.c nfs_prot_xdr.c mtab.c mount_fs.c misc_rpc.c hasmntopt.c 

config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} > ${.TARGET}

.include <bsd.lib.mk>
LDADD=
DPADD=
${OBJS}: config_local.h
