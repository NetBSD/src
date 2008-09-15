#	$NetBSD: bsd.x11.mk,v 1.66 2008/09/15 08:05:19 rtr Exp $

.include <bsd.init.mk>

.if make(depend) || make(all) || make(dependall)
.if (${MKX11} != "no" && ${MKXORG} != "no")
.BEGIN:
	@echo
	@echo "ERROR: \$$MKX11 and \$$MKXORG are mutually exclusive."
	@echo
	@false
.endif
.endif

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
.if ${MKXORG} != "no"
X11FLAGS.BASE_EXTENSION=	-DMITMISC -DXTEST -DXTRAP -DXSYNC -DXCMISC \
				-DXRECORD -DMITSHM -DBIGREQS -DXF86VIDMODE \
				-DXF86MISC -DDPMSExtension -DEVI \
				-DSCREENSAVER -DXV -DXVMC -DGLXEXT \
				-DFONTCACHE -DRES

X11FLAGS.PERVASIVE_EXTENSION=	-DSHAPE -DXINPUT -DXKB -DLBX -DXAPPGROUP \
				-DXCSECURITY -DTOGCUP -DXF86BIGFONT \
				-DDPMSExtension -DPIXPRIV -DPANORAMIX \
				-DRENDER -DRANDR -DXFIXES -DDAMAGE \
				-DCOMPOSITE -DXEVIE
X11FLAGS.EXTENSION=	${X11FLAGS.BASE_EXTENSION} \
			${X11FLAGS.PERVASIVE_EXTENSION}

X11FLAGS.DIX=		-DHAVE_DIX_CONFIG_H -D_BSD_SOURCE -DHAS_FCHOWN \
			-DHAS_STICKY_DIR_BIT -D_POSIX_THREAD_SAFE_FUNCTIONS
X11INCS.DIX=		-I${X11INCSDIR}/freetype2  \
			-I${X11INCSDIR}/pixman-1 \
			-I$(X11SRCDIR.xorg-server)/include \
			-I$(X11SRCDIR.xorg-server)/Xext \
			-I$(X11SRCDIR.xorg-server)/composite \
			-I$(X11SRCDIR.xorg-server)/damageext \
			-I$(X11SRCDIR.xorg-server)/xfixes \
			-I$(X11SRCDIR.xorg-server)/Xi \
			-I$(X11SRCDIR.xorg-server)/mi \
			-I$(X11SRCDIR.xorg-server)/miext/shadow \
			-I$(X11SRCDIR.xorg-server)/miext/damage \
			-I$(X11SRCDIR.xorg-server)/render \
			-I$(X11SRCDIR.xorg-server)/randr \
			-I$(X11SRCDIR.xorg-server)/fb
.else
X11FLAGS.EXTENSION=	-DMITMISC -DXTEST -DXTRAP -DXSYNC -DXCMISC -DXRECORD \
			-DMITSHM -DBIGREQS -DXF86MISC -DDBE -DDPMSExtension \
			-DEVI -DSCREENSAVER -DXV -DXVMC -DGLXEXT \
			-DGLX_USE_MESA -DFONTCACHE -DRES
.endif

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
    ${MACHINE} == "ofppc"	|| \
    ${MACHINE} == "sgimips"	|| \
    ${MACHINE} == "sparc64"	|| \
    ${MACHINE} == "sparc"	|| \
    ${MACHINE} == "shark"
#	LOADABLE
X11FLAGS.LOADABLE=	-DXFree86LOADER -DIN_MODULE -DXFree86Module \
			-fno-merge-constants
.endif
  
# XXX FIX ME
.if ${MKXORG} != "no"
XVENDORNAMESHORT=	'"X.Org"'
XVENDORNAME=		'"The X.Org Foundation"'
XORG_RELEASE=		'"Release 1.4.2"'
__XKBDEFRULES__=	'"xorg"'
XLOCALE.DEFINES=	-DXLOCALEDIR=\"${X11LIBDIR}/locale\" \
			-DXLOCALELIBDIR=\"${X11LIBDIR}/locale\"

# XXX oh yeah, fix me later
XORG_VERSION_CURRENT="(((1) * 10000000) + ((4) * 100000) + ((2) * 1000) + 0)"
.endif

PRINT_PACKAGE_VERSION=	awk '/^PACKAGE_VERSION=/ {			\
				match($$1, "([0-9]+\\.)+[0-9]+");	\
				version = substr($$1, RSTART, RLENGTH);	\
			} END { print version }'


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
# X.Org pkgconfig files handling
#
# PKGCONFIG is expected to contain a list of pkgconfig module names.
# They will produce the files <module1>.pc, <module2>.pc, etc, to be
# put in X11USRLIBDIR/pkgconfig.
#
# PKGDIST contains the name of a X11SRCDIR subscript where to find the
# source file for the pkgconfig files.
#
# If PKGDIST is not suitable, a consumer can set PKGDIST.<module> with
# the full path to the source file.
#
# Also, the consumer can use PKGDIST alone, and a PKGCONFIG will be
# derived from it.  Many times, PKGDIST is capitalized and PKGCONFIG is
# the lower case version.
#

