#	$NetBSD: Makefile,v 1.4.14.1 2002/05/30 15:33:44 gehenna Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !(install)
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
