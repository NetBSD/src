#	$NetBSD: Makefile,v 1.21 1997/04/19 07:20:13 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	hack
CFLAGS+=-I.
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
LDADD+=	-ltermcap -lcompat
HIDEGAME=hidegame
CLEANFILES+=hack.onames.h makedefs
FILES=help hh data
FILESDIR=/var/games/hackdir

${PROG}: hack.onames.h

hack.onames.h: makedefs def.objects.h
	${.OBJDIR}/makedefs ${.CURDIR}/def.objects.h > hack.onames.h

makedefs: makedefs.c
	${HOST_CC} ${HOST_CFLAGS} ${HOST_LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${.CURDIR}/${.PREFIX}.c

.if !exists(${DESTDIR}${FILESDIR}/perm)
proginstall:: clobber
.endif

clobber: 
	${INSTALL} -c -o games -g games -m 600 /dev/null \
	    ${DESTDIR}/var/games/hackdir/perm
	${INSTALL} -c -o games -g games -m 600 /dev/null \
	    ${DESTDIR}/var/games/hackdir/record
	rm -f ${DESTDIR}/var/games/hackdir/bones*

.include <bsd.prog.mk>
