# $NetBSD: Makefile,v 1.3.2.3 1998/12/10 06:59:55 cgd Exp $
#
# XXX this is evil

TC_SRC_DIR?=/usr/local/toolchain
SRC_DIR?=/usr/src

includes lint obj regress tags:

# dependencies should be handled, but aren't for now.
depend:

all: MADE
MADE:
	cd $(TC_SRC_DIR) ; configure
	cd $(TC_SRC_DIR) ; make BISON="yacc"
	touch MADE
clean cleandir distclean::
	cd $(TC_SRC_DIR) ; make -k distclean
	rm -f MADE
install:
	NETBSDSRCDIR=${SRC_DIR}				\
	  sh -e mk-toolchain-install-cmds		\
	  | (cd $(TC_SRC_DIR) ;				\
		STRIP=$(TC_SRC_DIR)/binutils/strip.new	\
		  sh -e -x)
