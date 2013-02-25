#	$NetBSD: Makefile,v 1.47.12.1 2013/02/25 00:28:02 tls Exp $

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

.else
UUDECODE_FILES=
.endif
.endif

MAN= ld.aout_so.1

.include <bsd.files.mk>
.include <bsd.prog.mk>
