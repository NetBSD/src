# $NetBSD: Makefile.inc,v 1.18 2009/10/23 23:23:56 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/Makefile.inc,v 2.7 2003/05/01 13:22:18 ru Exp $

BINDIR?=	/usr/bin
ECHO=		echo

# Define `page' to be letter if your PostScript printer uses 8.5x11
# paper (USA) and define it to be A4, if it uses A4 paper (rest of the
# world).
PAGE=letter

# Normally the Postscript driver, grops, produces output that conforms
# to version 3.0 of the Adobe Document Structuring Conventions.
# Unfortunately some spoolers and previewers can't handle such output.
# The BROKEN_SPOOLER_FLAGS variable tells grops what it should do to
# make its output acceptable to such programs.  This variable controls
# only the default behaviour of grops; the behaviour can be changed at
# runtime by the grops -b option (and so by groff -P-b).
#
# Use a value of 0 if your spoolers and previewers are able to handle
# conforming PostScript correctly.
#
# Add 1 if no %%{Begin,End}DocumentSetup comments should be generated;
# this is needed for early versions of TranScript that get confused by
# anything between the %%EndProlog line and the first %%Page: comment.
#
# Add 2 if lines in included files beginning with %! should be
# stripped out; this is needed for the OpenWindows 2.0 pageview previewer.
#
# Add 4 if %%Page, %%Trailer and %%EndProlog comments should be
# stripped out of included files; this is needed for spoolers that
# don't understand the %%{Begin,End}Document comments. I suspect this
# includes early versions of TranScript.
#
# Add 8 if the first line of the PostScript output should be %!PS-Adobe-2.0
# rather than %!PS-Adobe-3.0; this is needed when using Sun's Newsprint
# with a printer that requires page reversal.
#
BROKEN_SPOOLER_FLAGS=0

# DEVICE is the default device.
DEVICE=ps

# PSPRINT is the command to use for printing a PostScript file,
# for example `lpr'.
PSPRINT=lpr

# DVIPRINT is the command to use for printing a TeX dvi file,
# for example `lpr -d'.
DVIPRINT=lpr -d
########################################################################
# Don't touch...

g=
tmac_s_prefix=
tmac_m_prefix=
tmac_an_prefix=
fontdir=/usr/share/groff_font
localfontdir=$(fontdir)
legacyfontdir=$(fontdir)
tmacdir=/usr/share/tmac
systemtmacdir=/usr/share/tmac
localtmacdir=/usr/share/tmac
docdir=/usr/src/contrib/groff/doc
exampledir=XXX
htmldocdir=XXX
pdfdocdir=XXX
indexext=.i
common_words_file=/usr/share/dict/eign
indexdir=/usr/share/dict/papers
indexname=Ind

########################################################################
# Libraries

.include <bsd.own.mk>

.ifndef LIBGROFF
LIBGROFFDIR!=cd ${NETBSDSRCDIR}/gnu/usr.bin/groff/src/libs/libgroff && ${PRINTOBJDIR}
LIBDRIVERDIR!=cd ${NETBSDSRCDIR}/gnu/usr.bin/groff/src/libs/libdriver && ${PRINTOBJDIR}
LIBBIBDIR!=cd ${NETBSDSRCDIR}/gnu/usr.bin/groff/src/libs/libbib && ${PRINTOBJDIR}

LIBGROFF=       ${LIBGROFFDIR}/libgroff.a
LIBDRIVER=      ${LIBDRIVERDIR}/libdriver.a
LIBBIB=         ${LIBBIBDIR}/libbib.a
.endif

CFLAGS+=	-DHAVE_CONFIG_H
CFLAGS+=	-I${GROFF_DIST}/src/include -I${NETBSDSRCDIR}/gnu/usr.bin/groff/src/include
CXXFLAGS+=	-fno-rtti -fno-exceptions

# We only need -lsupc++
USE_LIBSTDCXX=	no


.ifdef GROFFYSRCS
YFLAGS+=	-d
SRCS+=		${GROFFYSRCS:.y=.cpp}
DPSRCS+=	${GROFFYSRCS:.y=.cpp}
CLEANFILES+=	${GROFFYSRCS:.y=.cpp} ${GROFFYSRCS:.y=_tab.h}
.endif

