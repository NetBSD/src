#	$NetBSD: Makefile,v 1.3.4.1 2001/08/03 04:12:05 lukem Exp $

SUBDIR= libbug libsa sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !(install)
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
