# $NetBSD: Makefile.inc,v 1.1.32.1 2013/12/18 18:43:29 matt Exp $

USE_LIBEDIT=no
USE_TUI=yes

GDB_MACHINE_ARCH=${MACHINE_ARCH:S/armv7/arm/}
