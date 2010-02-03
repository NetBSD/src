#	$NetBSD: Makefile,v 1.42 2010/02/03 15:34:38 roy Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	hack
CPPFLAGS+=-I.
DPSRCS+=hack.onames.h hack.h
SRCS+=	alloc.c hack.Decl.c hack.apply.c hack.bones.c hack.c hack.cmd.c \
	hack.do.c hack.do_name.c hack.do_wear.c hack.dog.c hack.eat.c \
	hack.end.c hack.engrave.c hack.fight.c hack.invent.c hack.ioctl.c \
	hack.lev.c hack.main.c hack.makemon.c hack.mhitu.c hack.mklev.c \
	hack.mkmaze.c hack.mkobj.c hack.mkshop.c hack.mon.c hack.monst.c \
	hack.o_init.c hack.objnam.c hack.options.c hack.pager.c hack.potion.c \
	hack.pri.c hack.read.c hack.rip.c hack.rumors.c hack.save.c \
	hack.search.c hack.shk.c hack.shknam.c hack.steal.c hack.terminfo.c \
	hack.timeout.c hack.topl.c hack.track.c hack.trap.c hack.tty.c \
	hack.u_init.c hack.unix.c hack.vault.c hack.version.c hack.wield.c \
	hack.wizard.c hack.worm.c hack.worn.c hack.zap.c rnd.c
MAN=	hack.6
DPADD+=	${LIBTERMINFO}
LDADD+=	-lterminfo
HIDEGAME=hidegame
SETGIDGAME=yes
CLEANFILES+=makedefs makedefs.lo
FILES=help hh data
FILESDIR=/var/games/hackdir

CLEANFILES+=hack.onames.h
hack.onames.h: makedefs def.objects.h
	${_MKTARGET_CREATE}
	./makedefs ${.CURDIR}/def.objects.h > hack.onames.h

makedefs: makedefs.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC}

hack.h:	hack.onames.h

.if defined(HAVE_PCC)
COPTS.hack.bones.c+=	-Wno-shadow
COPTS.hack.do.c+=	-Wno-shadow
.endif

.if !exists(${DESTDIR}${FILESDIR}/perm)
afterinstall: clobber
.endif

clobber: 
	${_MKMSG_INSTALL} ${DESTDIR}/var/games/hackdir/perm
	${INSTALL_FILE} -o games -g games -m 660 /dev/null \
	    ${DESTDIR}/var/games/hackdir/perm
	${_MKMSG_INSTALL} ${DESTDIR}/var/games/hackdir/record
	${INSTALL_FILE} -o games -g games -m 660 /dev/null \
	    ${DESTDIR}/var/games/hackdir/record
	${_MKMSG_REMOVE} "${DESTDIR}/var/games/hackdir/bones*"
	rm -f ${DESTDIR}/var/games/hackdir/bones*

.include <bsd.prog.mk>
