#	$NetBSD: Makefile,v 1.4 2001/07/19 18:18:30 scw Exp $

SUBDIR= libbug libsa sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !(install)
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
