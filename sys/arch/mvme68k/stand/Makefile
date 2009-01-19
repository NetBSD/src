#	$NetBSD: Makefile,v 1.6 2009/01/19 23:09:16 he Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !install
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
