#	$NetBSD: bsd.x11.mk,v 1.3 2003/09/13 20:10:44 lukem Exp $

.include <bsd.init.mk>


BINDIR=			${X11BINDIR}
LIBDIR=			${X11USRLIBDIR}
MANDIR=			${X11MANDIR}


X11FLAGS.VERSION=	-DOSMAJORVERSION=1 -DOSMINORVERSION=6		# XXX

X11FLAGS.THREADS=	-DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API \
			-DXNO_MTSAFE_PWDAPI

X11FLAGS.THREADLIB=	${X11FLAGS.THREADS} -DUSE_NBSD_THREADLIB

X11FLAGS.CONNECTION=	-DTCPCONN -DUNIXCONN -DHAS_STICKY_DIR_BIT -DHAS_FCHOWN

X11FLAGS.EXTENSION=	-DMITMISC -DXTEST -DXTRAP -DXSYNC -DXCMISC -DXRECORD \
			-DMITSHM -DBIGREQS -DXF86VIDMODE -DXF86MISC -DDBE \
			-DDPMSExtension -DEVI -DSCREENSAVER -DXV -DXVMC \
			-DGLXEXT -DGLX_USE_MESA -DFONTCACHE -DRES

X11FLAGS.SERVER=	-DSHAPE -DXINPUT -DXKB -DLBX -DXAPPGROUP -DXCSECURITY \
			-DTOGCUP -DXF86BIGFONT -DDPMSExtension -DPIXPRIV \
			-DPANORAMIX -DRENDER -DRANDR -DGCCUSESGAS \
			-DAVOID_GLYPHBLT -DPIXPRIV -DSINGLEDEPTH -DXFreeXDGA \
			-DXvExtension -DXFree86LOADER -DXFree86Server \
			-DXF86VIDMODE -DXvMCExtension -DSMART_SCHEDULE \
			-DBUILDDEBUG -DXResExtension -DNDEBUG -DX_BYTE_ORDER=0

X11TOOL_UNXCOMM=	sed -e '/^\#  *[0-9][0-9]*  *.*$$/d' \
			    -e '/^XCOMM$$/s//\#/' \
			    -e '/^XCOMM[^a-zA-Z0-9_]/s/^XCOMM/\#/'


CPPFLAGS+=		-DCSRG_BASED -DFUNCPROTO=15 -DNARROWPROTO
CPPFLAGS+=		-I${DESTDIR}${X11INCDIR}

LDFLAGS+=		-Wl,-rpath-link,${DESTDIR}${X11USRLIBDIR} \
			-R${X11USRLIBDIR} \
			-L${DESTDIR}${X11USRLIBDIR}


#
# .cpp -> "" handling
# CPPSCRIPTS		list of files/scripts to run through cpp
# CPPSCRIPTFLAGS	extra flags to ${CPP}
# CPPSCRIPTFLAGS_fn	extra flags to ${CPP} for file `fn'
#
.if defined(CPPSCRIPTS)						# {
.SUFFIXES:	.cpp

.cpp:
	@rm -f ${.TARGET}
	${CPP} -undef -traditional \
	    ${CPPSCRIPTFLAGS_${.TARGET}:U${CPPSCRIPTFLAGS}} \
	    < ${.IMPSRC} | ${X11TOOL_UNXCOMM} > ${.TARGET}

realall: ${CPPSCRIPTS}

clean: cleancppscripts
cleancppscripts:
	rm -f ${CPPSCRIPTS}
.endif								# }


#
# APPDEFS (app defaults) handling
#
.if defined(APPDEFS)						# {
appdefsinstall:: ${APPDEFS:@S@${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}@}
.PRECIOUS:       ${APPDEFS:@S@${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}@}

__appdefinstall: .USE
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${SYSPKGTAG} ${.ALLSRC} ${.TARGET}

.for S in ${APPDEFS:O:u}
${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}: ${S} __appdefinstall
.endfor

realinstall: appdefsinstall
.endif								# }


#
# .man page handling
#
.if (${MAN:U} != "" && ${MKMAN} != "no")			# {
cleandir: cleanx11man
cleanx11man:
	rm -f ${MAN}
.endif								# }

.SUFFIXES:	.man .1 .3 .7

.man.1 .man.3 .man.7:
	rm -f ${.TARGET}
	${CPP} -undef -traditional \
	    -D__apploaddir__=${X11ROOTDIR}/lib/X11/app-defaults \
	    -D__filemansuffix__=5 -D__libmansuffix__=3 \
	    -D__miscmansuffix__=7 -D__drivermansuffix__=4 \
	    -D__projectroot__=${X11ROOTDIR} \
	    -D__xorgversion__='"Release 6.6" "X Version 11"' \
	    -D__vendorversion__="XFree86 4.3.0-imakeicideII" \
	    ${X11EXTRAMANDEFS} \
	< ${.IMPSRC} | ${X11TOOL_UNXCOMM} > ${.TARGET}
