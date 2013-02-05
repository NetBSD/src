#	$NetBSD: Makefile,v 1.48 2013/02/05 07:22:59 matt Exp $

.include <bsd.own.mk>			# for MKPIC definition
.include <bsd.endian.mk>		# for TARGET_ENDIANNESS

.if ${MKPIC} != "no"
.if ${TARGET_ENDIANNESS} == "1234" && ${MACHINE_CPU} == "arm"
UUDECODE_FILES=	ld.so.${MACHINE_CPU}
.else
UUDECODE_FILES=	ld.so.${MACHINE_ARCH}
.endif

.if exists(${UUDECODE_FILES}.uue)
FILES=		ld.so
FILESDIR=	/usr/libexec
FILESMODE=	${BINMODE}
UUDECODE_FILES_RENAME_${UUDECODE_FILES}= ld.so

all dependall: ${FILES}

ld.so:	${UUDECODE_FILES}

.endif
.endif

MAN= ld.aout_so.1

.include <bsd.files.mk>
.include <bsd.prog.mk>
