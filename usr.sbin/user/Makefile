# $NetBSD: Makefile,v 1.5 1999/12/08 00:01:36 soren Exp $
#
.include <bsd.own.mk>

CPPFLAGS+= -DEXTENSIONS

SKEL_DIR=	/etc/skel
EXAMPLE_DIR=	${DESTDIR}/usr/share/examples/usermgmt

PROG= user
LINKS+=	${BINDIR}/user ${BINDIR}/useradd
LINKS+=	${BINDIR}/user ${BINDIR}/userdel
LINKS+=	${BINDIR}/user ${BINDIR}/usermod
LINKS+=	${BINDIR}/user ${BINDIR}/group
LINKS+=	${BINDIR}/user ${BINDIR}/groupadd
LINKS+=	${BINDIR}/user ${BINDIR}/groupdel
LINKS+=	${BINDIR}/user ${BINDIR}/groupmod
LDADD+= -lutil 
DPADD+= ${LIBUTIL}
MAN= user.8 useradd.8 userdel.8 usermod.8 group.8 groupadd.8 groupdel.8 groupmod.8
MLINKS=	useradd.8 adduser.8

.if ${MKSHARE} != "no"
FILES=			dot.cshrc dot.login dot.profile
FILESNAME_dot.cshrc=	.cshrc
FILESNAME_dot.login=	.login
FILESNAME_dot.profile=	.profile
FILESDIR=		/usr/share/examples/usermgmt
.endif

.include <bsd.prog.mk>

test: ${PROG}
	@echo "No news is good news"
	@echo "1. Adding new user"
	./${PROG} add -m -g=uid test1.1
	@echo "2. Modifying new user"
	./${PROG} mod -l test1.2 test1.1
	@echo "3. Deleting new user"
	./${PROG} del -r test1.2
	@echo "4. Attempting to add an invalid user name - IGNORE ANY ERROR"
	-./${PROG} add -m test1%1
	@echo "5. Bad usage - IGNORE ANY ERROR"
	-./${PROG} add -m
	@echo "All tests completed"
