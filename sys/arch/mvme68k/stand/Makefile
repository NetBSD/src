#	$NetBSD: Makefile,v 1.5.112.1 2009/05/04 08:11:35 yamt Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !install
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
