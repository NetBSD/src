#	MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
#	M68000 Hi-Performance Microprocessor Division
#	M68040 Software Package 
#
#	M68040 Software Package Copyright (c) 1993, 1994 Motorola Inc.
#	All rights reserved.
#
#	THE SOFTWARE is provided on an "AS IS" basis and without warranty.
#	To the maximum extent permitted by applicable law,
#	MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
#	INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
#	PARTICULAR PURPOSE and any warranty against infringement with
#	regard to the SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF)
#	and any accompanying written materials. 
#
#	To the maximum extent permitted by applicable law,
#	IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
#	(INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS
#	PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR
#	OTHER PECUNIARY LOSS) ARISING OF THE USE OR INABILITY TO USE THE
#	SOFTWARE.  Motorola assumes no responsibility for the maintenance
#	and support of the SOFTWARE.  
#
#	You are hereby granted a copyright license to use, modify, and
#	distribute the SOFTWARE so long as this entire notice is retained
#	without alteration in any modified and/or redistributed versions,
#	and that such modified versions are clearly identified as such.
#	No licenses are granted by implication, estoppel or otherwise
#	under any patents or trademarks of Motorola, Inc.

#
#	Makefile 3.3 3/27/91
#
#	Makefile for 68040 Floating Point Software Package
#

TARGET = FPSP

ASM		= ./pasm
ASMOPTS		= +p=68020/68881 +l +C +M
PLINK		= ./plink
PLINKOPTS	= dummy +q$(TARGET) +w32 +x +i +m$(TARGET).linkmap

#
# For the Library Version:
#
AR		= ar
LIB_FILTER 	= sed 's/fpsp.h/l_fpsp.h/'
LIB_TARGET	= libFPSP.a
#
# SYS selects the template set to use
#	templates are supplied for R3V6, CI5 and GEN(generic)
# PREFIX is a string that begins a temporary label in the assembler
#	R3V6 uses 'L%', CI5 likes '.L'
#
#SYS		= R3V6
#PREFIX		= L%%
#
#SYS		= CI5
#PREFIX		= .L
#
SYS		= GEN
PREFIX		= L_

.SUFFIXES:	.s .s~ .sa .sa~ .ro .h .h~

.sa.ro:
	$(ASM) $(ASMOPTS) $*.sa
	
.sa~.ro:
	sccs get SCCS/s.$*.sa
	$(ASM) $(ASMOPTS) $*.sa
	rm -f $*.sa

.sa.s:
	asm2as $*.sa >$*.s

.sa~.sa:
	sccs get SCCS/s.$*.sa

.s~.s:
	sccs get SCCS/s.$*.s

.h~.h:
	sccs get SCCS/s.$*.h

RO_FILES = \
	skeleton.ro \
	bindec.ro \
	binstr.ro \
	decbin.ro \
	do_func.ro \
	gen_except.ro \
	get_op.ro \
	kernel_ex.ro \
	res_func.ro \
	round.ro \
	sacos.ro \
	sasin.ro \
	satan.ro \
	satanh.ro \
	scosh.ro \
	setox.ro \
	sgetem.ro \
	sint.ro \
	slogn.ro \
	slog2.ro \
	smovecr.ro \
	srem_mod.ro \
	scale.ro \
	ssin.ro \
	ssinh.ro \
	stan.ro \
	stanh.ro \
	sto_res.ro \
	stwotox.ro \
	tbldo.ro \
	util.ro \
	x_bsun.ro \
	x_fline.ro \
	x_operr.ro \
	x_ovfl.ro \
	x_snan.ro \
	x_store.ro \
	x_unfl.ro \
	x_unimp.ro \
	x_unsupp.ro \
	bugfix.ro \
	FPSP.ro

LIB_RO_FILES	= \
	l_entry.ro \
	l_do_func.ro \
	l_round.ro \
	l_sacos.ro \
	l_sasin.ro \
	l_satan.ro \
	l_satanh.ro \
	l_scale.ro \
	l_scosh.ro \
	l_setox.ro \
	l_sgetem.ro \
	l_sint.ro \
	l_slog2.ro \
	l_slogn.ro \
	l_srem_mod.ro \
	l_ssin.ro \
	l_ssinh.ro \
	l_stan.ro \
	l_stanh.ro \
	l_stwotox.ro \
	l_support.ro

