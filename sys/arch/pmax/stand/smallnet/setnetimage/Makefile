# $NetBSD: Makefile,v 1.3.2.3 2002/04/17 00:04:11 nathanw Exp $

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
	${ELF2ECOFF} smallnet.elf smallnet
.endif

.include <bsd.prog.mk>
