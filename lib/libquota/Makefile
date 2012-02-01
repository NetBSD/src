#	$NetBSD: Makefile,v 1.8 2012/02/01 17:52:07 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

WARNS=	4
LIB=	quota

LIBDPLIBS+=     rpcsvc  ${.CURDIR}/../librpcsvc

SRCS+=	quota_open.c
SRCS+=	quota_schema.c
SRCS+=	quota_get.c quota_put.c quota_delete.c
SRCS+=	quota_cursor.c
SRCS+=	quota_nfs.c
SRCS+=	quota_oldfiles.c
SRCS+=	quota_kernel.c

MAN=	libquota.3
MLINKS=	libquota.3 quota_close.3 \
	libquota.3 quota_delete.3 \
	libquota.3 quota_get.3 \
	libquota.3 quota_getimplname.3 \
	libquota.3 quota_getmountdevice.3 \
	libquota.3 quota_getmountpoint.3 \
	libquota.3 quota_getnumidtypes.3 \
	libquota.3 quota_getnumobjtypes.3 \
	libquota.3 quota_idtype_getname.3 \
	libquota.3 quota_objtype_getname.3 \
	libquota.3 quota_objtype_isbytes.3 \
	libquota.3 quota_open.3 \
	libquota.3 quota_opencursor.3 \
	libquota.3 quota_put.3 \
	libquota.3 quotacursor_atend.3 \
	libquota.3 quotacursor_close.3 \
	libquota.3 quotacursor_get.3 \
	libquota.3 quotacursor_getn.3 \
	libquota.3 quotacursor_rewind.3 \
	libquota.3 quotacursor_skipidtype.3 \
	libquota.3 quotaval_clear.3

.include <bsd.lib.mk>
