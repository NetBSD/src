#	$NetBSD: Makefile,v 1.5 2002/02/20 11:30:57 wiz Exp $
#

CPPFLAGS+=-I${.CURDIR}
.if defined(DEBUG_FORMS)
CFLAGS+=-g
CPPFLAGS+=-DDEBUG
.endif
LIB=	form
SRCS=	driver.c field_types.c internals.c field.c form.c post.c type_alnum.c \
        type_alpha.c type_integer.c type_numeric.c type_enum.c type_regex.c \
	type_ipv4.c type_ipv6.c
MAN=	form_cursor.3 form_data.3 form_driver.3 form_field.3 \
	form_field_attributes.3 form_field_buffer.3 form_field_info.3 \
	form_field_just.3 form_field_new.3 form_field_opts.3 \
	form_field_userptr.3 form_field_validation.3 form_fieldtype.3 \
	form_hook.3 form_new.3 form_new_page.3 form_opts.3 form_page.3 \
	form_post.3 form_userptr.3 form_win.3 forms.3
MLINKS+=	forms.3 form.3

INCS=	form.h
INCSDIR=/usr/include
LDFLAGS += -g

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
