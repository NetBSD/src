#	$NetBSD: Makefile,v 1.47 2010/07/06 05:59:53 mrg Exp $

.include <bsd.own.mk>			# for MKPIC definition

.if ${MKPIC} != "no" && exists(ld.so.${MACHINE_ARCH}.uue)

FILES=		ld.so
FILESDIR=	/usr/libexec
FILESMODE=	${BINMODE}
UUDECODE_FILES=	ld.so.${MACHINE_ARCH}
UUDECODE_FILES_RENAME_${UUDECODE_FILES}= ld.so

all dependall: ${FILES}

ld.so:	ld.so.${MACHINE_ARCH}

.endif

MAN= ld.aout_so.1

.include <bsd.files.mk>
.include <bsd.prog.mk>
