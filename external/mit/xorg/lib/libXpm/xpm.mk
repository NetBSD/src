#	$NetBSD: xpm.mk,v 1.2.4.2 2023/01/23 13:43:07 martin Exp $

CPPFLAGS+=	-DHAS_SNPRINTF
CPPFLAGS+=	-DXPM_PATH_COMPRESS=\"/usr/bin/compress\"
CPPFLAGS+=	-DXPM_PATH_GZIP=\"/usr/bin/gzip\"
CPPFLAGS+=	-DXPM_PATH_UNCOMPRESS=\"/usr/bin/uncompress\"
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
