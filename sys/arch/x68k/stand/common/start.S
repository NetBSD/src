|-----------------------------------------------------------
|
|	Simple C runtime startup for Human68k
|
|	  o no stdio support (DOS/IOCS only)
|	  o HUPAIR support
|
|	written by Yasha (ITOH Yasufumi)
|
|	This file is in the public domain
|
|	$NetBSD: start.S,v 1.2 1999/11/11 08:12:50 itohy Exp $

#include <machine/asm.h>

|-----------------------------------------------------------
|
|	configuration
|
#ifndef STACK_SIZE
#define STACK_SIZE	65536	/* stack size in bytes */
#endif

#ifndef STACK_SYMBOL
#ifdef	__ELF__
#define STACK_SYMBOL	_stack	/* stack top symbol name */
#else	/* a.out */
#define STACK_SYMBOL	stack_8K_hUMAn6	/* has largest hash val on NetBSD ld */
#endif					/* and will be at the end of bss */
#endif

#ifndef DUMMY___main
#define DUMMY___main	1	/* define dummy __main() for a.out */
#endif

#ifndef SUPPORT_R_EXEC		/* support  ".r"  relocatable executable */
#define SUPPORT_R_EXEC	0	/* (clear bss, don't use a1 at startup) */
#endif				/* XXX impossible for a.out */

#ifndef SUPPORT_HUPAIR
#define SUPPORT_HUPAIR	1	/* HUPAIR argument interface support */
#endif

#ifndef HUPAIR_ARGV0
#define HUPAIR_ARGV0	1	/* use argv[0] passed in HUPAIR manner */
#endif

#ifndef ADD_PATHNAME
#define ADD_PATHNAME	0	/* add command path to argv[0] if not HUPAIR */
#endif

#ifndef STRICT_SETBLOCK
#define STRICT_SETBLOCK	1	/* free unused memory after creating args */
#endif

#ifndef C_REGPARM
#define C_REGPARM	0	/* main() arguments are passed in registers */
#endif				/* (for  gcc -mregparm) */

#ifndef NEED_MEMCP
#define NEED_MEMCP	0	/* __memcp: MCB address */
#endif
#ifndef NEED_PROCP
#define NEED_PROCP	0	/* __procp: PDB address */
#endif
#ifndef NEED_VERNUM
#define NEED_VERNUM	1	/* __vernum: Human68k version */
#endif
#ifndef NEED_PROGNAME
#define NEED_PROGNAME	1	/* ___progname: program basename */
#endif
#ifndef NEED_ENVIRON
#define NEED_ENVIRON	1	/* _environ: environment vector */
#endif

|-----------------------------------------------------------
|
|	DOS call
|
#define DOS(x)		.word x

#define __FPUTS		0xFF1E
#define __VERNUM	0xFF30
#define __SETBLOCK	0xFF4A
#define __EXIT2		0xFF4C

|	
|	seed to estimate argument string/vector and environment vector size
|	(max nohupair argv[0](92+4) + NULLs(8) + alignment(3))	<- donburi?
|	
#define estimated_argsz	107
#define estimated_com	92	/* estimated command name length (included) */

|
|	other constants
|
#define char_tab	0x09
#define char_space	0x20
#define char_dquote	0x22
#define char_squote	0x27
#define char_slash	0x2f
#define char_backslash	0x5c

#define pdb_mcb		0x10	/* PDB address - MCB address */
#define drvpath_pdb	0x070	/* drive and path address - PDB address */
#define command_pdb	0x0b4	/* command name address - PDB address */
#define top_pdb		0xf0	/* program load address - PDB address */

#define stderr		2	/* stderr  file handle */
#define exit_nomem	127	/* exit status on SETBLOCK failure */

|-----------------------------------------------------------
|
|	execution start
|
|	a0: MCB address, a1: program end + 1,
|	a2: command line, a3: environ, a4: execution start
|
	|.cpu	68000
	.text
	.even

	.globl	_C_LABEL(main)

#ifdef	__ELF__
ASENTRY_NOPROFILE(_start)
#else
ASENTRY_NOPROFILE(start)
#endif
#if SUPPORT_HUPAIR
	.word	0x611e,0x2348,0x5550,0x4149,0x5200
#else
	.word	0x6016
#endif
#if SUPPORT_R_EXEC
	.word	0x7263
#else
	.word	0x7863
#endif
	.long	0x72743020,0x56312E31,0x42206279,0x20596173,0x68610000

|
|	check if hupair
|
#if SUPPORT_HUPAIR
	moveal	%a7@+,%a4
	lea	%a2@(-8),%a6
	moveql	#7,%d3
