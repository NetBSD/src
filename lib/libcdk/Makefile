# $NetBSD: Makefile,v 1.1 2001/01/04 20:21:18 garbled Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/cdk
.PATH: ${DIST}/cdk/include
.PATH: ${DIST}/cdk/man

WARNS?=  1
MKLINT= no

LIB=	cdk

SRCS=	alphalist.c binding.c buttonbox.c calendar.c cdk.c cdkscreen.c \
	debug.c dialog.c draw.c entry.c fselect.c graph.c histogram.c \
	itemlist.c label.c marquee.c matrix.c mentry.c menu.c position.c \
	radio.c scale.c scroll.c selection.c slider.c swindow.c template.c \
        viewer.c

INCS=	alphalist.h binding.h buttonbox.h calendar.h cdk.h cdk_config.h \
	cdk_objs.h cdk_util.h cdkscreen.h curdefs.h dialog.h draw.h \
	entry.h fselect.h graph.h histogram.h itemlist.h label.h marquee.h \
	matrix.h mentry.h menu.h radio.h scale.h scroll.h selection.h \
	slider.h swindow.h template.h viewer.h

MAN=	cdk_alphalist.3 cdk_binding.3 cdk_buttonbox.3 cdk_calendar.3 \
	cdk_cdk.3 cdk_dialog.3 cdk_display.3 cdk_entry.3 cdk_fselect.3 \
	cdk_graph.3 cdk_histogram.3 cdk_itemlist.3 cdk_label.3 cdk_marquee.3 \
	cdk_matrix.3 cdk_mentry.3 cdk_menu.3 cdk_misc.3 cdk_radio.3 \
	cdk_scale.3 cdk_screen.3 cdk_scroll.3 cdk_selection.3 cdk_slider.3 \
	cdk_swindow.3 cdk_template.3 cdk_viewer.3

MLINKS=	cdk_cdk.3 cdk.3

INCSDIR= /usr/include/cdk

DPSRCS=	print_version.h

CPPFLAGS+= -I. \
	-I${DIST}/cdk/include \
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
