# test of the menu system

.if exists(../obj/menuc)
MENUC=MENUDEF=.. ../obj/menuc
.elif exists(../menuc)
MENUC=MENUDEF=.. ../menuc
.else
MENUC=menuc
.endif

PROG=testm

SRCS=menu_defs.c main.c

LDADD=-lcurses -ltermcap

COPTS=-g

NOMAN=y

WARNS=1

CLEANFILES= menu_defs.c menu_defs.h

.include <bsd.prog.mk>

menu_defs.c menu_defs.h: menus.mc
	${MENUC} menus.mc