.y.cpp:
	${_MKTARGET_YACC}
	${YACC.y} -o ${.TARGET} ${.IMPSRC}
	mv -f ${.PREFIX}.h ${.PREFIX}_tab.h


.SUFFIXES: .man .1 .2 .3 .4 .5 .6 .7 .8

version=`cat $(GROFF_DIST)/VERSION`
revision=`${TOOL_SED} -e 's/^0$$//' -e 's/^[1-9].*$$/.&/' \
		$(GROFF_DIST)/REVISION`

.man.8 .man.7 .man.6 .man.5 .man.4 .man.3 .man.2 .man.1:
	${_MKTARGET_CREATE}
	rm -f $@
	${TOOL_SED} -e "s;@BINDIR@;${BINDIR};g" \
	-e "s;@FONTDIR@;$(fontdir);g" \
	-e "s;@LOCALFONTDIR@;$(localfontdir);g" \
	-e "s;@LEGACYFONTDIR@;$(legacyfontdir);g" \
	-e "s;@MACRODIR@;$(tmacdir);g" \
	-e "s;@SYSTEMMACRODIR@;$(systemtmacdir);g" \
	-e "s;@LOCALMACRODIR@;$(localtmacdir);g" \
	-e "s;@DOCDIR@;$(docdir);g" \
	-e "s;@EXAMPLEDIR@;$(exampledir);g" \
	-e "s;@HTMLDOCDIR@;$(htmldocdir);g" \
	-e "s;@PDFDOCDIR@;$(pdfdocdir);g" \
	-e "s;@DEVICE@;$(DEVICE);g" \
	-e "s;@DEFAULT_INDEX@;$(indexdir)/$(indexname);g" \
	-e "s;@DEFAULT_INDEX_NAME@;$(indexname);g" \
	-e "s;@INDEX_SUFFIX@;$(indexext);g" \
	-e "s;@COMMON_WORDS_FILE@;$(common_words_file);g" \
	-e "s;@MAN1EXT@;1;g" \
	-e "s;@MAN5EXT@;5;g" \
	-e "s;@MAN7EXT@;7;g" \
	-e "s;@TMAC_S_PREFIX@;$(tmac_s_prefix);g" \
	-e "s;@TMAC_M_PREFIX@;$(tmac_m_prefix);g" \
	-e "s;@TMAC_AN_PREFIX@;$(tmac_an_prefix);g" \
	-e "s;@TMAC_MDIR@;$(tmacdir)/mm;g" \
	-e "s;@BROKEN_SPOOLER_FLAGS@;$(BROKEN_SPOOLER_FLAGS);g" \
	-e "s;@VERSION@;$(version)$(revision);g" \
	-e "s;@MDATE@;`${HOST_SH} ${GROFF_DIST}/mdate.sh $<`;g" \
	-e "s;@g@;$(g);g" \
	-e "s;@G@;`echo $(g) | LC_ALL=C tr [a-z] [A-Z]`;g" \
	$< >$@

.SUFFIXES: .sh .pl

.sh .pl:
	${_MKTARGET_CREATE}
	rm -f $@
	${TOOL_SED} -e "s|@BINDIR@|${BINDIR}|g" \
	     -e "s;@FONTDIR@;$(fontdir);g" \
	     -e 's|@GROFF_BIN_PATH_SETUP@|GROFF_RUNTIME="$${GROFF_BIN_PATH=/usr/bin}:"|g' \
	     -e "s|@SEP@|:|" \
	     -e "s|@VERSION@|$(version)$(revision)|" \
	     -e "s|@g@|$(g)|g" \
	     ${.IMPSRC} >${.TARGET}

GROFF_DIST=	${NETBSDSRCDIR}/gnu/dist/groff
DIST_SUBDIR?=	${.CURDIR:T}
DIST_DIR=	${GROFF_DIST}/${DIST_SUBDIR}
.PATH: ${DIST_DIR}

NOMANDOC=	yes
