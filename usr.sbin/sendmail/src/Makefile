#	$NetBSD: Makefile,v 1.37 1999/02/13 02:55:00 lukem Exp $
#
#  NetBSD Makefile
#
#	@(#)Makefile.NetBSD	8.3 (Berkeley) 9/13/95
#	@Id: Makefile.NetBSD,v 1.3 1994/02/01 05:33:44 glass Exp $
#
#	@(#)Makefile	8.8 (Berkeley) 3/28/97

#########################################################################
#  This Makefile is for 4.4BSD only!!!  For all other systems, use	#
#  the "makesendmail" script.						#
#########################################################################

.include <bsd.own.mk>

PROG=	sendmail

# define the database format to use for aliases et al.  Can be -DNEWDB (for
# the new BSD database package -- this is preferred) or -DNDBM for the NDBM
# database package.  The old putrescent V7 DBM package is no longer
# supported.
# You can define both NEWDB and NDBM during a transition period; old
# databases are read, but the new format will be used on any rebuilds.  On
# really gnarly systems, you can set this to null; it will crawl like a high
# spiral snail, but it will work.
DBMDEF=	-DNEWDB -DNIS

CPPFLAGS+=-I${.CURDIR} ${DBMDEF} -DNETISO -DTCPWRAPPERS
LDADD+=-lwrap
DPADD+=${LIBWRAP}

SRCS=	alias.c arpadate.c clock.c collect.c conf.c convtime.c daemon.c \
	deliver.c domain.c envelope.c err.c headers.c macro.c main.c map.c \
	mci.c mime.c parseaddr.c queue.c readcf.c recipient.c safefile.c \
	savemail.c srvrsmtp.c stab.c stats.c sysexits.c trace.c udb.c \
	usersmtp.c util.c version.c
MAN=	aliases.5 mailq.1 newaliases.1 sendmail.8
# symlinks are now made by the mailwrapper make file
#SYMLINKS=	/usr/sbin/sendmail /usr/bin/newaliases \
#		/usr/sbin/sendmail /usr/bin/mailq
BINDIR=	/usr/libexec/sendmail
BINOWN=	root
BINGRP=	kmem
BINMODE=6555
.if ${MKSHARE} != "no"
FILES=sendmail.hf
FILESDIR=/usr/share/misc
.endif

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/log/sendmail.st

.include <bsd.prog.mk>
