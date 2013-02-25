# $NetBSD: Makefile,v 1.1.2.3 2013/02/25 00:26:03 tls Exp $

.include <bsd.init.mk>
.include <bsd.endian.mk>

.PATH:		${DIST}/elftosb2

.if ${MACHINE} == "evbarm" \
    && ${MACHINE_CPU} == "arm" \
    && ${TARGET_ENDIANNESS} == "1234"

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
SRCS.lib!=cd ${.PARSEDIR}/../../lib && ${MAKE} -V '$${SRCS}'
SRCS+=${SRCS.lib}
.endif

.endif

MAN=		elftosb.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

