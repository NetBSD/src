#	$NetBSD: Makefile,v 1.13 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	Xt
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

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

INCS=	Composite.h CompositeP.h ConstrainP.h Constraint.h Core.h \
	CoreP.h Intrinsic.h IntrinsicP.h Object.h ObjectP.h RectObj.h \
	RectObjP.h Shell.h ShellP.h StringDefs.h Vendor.h VendorP.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	${X11FLAGS.THREADS}
CPPFLAGS+=	-DUSE_POLL -DUSE_SNPRINTF -DLIBXT -I.

CPPFLAGS.Intrinsic.c=	-DXFILESEARCHPATHDEFAULT=\"${X11LIBDIR}/%L/%T/%N%C%S:${X11LIBDIR}/%l/%T/%N%C%S:${X11LIBDIR}/%T/%N%C%S:${X11LIBDIR}/%L/%T/%N%S:${X11LIBDIR}/%l/%T/%N%S:${X11LIBDIR}/%T/%N%S\"
CPPFLAGS.Error.c=	-DERRORDB=\"${X11LIBDIR}/XtErrorDB\" \
			-DXTERROR_PREFIX=\"\" -DXTWARNING_PREFIX=\"\"
CPPFLAGS.TMkey.c=	-DXKB

LIBDPLIBS=\
	X11	${.CURDIR}/../X11/dynamic \
	SM	${.CURDIR}/../SM \
	ICE	${.CURDIR}/../ICE

.include "${NETBSDSRCDIR}/x11/tools/makestrs/Makefile.makestrs"

STRINGS_FILES=	StringDefs.c StringDefs.h Shell.h
STRINGS_TMPLS=	util/StrDefs.ct util/StrDefs.ht util/Shell.ht
STRINGS_LIST=	util/string.list

${STRINGS_FILES}: strings_files.stamp
strings_files.stamp: ${MAKESTRS} ${STRINGS_LIST} ${STRINGS_TMPLS}
	${_MKMSG_CREATE} ${STRINGS_FILES}
	rm -f ${.TARGET} util ${STRINGS_FILES}
	ln -s ${X11SRCDIR.xc}/lib/${LIB}/util util
	${MAKESTRS} < ${STRINGS_LIST} > StringDefs.c
	touch ${.TARGET}
CLEANFILES+=	${STRINGS_FILES} util strings_files.stamp
DPSRCS+=	${STRINGS_FILES}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
