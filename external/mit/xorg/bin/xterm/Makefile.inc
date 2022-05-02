#	$NetBSD: Makefile.inc,v 1.4 2022/05/02 10:50:02 uwe Exp $

VERSION_H=	${X11SRCDIR.xterm}/version.h
XTERM_DATE=	${:!${TOOL_GREP} XTERM_DATE ${VERSION_H}!:C/^[^0-9]*//}
XTERM_PATCH=	${:!${TOOL_GREP} XTERM_PATCH ${VERSION_H}!:C/^[^0-9]*//}

# my make-fu is too weak to use X11EXTRAMANTRANSFORMS to set
# __app_version__
X11EXTRAMANDEFS+= \
	-e 's,__app_version__,Patch \#${XTERM_PATCH},g' \
	-e 's,__app_date__,${XTERM_DATE},g'


# The standard name for this is __appmansuffix__ but xterm doesn't use it.
X11EXTRAMANTRANSFORMS += __mansuffix__		1

# XXX: FIXME: we do NOT install xpm icons distributed with xterm.
#
# Also, X11INCDIR was IMO not a great choice.  The fact that xpm files
# happen to also be valid C is not reason enough to declare them
# "includes" - this is not the way they are primarily used.
X11EXTRAMANTRANSFORMS += __pixmapsdir__		${X11INCDIR}/X11/pixmaps

# Cf. xsrc/external/mit/xterm/include/xtermcfg.h
X11EXTRAMANTRANSFORMS += __alt_sends_esc__	False
X11EXTRAMANTRANSFORMS += __backarrow_is_bs__	True
X11EXTRAMANTRANSFORMS += __backarrow_is_erase__	False
X11EXTRAMANTRANSFORMS += __default_class__	XTerm
X11EXTRAMANTRANSFORMS += __default_termid__	420
X11EXTRAMANTRANSFORMS += __default_termname__	xterm
X11EXTRAMANTRANSFORMS += __delete_is_del__	Maybe
X11EXTRAMANTRANSFORMS += __double_buffer__	False
X11EXTRAMANTRANSFORMS += __initial_erase__	False
X11EXTRAMANTRANSFORMS += __meta_sends_esc__	False
