## Makefile.am -- Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src tests doc

EXTRA_HEADERS = src/mpc-log.h
include_HEADERS = src/mpc.h @MPC_LOG_H@
EXTRA_DIST = src/mpc-log.h tests/tgeneric.c Makefile.vc
