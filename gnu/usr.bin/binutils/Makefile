#	$NetBSD: Makefile,v 1.8 1998/11/21 22:54:34 kristerw Exp $

.include <bsd.own.mk>

SUBDIR=	common addr2line ar c++filt nm objcopy objdump ranlib size strings

# XXX once a.out works, we can add strip to all archs
.if (${OBJECT_FMT} == "ELF")
SUBDIR+=strip
.endif

all-addr2line all-ar all-c++filt all-nm all-objcopy all-objdump \
    all-ranlib all-size all-strings all-strip: all-common

.include <bsd.subdir.mk>
