#	$NetBSD: Makefile.inc,v 1.3 2022/05/01 23:24:34 uwe Exp $

VERSION_H=	${X11SRCDIR.xterm}/version.h
XTERM_DATE=	${:!${TOOL_GREP} XTERM_DATE ${VERSION_H}!:C/^[^0-9]*//}
XTERM_PATCH=	${:!${TOOL_GREP} XTERM_PATCH ${VERSION_H}!:C/^[^0-9]*//}

X11EXTRAMANDEFS+= \
	-e 's,__app_version__,Patch \#${XTERM_PATCH},g' \
	-e 's,__app_date__,${XTERM_DATE},g' \
	-e 's,__default_termname__,xterm,g' \
	-e 's,__default_termid__,420,g' \
	-e 's,__double_buffer__,False,g' \
	-e 's,__alt_sends_esc__,False,g' \
	-e 's,__meta_sends_esc__,False,g' \
	-e 's,__backarrow_is_bs__,True,g' \
	-e 's,__backarrow_is_erase__,False,g' \
	-e 's,__delete_is_del__,Maybe,g' \
	-e 's,__initial_erase__,False,g' \
	-e 's,__default_class__,XTerm,g' \
	-e 's,__mansuffix__,1,g' \
	-e 's,__pixmapsdir__,${X11INCDIR}/X11/pixmaps,g'
