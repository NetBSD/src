# $NetBSD: Makefile,v 1.6 1999/12/24 09:08:48 agc Exp $
#
.include <bsd.own.mk>

CPPFLAGS+= -DEXTENSIONS
WARNS= 1

SKEL_DIR=	/etc/skel
EXAMPLE_DIR=	${DESTDIR}/usr/share/examples/usermgmt

PROG= user
SRCS+= user.c main.c
LINKS+=	${BINDIR}/user ${BINDIR}/useradd
LINKS+=	${BINDIR}/user ${BINDIR}/userdel
LINKS+=	${BINDIR}/user ${BINDIR}/usermod
LINKS+=	${BINDIR}/user ${BINDIR}/group
LINKS+=	${BINDIR}/user ${BINDIR}/groupadd
LINKS+=	${BINDIR}/user ${BINDIR}/groupdel
LINKS+=	${BINDIR}/user ${BINDIR}/groupmod
LINKS+=	${BINDIR}/user ${BINDIR}/userinfo
LINKS+=	${BINDIR}/user ${BINDIR}/groupinfo
LDADD+= -lutil 
DPADD+= ${LIBUTIL}
MAN= user.8 useradd.8 userdel.8 usermod.8 userinfo.8
MAN+= group.8 groupadd.8 groupdel.8 groupmod.8 groupinfo.8
MLINKS=	useradd.8 adduser.8

.if ${MKSHARE} != "no"
FILES=			dot.cshrc dot.login dot.profile
FILESNAME_dot.cshrc=	.cshrc
FILESNAME_dot.login=	.login
FILESNAME_dot.profile=	.profile
FILESDIR=		/usr/share/examples/usermgmt
.endif

# this target checks the built-in default group, and, if it doesn't exist,
# creates it
default-group:
	@ln -fs ${.OBJDIR}/user ${.OBJDIR}/group;			\
	defgrp=`${.OBJDIR}/user add -D | awk '/^group/ { print $$2 }'`;	\
	if ${.OBJDIR}/group info -e $$defgrp; then			\
		defgid=`${.OBJDIR}/group info $$defgrp | awk '/^gid/ { print $$2 }'`; \
	else								\
		defgid=99;						\
		while [ $$defgid -gt 0 ]; do				\
			${.OBJDIR}/group info -e $$defgid || break;	\
			defgid=`expr $$defgid - 1`;			\
		done;							\
		if [ $$defgid -eq 0 ]; then				\
			defgid=100;					\
			while [ $$defgid -lt 60000 ]; do		\
				${.OBJDIR}/group info -e $$defgid || break; \
				defgid=`expr $$defgid + 1`;		\
			done;						\
			if [ $$defgid -eq 60000 ]; then			\
				echo "No gids left";			\
				exit 1;					\
			fi;						\
		fi;							\
		${.OBJDIR}/group add -g $$defgid $$defgrp;		\
	fi;								\
	echo "Default group is $$defgrp ($$defgid):";			\
	${.OBJDIR}/group info $$defgrp

.include <bsd.prog.mk>

test: ${PROG}
	@echo "No news is good news"
	@echo "1. Adding new user"
	@rm -f useradd
	@ln -s user useradd
	-./useradd -m -g=uid test1.1
	@echo "2. Modifying new user"
	-./${PROG} mod -l test1.2 test1.1
	@echo "3. Deleting new user"
	-./${PROG} del -r test1.2
	@echo "4. Attempting to add an invalid user name - IGNORE ANY ERROR"
	-./${PROG} add -m test1%1
	@echo "5. Bad usage - IGNORE ANY ERROR"
	-./${PROG} add -m
	@echo "6. Set range defaults"
	-./${PROG} add -D -r4000..6000
	-./${PROG} add -D
	@echo "7. Get user information"
	-./${PROG} info root
	@echo "8. Bad user name - IGNORE ANY ERROR"
	-./${PROG} info test1%1 || echo "User not found"
	@echo "All tests completed"