chkhupair:
	cmpmb	%a6@+,%a4@+
	dbne	%d3,chkhupair
				| d3.l: 0xFFFF: hupair, 0x000x: not hupair
	addqw	#1,%d3
	beqs	ishupair
#endif
	moveql	#char_tab,%d3		| tab (= 9)
ishupair:				| d3.l: 0: hupair, 9: not hupair

|
|	(over)estimate and allocate argument/environ area beforehand
|
	addql	#1,%a2			| skip byte count
	moveql	#estimated_argsz,%d1	| byte counter
	moveal	%a2,%a6
	moveql	#char_space,%d4		| space
acou1:	addql	#1,%d1
	moveb	%a6@+,%d0
	beqs	acou2
	cmpb	%d4,%d0			| space
	beqs	acous
	cmpb	%d3,%d0			| tab (if not hupair)
	bnes	acou1
acous:	addql	#4,%d1			| for argv area
	bras	acou1

acou2:
#if SUPPORT_HUPAIR && HUPAIR_ARGV0
	tstb	%d3
	bnes	anohp
	moveql	#-estimated_com,%d2	| reset argv[0] length
	moveal	%a6,%a4			| preserve argv[0] string address
acouhp:	addql	#1,%d2
	tstb	%a6@+
	bnes	acouhp
	addl	%d2,%d1
anohp:
#endif
					| d1: estimated argument bytes

#if NEED_ENVIRON
	addql	#4,%a3			| skip length field
	moveal	%a3,%a6
ecou1:	addql	#4,%d1
	tstb	%a6@+
	beqs	ecoue
ecou2:	tstb	%a6@+
	bnes	ecou2
	bras	ecou1
ecoue:
#endif
					| d1: estimated byte count

|
|	free memory
|	and ensure the bss/stack (for .r executable) and argument areas valid
|
	lea	%a0@(pdb_mcb),%a5	| a5: PDB address
	subl	%a5,%d1
#if SUPPORT_R_EXEC
#define RELOC(sym, reg)		lea sym+top_pdb,reg; addl %a5,reg
	moveal	%a1,%a6			| end of data
	RELOC(_end, %a1)		| end of bss
#endif
	pea	%a1@(0,%d1:l)		| _end + size - pdb
	movel	%a5,%a7@-
	DOS(__SETBLOCK)
	tstl	%d0
	bpls	sbnoerr

setblock_err:
	movew	#stderr,%a7@
	bsrs	sberr1			| pea %pc@
	.asciz	"setblock failed\r\n"
	.even
sberr1:	DOS(__FPUTS)
	movew	#exit_nomem,%a7@
	DOS(__EXIT2)			| _exit(exit_nomem)

sbnoerr:

|	here, the bss, stack, and argument/environ areas are certainly valid

|
|	set stack
|
	moveal	#STACK_SYMBOL+STACK_SIZE,%a7

#if SUPPORT_R_EXEC
|
|	clear bss section
|
loop_clrbss:
	clrl	%a6@+
	cmpal	%a1,%a6
	bcss	loop_clrbss
#endif

|
|	save MCB address
|
#if NEED_MEMCP
# if SUPPORT_R_EXEC
	RELOC(_C_LABEL(_memcp), %a6)
	movel	%a0,%a6@
# else
	movel	%a0,_C_LABEL(_memcp)
# endif
#endif

|
|	save PDB address
|
#if NEED_PROCP
# if SUPPORT_R_EXEC
	RELOC(_C_LABEL(_procp), %a6)
	movel	%a5,%a6@
# else
	movel	%a5,_C_LABEL(_procp)
# endif
#endif

|
|	get version no of Human
|
#if NEED_VERNUM
	DOS(__VERNUM)
# if SUPPORT_R_EXEC
	RELOC(_C_LABEL(_vernum), %a6)
	movel	%d0,%a6@
# else
	movel	%d0,_C_LABEL(_vernum)
# endif
#endif

|
|	create argv[0]
|
	moveal	%a1,%a0			| top of argument strings
#if SUPPORT_HUPAIR && HUPAIR_ARGV0
	tstb	%d3
	beqs	arg0lp
#endif
#if ADD_PATHNAME
	lea	%a5@(drvpath_pdb),%a4	| drive and path name
arg0path:
	moveb	%a4@+,%a1@+
	bnes	arg0path
	subql	#1,%a1			| remove nul char
#endif
	lea	%a5@(command_pdb),%a4	| command name
arg0lp:	moveb	%a4@+,%a1@+
	bnes	arg0lp

#if NEED_PROGNAME
|
|	find program basename
|
	moveal	%a1,%a4
