#	$NetBSD: Makefile,v 1.7 1998/04/09 00:52:06 tv Exp $

LIB= amu
SRCS=	mtabutil.c tranputil.c umount_fs.c xutil.c xdr_func.c wire.c \
util.c nfs_prot_xdr.c mtab.c mount_fs.c misc_rpc.c hasmntopt.c config_local.h

NOLINT=
NOPROFILE=
NOSTATICLIB=

config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} > ${.TARGET}

CLEANFILES+=config_local.h

install: __libinstall
 
.include <bsd.lib.mk>
 
.if !defined(NOPIC)
__libinstall: ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.else
__libinstall:
.endif

LDADD=
DPADD=
${OBJS}: config_local.h
