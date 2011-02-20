# $NetBSD: Makefile,v 1.2 2011/02/20 02:14:42 christos Exp $

.include <bsd.own.mk>

CFG_DIR=/usr/share/examples/libsaslc

CFG_FILE= saslc.conf
CFG_FILE+= README
CFG_MECH=
CFG_MECH+= ANONYMOUS.conf
CFG_MECH+= CRAM-MD5.conf
CFG_MECH+= DIGEST-MD5.conf
CFG_MECH+= EXTERNAL.conf
CFG_MECH+= GSSAPI.conf
CFG_MECH+= LOGIN.conf
CFG_MECH+= PLAIN.conf

install:
	${INSTALL_FILE} -o root -g wheel -m 444 \
	    ${CFG_FILE} ${DESTDIR}${CFG_DIR}
	${INSTALL_FILE} -o root -g wheel -m 444 \
	    ${CFG_MECH} ${DESTDIR}${CFG_DIR}/mech
