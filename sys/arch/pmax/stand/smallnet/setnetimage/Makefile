# $NetBSD: Makefile,v 1.1 1999/05/13 08:38:05 simonb Exp $

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
SNDIR!=	cd ${.CURDIR}/.. ; make print-objdir

smallnet.elf: ${SNDIR}/smallnet
	cp -p ${.ALLSRC} ${.TARGET}

kernel: smallnet.elf
	./${PROG} ${KERNEL} ${.ALLSRC}
	elf2ecoff smallnet.elf smallnet
.endif
	

.include <bsd.prog.mk>
