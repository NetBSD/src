#	$NetBSD: Makefile,v 1.32 2002/02/09 09:24:05 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	hack
CPPFLAGS+=-I.
SRCS+=	hack.onames.h
SRCS+=	alloc.c hack.Decl.c hack.apply.c hack.bones.c hack.c hack.cmd.c \
	hack.do.c hack.do_name.c hack.do_wear.c hack.dog.c hack.eat.c \
	hack.end.c hack.engrave.c hack.fight.c hack.invent.c hack.ioctl.c \
	hack.lev.c hack.main.c hack.makemon.c hack.mhitu.c hack.mklev.c \
	hack.mkmaze.c hack.mkobj.c hack.mkshop.c hack.mon.c hack.monst.c \
	hack.o_init.c hack.objnam.c hack.options.c hack.pager.c hack.potion.c \
	hack.pri.c hack.read.c hack.rip.c hack.rumors.c hack.save.c \
	hack.search.c hack.shk.c hack.shknam.c hack.steal.c hack.termcap.c \
	hack.timeout.c hack.topl.c hack.track.c hack.trap.c hack.tty.c \
	hack.u_init.c hack.unix.c hack.vault.c hack.version.c hack.wield.c \
	hack.wizard.c hack.worm.c hack.worn.c hack.zap.c rnd.c
MAN=	hack.6
DPADD+=	${LIBTERM} ${LIBCOMPAT}
LDADD+=	-ltermcap
HIDEGAME=hidegame
SETGIDGAME=yes
CLEANFILES+=hack.onames.h makedefs
FILES=help hh data
FILESDIR=/var/games/hackdir

hack.onames.h: makedefs def.objects.h
	${.OBJDIR}/makedefs ${.CURDIR}/def.objects.h > hack.onames.h

makedefs: makedefs.c
	${HOST_LINK.c} -o makedefs ${.CURDIR}/makedefs.c

hack.h:	hack.onames.h

alloc.o hack.Decl.o hack.apply.o hack.bones.o hack.o hack.cmd.o hack.do.o \
    hack.do_name.o hack.do_wear.o hack.dog.o hack.eat.o hack.end.o \
    hack.engrave.o hack.fight.o hack.invent.o hack.ioctl.o hack.lev.o \
    hack.main.o hack.makemon.o hack.mhitu.o hack.mklev.o hack.mkmaze.o \
    hack.mkobj.o hack.mkshop.o hack.mon.o hack.monst.o hack.o_init.o \
    hack.objnam.o hack.options.o hack.pager.o hack.potion.o hack.pri.o \
    hack.read.o hack.rip.o hack.rumors.o hack.save.o hack.search.o hack.shk.o \
    hack.shknam.o hack.steal.o hack.termcap.o hack.timeout.o hack.topl.o \
    hack.track.o hack.trap.o hack.tty.o hack.u_init.o hack.unix.o hack.vault.o \
    hack.version.o hack.wield.o hack.wizard.o hack.worm.o hack.worn.o \
    hack.zap.o rnd.o: hack.h

.if !exists(${DESTDIR}${FILESDIR}/perm)
afterinstall: clobber
.endif

clobber: 
	${INSTALL_FILE} -o games -g games -m 660 /dev/null \
	    ${DESTDIR}/var/games/hackdir/perm
	${INSTALL_FILE} -o games -g games -m 660 /dev/null \
	    ${DESTDIR}/var/games/hackdir/record
	rm -f ${DESTDIR}/var/games/hackdir/bones*

.include <bsd.prog.mk>
