#	$NetBSD: Makefile,v 1.22 2004/08/06 16:31:43 mycroft Exp $
#
#	@(#)Makefile	8.3 (Berkeley) 5/16/94

.include <bsd.own.mk>

# The following modules do not build/install:
# 08.mh

# Missing:
# 14.jove 15.sed 28.bib

# Missing from 4.4BSD-Lite:
# 01.begin 02.learn 03.shell 05.dc 06.bc 09.edtut 10.edadv
# 16.awk 17.msmacros 21.troff 22.trofftut 23.eqn 24.eqnguide
# 25.tbl 26.refer 27.invert 29.diction

BINDIR=	/usr/share/doc/usd
.if ${MKDOC} != "no"
FILES=	00.contents Makefile Title
.endif
#SUBDIR+=04.csh 07.mail 11.edit 12.ex 12.vi 13.viref
SUBDIR+=18.msdiffs 19.memacros 20.meref
#SUBDIR+=30.rogue 31.trek

Title.ps: ${FILES}
	${TOOL_ROFF_PS} -ms Title > ${.TARGET}

contents.ps: ${FILES}
	${TOOL_ROFF_PS} -ms 00.contents > ${.TARGET}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
