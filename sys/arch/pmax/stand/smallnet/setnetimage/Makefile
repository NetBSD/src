# $NetBSD: Makefile,v 1.2.4.1 2002/01/10 19:47:57 thorpej Exp $

PROG=	setnetimage
NOMAN=	# defined
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
.include <bsd.own.mk>

SNDIR!=	cd ${.CURDIR}/.. && ${PRINTOBJDIR}

smallnet.elf: ${SNDIR}/smallnet
	cp -p ${.ALLSRC} ${.TARGET}

kernel: smallnet.elf
	./${PROG} ${KERNEL} ${.ALLSRC}
	elf2ecoff smallnet.elf smallnet
.endif

.include <bsd.prog.mk>