SA_FILES = \
	skeleton.sa \
	bindec.sa \
	binstr.sa \
	decbin.sa \
	do_func.sa \
	get_op.sa \
	gen_except.sa \
	kernel_ex.sa \
	res_func.sa \
	round.sa \
	sacos.sa \
	sasin.sa \
	satan.sa \
	satanh.sa \
	scosh.sa \
	setox.sa \
	sgetem.sa \
	sint.sa \
	slogn.sa \
	slog2.sa \
	smovecr.sa \
	srem_mod.sa \
	scale.sa \
	ssin.sa \
	ssinh.sa \
	stan.sa \
	stanh.sa \
	sto_res.sa \
	stwotox.sa \
	tbldo.sa \
	util.sa \
	x_bsun.sa \
	x_fline.sa \
	x_operr.sa \
	x_ovfl.sa \
	x_snan.sa \
	x_store.sa \
	x_unfl.sa \
	x_unimp.sa \
	x_unsupp.sa \
	bugfix.sa \
	FPSP.sa

LIB_SA_FILES	= \
	l_entry.sa \
	l_do_func.sa \
	l_round.sa \
	l_sacos.sa \
	l_sasin.sa \
	l_satan.sa \
	l_satanh.sa \
	l_scale.sa \
	l_scosh.sa \
	l_setox.sa \
	l_sgetem.sa \
	l_sint.sa \
	l_slog2.sa \
	l_slogn.sa \
	l_srem_mod.sa \
	l_ssin.sa \
	l_ssinh.sa \
	l_stan.sa \
	l_stanh.sa \
	l_stwotox.sa

H_FILES = \
	fpsp.h \
	l_fpsp.h

U_FILES = \
	unix.s \
	bindec.s \
	binstr.s \
	decbin.s \
	do_func.s \
	get_op.s \
	gen_except.s \
	kernel_ex.s \
	res_func.s \
	round.s \
	sacos.s \
	sasin.s \
	satan.s \
	satanh.s \
	scosh.s \
	setox.s \
	sgetem.s \
	sint.s \
	slogn.s \
	slog2.s \
	smovecr.s \
	srem_mod.s \
	scale.s \
	ssin.s \
	ssinh.s \
	stan.s \
	stanh.s \
	sto_res.s \
	stwotox.s \
	tbldo.s \
	util.s \
	x_bsun.s \
	x_fline.s \
	x_operr.s \
	x_ovfl.s \
	x_snan.s \
	x_store.s \
	x_unfl.s \
	x_unimp.s \
	x_unsupp.s \
	bugfix.s \
	fpsp.mk \
	fpsp.defs \
	l_entry.s \
	l_do_func.s \
	l_round.s \
	l_sacos.s \
	l_sasin.s \
	l_satan.s \
	l_satanh.s \
	l_scale.s \
	l_scosh.s \
	l_setox.s \
	l_sgetem.s \
	l_sint.s \
	l_slog2.s \
	l_slogn.s \
	l_srem_mod.s \
	l_ssin.s \
	l_ssinh.s \
	l_stan.s \
	l_stanh.s \
	l_stwotox.s \
	l_support.s \
	l_fpsp.defs

#
#	Build the target object.  The linkfile is created on the fly.
#	Change the SEG directives to suit your system.
#
$(TARGET).mx:	$(RO_FILES)
	rm -f linkfile
	echo 'IDENT FPSP,2,1,68040 Floating Point Software Package'>linkfile
	echo 'SEG SEG0(G):7  $$00004000' >>linkfile
	echo 'SEG SEG1(G):8  $$000D0000' >>linkfile
	echo 'SEG SEG2(G):15 $$000E0000' >>linkfile
	echo $(RO_FILES) | tr ' ' '\012' | sed 's/^/INPUT /' >>linkfile
	echo 'END' >>linkfile
	$(PLINK) $(PLINKOPTS) <linkfile

#
#	Just about every file needs fpsp.h so:
#
$(RO_FILES):	fpsp.h

fpsp.defs:	fpsp.h
	asm2as fpsp.h >fpsp.defs

unix.s:
	sccs get unix.s

fpsp.mk:
	sccs get fpsp.mk

