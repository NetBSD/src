# $NetBSD: Makefile,v 1.2 2012/11/27 20:00:38 jkunz Exp $

.include <bsd.init.mk>

.PATH:		${DIST}/elftosb2

.if (${MACHINE} == "evbarm") && (${MACHINE_ARCH} == "arm")
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

