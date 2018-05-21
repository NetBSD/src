# $NetBSD: Makefile,v 1.5.4.1 2018/05/21 04:35:51 pgoyette Exp $

.include <bsd.init.mk>
.include <bsd.endian.mk>
.include <bsd.own.mk>

.if !defined(DIST)
.include "${.PARSEDIR}/../../Makefile.inc"
.endif

.PATH:		${DIST}/elftosb2

.if ${MACHINE_CPU} == "arm" && ${TARGET_ENDIANNESS} == "1234"

PROG_CXX=	elftosb
SRCS=		BootImageGenerator.cpp \
		ConversionController.cpp \
		ElftosbAST.cpp \
		ElftosbLexer.cpp \
		EncoreBootImageGenerator.cpp \
		elftosb.cpp \
		elftosb_lexer.cpp \
		elftosb_parser.tab.cpp

.ifdef HOSTPROG
SRCS.lib!=cd ${.PARSEDIR}/../../lib && ${MAKE} -v SRCS
SRCS+=${SRCS.lib}
.endif

.endif

MAN=		elftosb.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

