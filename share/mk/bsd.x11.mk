#	$NetBSD: bsd.x11.mk,v 1.55 2007/06/07 11:49:17 tron Exp $

.include <bsd.init.mk>

BINDIR=			${X11BINDIR}
LIBDIR=			${X11USRLIBDIR}
MANDIR=			${X11MANDIR}

COPTS+=			-fno-strict-aliasing

.if defined(USE_SSP) && (${USE_SSP} != "no")
CPPFLAGS+=		-DNO_ALLOCA
.endif

X11FLAGS.VERSION=	-DOSMAJORVERSION=1 -DOSMINORVERSION=6		# XXX

#	 THREADS_DEFINES
X11FLAGS.THREADS=	-DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API \
			-DXNO_MTSAFE_PWDAPI

#	 CONNECTION_FLAGS
X11FLAGS.CONNECTION=	-DTCPCONN -DUNIXCONN -DHAS_STICKY_DIR_BIT \
			-DHAS_FCHOWN

.if (${USE_INET6} != "no")
X11FLAGS.CONNECTION+=	-DIPv6
.endif

#	 EXT_DEFINES
X11FLAGS.EXTENSION=	-DMITMISC -DXTEST -DXTRAP -DXSYNC -DXCMISC -DXRECORD \
			-DMITSHM -DBIGREQS -DXF86MISC -DDBE -DDPMSExtension \
			-DEVI -DSCREENSAVER -DXV -DXVMC -DGLXEXT \
			-DGLX_USE_MESA -DFONTCACHE -DRES

X11FLAGS.DRI=		-DGLXEXT -DXF86DRI -DGLX_DIRECT_RENDERING \
			-DGLX_USE_DLOPEN -DGLX_USE_MESA

.if ${X11DRI} != "no"
X11FLAGS.EXTENSION+=	${X11FLAGS.DRI}
.endif

#	 ServerDefines
X11FLAGS.SERVER=	-DSHAPE -DXKB -DLBX -DXAPPGROUP -DXCSECURITY \
			-DTOGCUP -DXF86BIGFONT -DDPMSExtension -DPIXPRIV \
			-DPANORAMIX -DRENDER -DRANDR -DGCCUSESGAS \
			-DAVOID_GLYPHBLT -DSINGLEDEPTH -DXvExtension \
			-DXFree86Server -DXvMCExtension -DSMART_SCHEDULE \
			-DBUILDDEBUG -DXResExtension -DNDEBUG

#	 OS_DEFINES
X11FLAGS.OS_DEFINES=	-DDDXOSINIT -DSERVER_LOCK -DDDXOSFATALERROR \
			-DDDXOSVERRORF -DDDXTIME -DUSB_HID

.if !(${MACHINE} == "acorn32"	|| \
    ${MACHINE} == "alpha"	|| \
    ${MACHINE} == "amiga"	|| \
    ${MACHINE} == "pmax"	|| \
    ${MACHINE} == "sun3"	|| \
    ${MACHINE} == "vax")
#	EXT_DEFINES
X11FLAGS.EXTENSION+=	-DXF86VIDMODE

#	ServerDefines
X11FLAGS.SERVER+=	-DXINPUT -DXFreeXDGA -DXF86VIDMODE
.endif

.if ${MACHINE_ARCH} == "alpha"	|| \
    ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "x86_64"
#	ServerDefines
X11FLAGS.SERVER+=	-D_XSERVER64
X11FLAGS.EXTENSION+=	-D__GLX_ALIGN64
.endif

.if ${MACHINE} == "amd64"	|| \
    ${MACHINE} == "cats"	|| \
    ${MACHINE} == "i386"	|| \
    ${MACHINE} == "macppc"	|| \
    ${MACHINE} == "netwinder"	|| \
    ${MACHINE} == "sgimips"	|| \
    ${MACHINE} == "sparc64"	|| \
    ${MACHINE} == "sparc"	|| \
    ${MACHINE} == "shark"
#	LOADABLE
X11FLAGS.LOADABLE=	-DXFree86LOADER -DIN_MODULE -DXFree86Module \
			-fno-merge-constants
.endif

# Extract X11VERSION
PRINTX11VERSION=awk '/^\#define XF86_VERSION_MAJOR/ {major = $$3} \
		     /^\#define XF86_VERSION_MINOR/ {minor = $$3} \
		     /^\#define XF86_VERSION_PATCH/ {patch = $$3} \
		     /^\#define XF86_VERSION_SNAP/ {snap = $$3} \
		     END { print "((("major") * 10000000) + (("minor") * 100000) + (("patch") * 1000) + "snap")"}' \
		     ${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf86Version.h

# Commandline to convert 'XCOMM' comments and 'XHASH' to '#', among other
# things. Transformed from the "CppSedMagic" macro from "Imake.rules".
#
X11TOOL_UNXCOMM=    sed	-e '/^\#  *[0-9][0-9]*  *.*$$/d' \
			-e '/^\#line  *[0-9][0-9]*  *.*$$/d' \
			-e '/^[ 	]*XCOMM$$/s/XCOMM/\#/' \
			-e '/^[ 	]*XCOMM[^a-zA-Z0-9_]/s/XCOMM/\#/' \
			-e '/^[ 	]*XHASH/s/XHASH/\#/' \
			-e '/\@\@$$/s/\@\@$$/\\/'


CPPFLAGS+=		-DCSRG_BASED -DFUNCPROTO=15 -DNARROWPROTO
CPPFLAGS+=		-I${DESTDIR}${X11INCDIR}

.if ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=		-D__AMD64__
.endif

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
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${CPP} -undef -traditional \
	    ${CPPSCRIPTFLAGS_${.TARGET}:U${CPPSCRIPTFLAGS}} \
	    < ${.IMPSRC} | ${X11TOOL_UNXCOMM} > ${.TARGET}

realall: ${CPPSCRIPTS}

clean: cleancppscripts
cleancppscripts: .PHONY
	rm -f ${CPPSCRIPTS}
.endif								# }


#
# APPDEFS (app defaults) handling
#
.if defined(APPDEFS)						# {
appdefsinstall:: .PHONY ${APPDEFS:@S@${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}@}
.PRECIOUS:	${APPDEFS:@S@${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}@}

__appdefinstall: .USE
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
	    ${.ALLSRC} ${.TARGET}

.for S in ${APPDEFS:O:u}
${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}: ${S} __appdefinstall
.endfor

realinstall: appdefsinstall
.endif								# }


#
# .man page handling
#
.if (${MKMAN} != "no" && (${MAN:U} != "" || ${PROG:U} != ""))	# {
cleandir: cleanx11man
cleanx11man: .PHONY
	rm -f ${MAN:U${PROG:D${PROG.1}}}
.endif								# }

.SUFFIXES:	.man .1 .3 .4 .5 .7

.man.1 .man.3 .man.4 .man.5 .man.7:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	sed -e 's/\\$$/\\ /' ${.IMPSRC} \
	| ${CPP} -undef -traditional \
	    -D__apploaddir__=${X11ROOTDIR}/lib/X11/app-defaults \
	    -D__libmansuffix__=3 \
	    -D__filemansuffix__=5 \
	    -D__miscmansuffix__=7 \
	    -D__drivermansuffix__=4 \
	    -D__adminmansuffix__=8 \
	    -D__projectroot__=${X11ROOTDIR} \
	    -D__xorgversion__='"Release 6.6" "X Version 11"' \
	    -D__vendorversion__="XFree86 4.5.0" \
	    ${X11EXTRAMANDEFS} \
	| ${X11TOOL_UNXCOMM} > ${.TARGET}