#
#-----------------------------------------------------------------------
#
#	For making a library version of the FPSP:
#
library:	$(LIB_TARGET)

$(LIB_TARGET):	$(LIB_RO_FILES)
	rm -f $(LIB_TARGET)
	$(AR) crv $(LIB_TARGET) $(LIB_RO_FILES)

$(LIB_RO_FILES): l_fpsp.h

l_fpsp.defs:	l_fpsp.h
	asm2as l_fpsp.h >l_fpsp.defs

#
# The entry points to the library version are created here
# by using two template files an awk script and a list of
# the entry routines for each function.
#
l_entry.sa: L_ENTRY.AWK L_LIST MONADIC.$(SYS) DYADIC.$(SYS) l_fpsp.h
	awk -f L_ENTRY.AWK SYS=$(SYS) PREFIX=$(PREFIX) - <L_LIST|sh>l_entry.sa

#
# Do_func.sa and round.sa need special editing to remove references that
# aren't needed in the library version.  Beware that changes in
# the source code may cause this editing to break....
#
l_do_func.sa: do_func.sa 
	$(LIB_FILTER) do_func.sa >l_do_func.sa
	echo '/xdef.*do_func/,/^	rts/d' >.SCRIPT
	echo 'g/smovcr/d' >>.SCRIPT
	echo 'g/tblpre/d' >>.SCRIPT
	echo 'w' >>.SCRIPT
	echo 'q' >>.SCRIPT
	ed - l_do_func.sa <.SCRIPT
	rm .SCRIPT

l_round.sa: round.sa 
	$(LIB_FILTER) round.sa >l_round.sa
	echo '/^not_E3:/-6,/^not_E3:/d' >.SCRIPT
	echo 'w' >>.SCRIPT
	echo 'q' >>.SCRIPT
	ed - l_round.sa <.SCRIPT
	rm .SCRIPT

l_sacos.sa: sacos.sa 
	$(LIB_FILTER) sacos.sa >l_sacos.sa

l_sasin.sa: sasin.sa 
	$(LIB_FILTER) sasin.sa >l_sasin.sa

l_satan.sa: satan.sa 
	$(LIB_FILTER) satan.sa >l_satan.sa

l_satanh.sa: satanh.sa 
	$(LIB_FILTER) satanh.sa >l_satanh.sa

l_scale.sa: scale.sa 
	$(LIB_FILTER) scale.sa >l_scale.sa

l_scosh.sa: scosh.sa 
	$(LIB_FILTER) scosh.sa >l_scosh.sa

l_setox.sa: setox.sa 
	$(LIB_FILTER) setox.sa >l_setox.sa

l_sgetem.sa: sgetem.sa 
	$(LIB_FILTER) sgetem.sa >l_sgetem.sa

l_sint.sa: sint.sa 
	$(LIB_FILTER) sint.sa >l_sint.sa

l_slog2.sa: slog2.sa 
	$(LIB_FILTER) slog2.sa >l_slog2.sa

l_slogn.sa: slogn.sa 
	$(LIB_FILTER) slogn.sa >l_slogn.sa

l_srem_mod.sa: srem_mod.sa 
	$(LIB_FILTER) srem_mod.sa >l_srem_mod.sa

l_ssin.sa: ssin.sa 
	$(LIB_FILTER) ssin.sa >l_ssin.sa

l_ssinh.sa: ssinh.sa 
	$(LIB_FILTER) ssinh.sa >l_ssinh.sa

l_stan.sa: stan.sa 
	$(LIB_FILTER) stan.sa >l_stan.sa

l_stanh.sa: stanh.sa 
	$(LIB_FILTER) stanh.sa >l_stanh.sa

l_stwotox.sa: stwotox.sa 
	$(LIB_FILTER) stwotox.sa >l_stwotox.sa

#
#	Extract all files from SCCS directory
#
getall:	$(SA_FILES) $(H_FILES)

convert:	$(U_FILES)

clean:
	rm -f $(RO_FILES)
	rm -f $(U_FILES)
	rm -f *.ls linkfile $(TARGET).linkmap
	rm -f FPSP.mx
	rm -f $(LIB_TARGET) .SCRIPT
	rm -f $(LIB_RO_FILES)
	rm -f $(LIB_SA_FILES)

clobber:	clean

