#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:05 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xt
.PATH:	${X11SRCDIR.${LIB}}/include/X11
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}

SRCS=	ActionHook.c Alloc.c ArgList.c Callback.c ClickTime.c \
	Composite.c Constraint.c Convert.c Converters.c Core.c \
	Create.c Destroy.c Display.c Error.c Event.c EventUtil.c \
	Functions.c GCManager.c Geometry.c GetActKey.c GetResList.c \
	GetValues.c HookObj.c Hooks.c Initialize.c Intrinsic.c \
	Keyboard.c Manage.c NextEvent.c Object.c PassivGrab.c \
	Pointer.c Popup.c PopupCB.c RectObj.c ResConfig.c Resources.c \
	Selection.c SetSens.c SetValues.c SetWMCW.c Shell.c \
	StringDefs.c TMaction.c TMgrab.c TMkey.c TMparse.c TMprint.c \
	TMstate.c Threads.c VarCreate.c VarGet.c Varargs.c Vendor.c

INCS=	CallbackI.h Composite.h CompositeP.h ConstrainP.h Constraint.h \
	ConvertI.h Core.h CoreP.h CreateI.h EventI.h HookObjI.h InitialI.h \
	Intrinsic.h IntrinsicI.h IntrinsicP.h Object.h ObjectP.h \
	PassivGraI.h RectObj.h RectObjP.h ResConfigP.h ResourceI.h \
	SelectionI.h Shell.h ShellI.h ShellP.h StringDefs.h ThreadsI.h \
	TranslateI.h VarargsI.h Vendor.h VendorP.h Xtos.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-DUSE_POLL -DUSE_SNPRINTF -DLIBXT -I.

CPPFLAGS.Intrinsic.c=	-DXFILESEARCHPATHDEFAULT=\"${X11LIBDIR}/%L/%T/%N%C%S:${X11LIBDIR}/%l/%T/%N%C%S:${X11LIBDIR}/%T/%N%C%S:${X11LIBDIR}/%L/%T/%N%S:${X11LIBDIR}/%l/%T/%N%S:${X11LIBDIR}/%T/%N%S\"
CPPFLAGS.Error.c=	-DERRORDB=\"${X11LIBDIR}/XtErrorDB\" \
			-DXTERROR_PREFIX=\"\" -DXTWARNING_PREFIX=\"\"
CPPFLAGS.TMkey.c=	-DXKB

LIBDPLIBS=\
	X11	${.CURDIR}/../libX11/dynamic \
	SM	${.CURDIR}/../libSM \
	ICE	${.CURDIR}/../libICE

.include "${NETBSDSRCDIR}/external/mit/xorg/tools/makestrs/Makefile.makestrs"

STRINGS_FILES=	StringDefs.c StringDefs.h Shell.h
STRINGS_TMPLS=	util/StrDefs.ct util/StrDefs.ht util/Shell.ht
STRINGS_LIST=	util/string.list

${STRINGS_FILES}: strings_files.stamp
strings_files.stamp: ${MAKESTRS} ${STRINGS_LIST} ${STRINGS_TMPLS}
	${_MKMSG_CREATE} ${STRINGS_FILES}
	rm -f ${.TARGET} ${STRINGS_FILES}
	rm -f util
	ln -s ${X11SRCDIR.Xt}/util util
	${MAKESTRS} -i . < ${STRINGS_LIST} > StringDefs.c
	touch ${.TARGET}
CLEANFILES+=	${STRINGS_FILES} util strings_files.stamp
DPSRCS+=	${STRINGS_FILES}

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
