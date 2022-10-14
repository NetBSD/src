# $NetBSD: Makefile,v 1.2 2022/10/14 19:41:18 ryo Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		Makefile.inst aarch64.s arm.s i386.s powerpc.s sh3.s x86_64.s
FILESDIR=	/usr/share/examples/asm/hello
FILESNAME_Makefile.inst=	Makefile
.endif

.include <bsd.prog.mk>
