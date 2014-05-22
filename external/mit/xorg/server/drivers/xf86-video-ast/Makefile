#	$NetBSD: Makefile,v 1.4.2.1 2014/05/22 14:09:45 yamt Exp $

DRIVER=		xf86-video-ast
DRIVER_NAME=	ast_drv

SRCS=		ast_2dtool.c ast_accel.c ast_cursor.c ast_driver.c
SRCS+=		ast_mode.c ast_tool.c ast_vgatool.c

CPPFLAGS+=	-DHAVE_XAA_H

CWARNFLAGS.clang+=	-Wno-format

.include "../Makefile.xf86-driver"
