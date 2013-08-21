#	$NetBSD: Makefile,v 1.5 2013/08/21 05:45:16 matt Exp $

WITHOUT_MAN=true  # defined

.include <bsd.obj.mk>		# Pull in OBJDIR name rules.
.include <bsd.own.mk>
.include <bsd.klinks.mk>

LIB=	kern
LIBISPRIVATE?=	yes
DBG=	-Os

S= ${NETBSDSRCDIR}/sys
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I${S}/arch

CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

### find out what to use for libkern
KERN_AS=	library
KERNDIR=	${S}/lib/libkern

.include "${S}/lib/libkern/Makefile.libkern"
.ifndef ARCHSUBDIR
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

.include <bsd.lib.mk>
