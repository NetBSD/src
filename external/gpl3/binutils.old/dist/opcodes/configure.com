$!
$! This file configures the opcodes library for use with openVMS.
$!
$! We do not use the configure script, since we do not have /bin/sh
$! to execute it.
$!
$! Written by Tristan Gingold (gingold@adacore.com)
$!
$ arch=F$GETSYI("ARCH_NAME")
$ arch=F$EDIT(arch,"LOWERCASE")

$!
$ write sys$output "Generate opcodes/build.com"
$!
$ if arch.eqs."ia64"
$ then
$   create build.com
$DECK
$ FILES="ia64-dis,ia64-opc"
$ DEFS="""ARCH_ia64"""
$EOD
$ endif
$ if arch.eqs."alpha"
$ then
$   create build.com
$DECK
$ FILES="alpha-dis,alpha-opc"
$ DEFS="""ARCH_alpha"""
$EOD
$ endif
$!
$ append sys$input build.com
$DECK
$ FILES=FILES + ",dis-init,dis-buf,disassemble"
$ OPT="/noopt/debug"
$ CFLAGS=OPT + "/include=([],""../include"",[-.bfd])/name=(as_is,shortened)" + -
  "/define=(" + DEFS + ")"
$ write sys$output "CFLAGS=",CFLAGS
$ NUM = 0
$ LOOP:
$   F = F$ELEMENT(NUM,",",FILES)
$   IF F.EQS."," THEN GOTO END
$   write sys$output "Compiling ", F, ".c"
$   cc 'CFLAGS 'F.c
$   NUM = NUM + 1
$   GOTO LOOP
$ END:
$ purge
$ lib/create libopcodes 'FILES
$EOD
$exit
