#	$NetBSD: Makefile,v 1.1.30.1 2000/11/20 20:05:33 bouyer Exp $

SUBDIR= libtos loadbsd aptck chg_pid rawwrite

.PHONY: all clean install

all:
	@for dir in ${SUBDIR}; do	\
	    ${MAKE} -C $$dir all;		\
	 done

clean:
	@for dir in ${SUBDIR}; do	\
	    ${MAKE} -C $$dir clean;	\
	 done

install:
	@for dir in ${SUBDIR}; do	\
	    ${MAKE} -C $$dir install;	\
	 done
