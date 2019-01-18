# $NetBSD: Makefile,v 1.5.200.1 2019/01/18 08:50:21 pgoyette Exp $

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
	${MIPS_ELF2ECOFF} smallnet.elf smallnet
.endif

.include <bsd.prog.mk>