prognlp:
	cmpal	%a0,%a4
	beqs	prognexit
	moveb	%a4@-,%d0
	cmpib	#char_slash,%d0
	beqs	prognfou
	cmpib	#char_backslash,%d0
	bnes	prognlp
prognfou:
	addql	#1,%a4			| next of slash
prognexit:
# if SUPPORT_R_EXEC
	RELOC(_C_LABEL(__progname), %a6)
	movel	%a4,%a6@
# else
	movel	%a4,_C_LABEL(__progname)
# endif
#endif

|
|	create argument strings
|
	moveql	#1,%d0			| (d0:l) # arg

spskip:	moveb	%a2@+,%d2
	beqs	comline_end
	cmpb	%d4,%d2			| space
	beqs	spskip
	cmpb	%d3,%d2			| tab (if not hupair)
	beqs	spskip

	| create an arg
	clrb	%d1			| no quote here
	addql	#1,%d0			| increment argc

arglp:	tstb	%d1
	bnes	in_quote
	cmpib	#char_dquote,%d2
	beqs	quote
	cmpib	#char_squote,%d2
	bnes	notquote
quote:	moveb	%d2,%d1			| save quote character
	bras	argnextc

in_quote:
	cmpb	%d1,%d2
	bnes	argcopyc
	clrb	%d1			| quote ended
	bras	argnextc

notquote:
	cmpb	%d4,%d2			| space
	beqs	arg_end
	cmpb	%d3,%d2			| tab (if not hupair)
	bnes	argcopyc
arg_end:
	clrb	%a1@+
	bras	spskip

argcopyc:
	moveb	%d2,%a1@+		| copy char

argnextc:
	moveb	%a2@+,%d2
	bnes	arglp
	clrb	%a1@+

comline_end:

|
|	create argv vector
|
	addql	#3,%a1
	movel	%a1,%d1
	andib	#0xfc,%d1		| long alignment
	moveal	%d1,%a1			| argv
	movel	%d0,%d4			| argc
					| a0 is at argument strings
mkargv:
	movel	%a0,%a1@+		| argv[0] ...
nxtarg:	tstb	%a0@+
	bnes	nxtarg
#if STRICT_SETBLOCK
	subqw	#1,%d0
#else
	subqw	#1,%d4
#endif
	bnes	mkargv

	clrl	%a1@+			| argv[argc] should be NULL

|
|	create envp vector
|
#if NEED_ENVIRON
	movel	%a1,%d2
envlp:	tstb	%a3@
	beqs	envend
	movel	%a3,%a1@+
envskp:	tstb	%a3@+
	bnes	envskp
	bras	envlp
envend:	clrl	%a1@+			| NULL termination
# if SUPPORT_R_EXEC
	RELOC(_C_LABEL(environ), %a0)
	movel	%d2,%a0@
# else
	movel	%d2,_C_LABEL(environ)
# endif
#endif

|
|	free unused memory
|
#if STRICT_SETBLOCK
	subal	%a5,%a1
	movel	%a1,%a7@-
	movel	%a5,%a7@-
	DOS(__SETBLOCK)			| reset donburi-kanjo (never fails)
	addql	#8,%a7
	movel	%d4,%d0			| argc
#endif

|
|	make parameter
|
#if NEED_ENVIRON
	movel	%d2,%a7@-		| arg #3 --- envp
#endif
#if !C_REGPARM
	movel	%d1,%a7@-		| arg #2 --- argv
	movel	%d0,%a7@-		| arg #1 --- argc
#endif

#if SUPPORT_R_EXEC
	RELOC(_C_LABEL(main), %a0)
	jsr	%a0@
#else
	jsr	_C_LABEL(main)
#endif

#if !C_REGPARM || NEED_ENVIRON
	movew	%d0,%a7@
#else
	movew	%d0,%a7@-
#endif
	DOS(__EXIT2)

#if !defined(__ELF__) && DUMMY___main
ENTRY_NOPROFILE(__main)
	rts
#endif

|-----------------------------------------------------------
|
|	variables
|
#if NEED_MEMCP
	.comm	_C_LABEL(_memcp),4
#endif

#if NEED_PROCP
	.comm	_C_LABEL(_procp),4		| PDB address
#endif

#if NEED_VERNUM
	.comm	_C_LABEL(_vernum),4
#endif

#if NEED_PROGNAME
	.comm	_C_LABEL(__progname),4
#endif

#if NEED_ENVIRON
	.comm	_C_LABEL(environ),4		| environ address
#endif

|-----------------------------------------------------------
|
|	stack
|
#ifdef	__ELF__
	.section	.stack,"aw",@nobits
	.align	4
STACK_SYMBOL:
	.space	STACK_SIZE
#else
	.comm	STACK_SYMBOL,STACK_SIZE
#endif
