#	$NetBSD: Makefile.inc,v 1.2 2001/12/14 22:21:46 thorpej Exp $

SRCS+=	rtld_start.S

COPTS+=	-fpic

CPPFLAGS+= -DELFSIZE=32 -DRTLD_RELOCATE_SELF -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
