# $NetBSD: Makefile,v 1.2 2000/08/30 23:51:54 jhawk Exp $

PROG=	setnetimage
MKMAN=	no
WARNS?=	1

MDSIDIR=${.CURDIR}/../../../../../../usr.sbin/mdsetimage

SRCS=	setnetimage.c exec_elf32.c
CPPFLAGS+=-I${MDSIDIR}
LDADD+=	-lz
DPADD+=	${LIBZ}

.PATH:	${MDSIDIR}

.if !defined(KERNEL)
kernel:
	@echo set KERNEL first!
	@false
.else
SNDIR!=	cd ${.CURDIR}/.. ; ${MAKE} print-objdir

smallnet.elf: ${SNDIR}/smallnet
	cp -p ${.ALLSRC} ${.TARGET}

kernel: smallnet.elf
	./${PROG} ${KERNEL} ${.ALLSRC}
	elf2ecoff smallnet.elf smallnet
.endif

.include <bsd.prog.mk>
