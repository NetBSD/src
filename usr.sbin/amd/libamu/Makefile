#	$NetBSD: Makefile,v 1.10 1999/02/13 02:54:58 lukem Exp $

LIB= amu
SRCS=	mtabutil.c tranputil.c umount_fs.c xutil.c xdr_func.c wire.c \
util.c nfs_prot_xdr.c mtab.c mount_fs.c misc_rpc.c hasmntopt.c config_local.h

MKLINKLIB=no
 
config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} > ${.TARGET}

CLEANFILES+=config_local.h

.include <bsd.lib.mk>
 
${OBJS}: config_local.h
