#	$NetBSD: Makefile,v 1.4.6.2 2002/06/20 03:40:02 nathanw Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !(install)
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