.if defined(PKGDIST) && !defined(PKGCONFIG)
PKGCONFIG=	${PKGDIST:tl}
.endif
.if defined(PKGCONFIG)

.include <bsd.files.mk>

_PKGCONFIG_FILES=	${PKGCONFIG:C/$/.pc/}

.PHONY:	pkgconfig-install
pkgconfig-install:

realall:	${_PKGCONFIG_FILES:O:u}
realinstall:	pkgconfig-install

.for _pkg in ${PKGCONFIG:O:u}
PKGDIST.${_pkg}?=	${X11SRCDIR.${PKGDIST:U${_pkg}}}
_PKGDEST.${_pkg}=	${DESTDIR}/${X11USRLIBDIR}/pkgconfig/${_pkg}.pc

.PATH:	${PKGDIST.${_pkg}}

FILESOWN_${_pkg}.pc=	${BINOWN}
FILESGRP_${_pkg}.pc=	${BINGRP}
FILESMODE_${_pkg}.pc=	${NONBINMODE}

${_PKGDEST.${_pkg}}: ${_pkg}.pc __fileinstall
pkgconfig-install: ${_PKGDEST.${_pkg}}
.endfor

# XXX
# The sed script is very, very ugly.  What we actually need is a
# mknative-xorg script that will generate all the .pc files from
# running the autoconfigure script.
# And yes, it has to be splitted in two otherwise it's too long
# for sed to handle.

.SUFFIXES:	.pc.in .pc
.pc.in.pc:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	if [ -n '${PKGCONFIG_VERSION.${.PREFIX}}' ]; then \
		_pkg_version='${PKGCONFIG_VERSION.${.PREFIX}}'; \
	else \
		_pkg_version=$$(${PRINT_PACKAGE_VERSION} \
		    ${PKGDIST.${.PREFIX}}/configure); \
	fi; \
	${TOOL_SED} -E \
		-e "s,@prefix@,${X11ROOTDIR},; \
		s,@INSTALL_DIR@,${X11ROOTDIR},; \
		s,@exec_prefix@,\\$$\{prefix\},; \
		s,@libdir@,\\$$\{prefix\}/lib,; \
		s,@includedir@,\\$$\{prefix\}/include,; \
		s,@datarootdir@,\\$$\{prefix\}/share,; \
		s,@appdefaultdir@,\\$$\{libdir}/X11/app-default,; \
		s,@MAPDIR@,\\$$\{libdir\}/X11/fonts/util,; \
		s,@ICONDIR@,\\$$\{datarootdir\}/icons,; \
		s,@PACKAGE_VERSION@,$${_pkg_version},; \
		s,@VERSION@,$${_pkg_version},; \
		s,@COMPOSITEEXT_VERSION@,$${_pkg_version%.*},; \
		s,@DAMAGEEXT_VERSION@,$${_pkg_version%.*},; \
		s,@FIXESEXT_VERSION@,$${_pkg_version%.*},; \
		s,@RANDR_VERSION@,$${_pkg_version%.*},; \
		s,@RENDER_VERSION@,$${_pkg_version%.*}," \
		-e "s,@moduledir@,\\$$\{libdir\}/modules,; \
		s,@sdkdir@,\\$$\{includedir\}/xorg,; \
		s,@PIXMAN_CFLAGS@,,; \
		s,@LIB_DIR@,/lib,; \
		s,@XKBPROTO_REQUIRES@,kbproto,; \
		s,@FREETYPE_REQUIRES@,freetype2,; \
		s,@EXPAT_LIBS@,-lexpat,; \
		s,@FREETYPE_LIBS@,-lfreetype,; \
		s,@DEP_CFLAGS@,,; \
		s,@DEP_LIBS@,,; \
		s,@X11_EXTRA_DEPS@,,; \
		s,@XTHREAD_CFLAGS@,-D_REENTRANT,; \
		s,@XTHREADLIB@,-lpthread,; \
		s,@fchown_define@,-DHAS_FCHOWN,; \
		s,@sticky_bit_define@,-DHAS_STICKY_DIR_BIT," \
		-e '/^Libs:/ s%-L([^[:space:]]+)%-Wl,-R\1 &%g' \
		< ${.IMPSRC} > ${.TARGET}.tmp && \
	mv -f ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	${_PKGCONFIG_FILES} ${_PKGCONFIG_FILES:C/$/.tmp/}
.endif

#
# APPDEFS (app defaults) handling
#
.if defined(APPDEFS)						# {
appdefsinstall:: .PHONY ${APPDEFS:@S@${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}@}
.PRECIOUS:	${APPDEFS:@S@${DESTDIR}${X11LIBDIR}/app-defaults/${S:T:R}@}

__appdefinstall: .USE
	${_MKTARGET_INSTALL}
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
	    -D__appmansuffix__=1 \
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
