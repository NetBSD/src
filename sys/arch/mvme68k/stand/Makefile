#	$NetBSD: Makefile,v 1.3.4.2 2002/06/23 17:38:21 jdolecek Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !(install)
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
