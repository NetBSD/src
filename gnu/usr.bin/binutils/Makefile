#	$NetBSD: Makefile,v 1.7 1998/09/14 17:49:21 tv Exp $

.include <bsd.own.mk>

SUBDIR=	common addr2line ar c++filt nm objdump ranlib size strings

# XXX once a.out works, we can add objcopy and strip to all archs
.if (${OBJECT_FMT} == "ELF")
SUBDIR+=strip
.endif

all-addr2line all-ar all-c++filt all-nm all-objcopy all-objdump \
    all-ranlib all-size all-strings all-strip: all-common

.include <bsd.subdir.mk>
