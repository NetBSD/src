#	$NetBSD: Makefile,v 1.1.1.2 1997/09/22 21:11:25 christos Exp $

LIB= amu
SRCS=	mtabutil.c tranputil.c umount_fs.c xutil.c xdr_func.c wire.c \
util.c nfs_prot_xdr.c mtab.c mount_fs.c misc_rpc.c hasmntopt.c 

config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} > ${.TARGET}

CLEANFILES+=config_local.h

.include <bsd.lib.mk>
LDADD=
DPADD=
${OBJS}: config_local.h
