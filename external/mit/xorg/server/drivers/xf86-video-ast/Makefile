#	$NetBSD: Makefile,v 1.6 2016/08/16 06:24:01 mrg Exp $

DRIVER=		xf86-video-ast
DRIVER_NAME=	ast_drv

SRCS=		ast_2dtool.c ast_accel.c ast_cursor.c ast_driver.c
SRCS+=		ast_mode.c ast_tool.c ast_vgatool.c

CWARNFLAGS.clang+=	-Wno-format

.include "../Makefile.xf86-driver"
