#	$Id: Makefile,v 1.1.1.1 1999/11/23 11:12:34 blymn Exp $
#

CPPFLAGS+=-I${.CURDIR} -DEXPOSE_PRIVATES
LIB=	menu
SRCS=	menu.c item.c userptr.c internals.c driver.c post.c attributes.c
MAN=	menu_attributes.3 menu_item_name.3 menu_items.3 menu_userptr.3 \
        menu_cursor.3 menu_item_new.3 menu_mark.3 menu_win.3 menu_driver.3 \
        menu_item_opts.3 menu_new.3 menus.3 menu_format.3 menu_item_userptr.3 \
        menu_opts.3 menu_hook.3 menu_item_value.3 menu_pattern.3 \
        menu_item_current.3 menu_item_visible.3 menu_post.3
INCS=	menu.h eti.h
INCSDIR=/usr/include
#LDFLAGS += -g

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
