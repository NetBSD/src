#	$NetBSD: Makefile,v 1.5 2002/05/28 19:34:15 bjh21 Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !(install)
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
