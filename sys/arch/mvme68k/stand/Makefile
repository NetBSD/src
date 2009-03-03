#	$NetBSD: Makefile,v 1.5.120.1 2009/03/03 18:29:04 skrll Exp $

SUBDIR= libbug libsa .WAIT sboot netboot bootxx bootsd bootst \
	installboot

.ifmake !install
SUBDIR+= wrtvid
.endif

.include <bsd.subdir.mk>
