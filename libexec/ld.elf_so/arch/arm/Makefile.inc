#	$NetBSD: Makefile.inc,v 1.1 2001/07/15 03:08:33 matt Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
