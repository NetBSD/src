#	$NetBSD: Makefile.inc,v 1.2 2000/12/21 09:12:54 scw Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
