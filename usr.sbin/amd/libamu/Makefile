#	$NetBSD: Makefile,v 1.9 1998/09/13 23:25:19 tv Exp $

LIB= amu
SRCS=	mtabutil.c tranputil.c umount_fs.c xutil.c xdr_func.c wire.c \
util.c nfs_prot_xdr.c mtab.c mount_fs.c misc_rpc.c hasmntopt.c config_local.h

NOLINKLIB=
 
config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} > ${.TARGET}

CLEANFILES+=config_local.h

.include <bsd.lib.mk>
 
${OBJS}: config_local.h
