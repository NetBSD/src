#! /usr/bin/awk -f
#
#	create DOS call interface from dos.h
#
#	written by Yasha (ITOH Yasufumi)
#	public domain
#
#	$NetBSD: makedoscalls.awk,v 1.2 1999/11/11 08:14:43 itohy Exp $

BEGIN {
	errno_nomem = 8		# errno for "Cannot allocate memory"
	argsiz["l"] = 4; argsiz["w"] = 2
	argsiz["lb31"] = 4; argsiz["wb8"] = 2; argsiz["wb15"] = 2
	print "#include \"dos_asm.h\""
}

$1 == "/*" && $2 ~ /^ff[0-9a-f][0-9a-f]$/ {
	funcnam=""
	dosno=$2
	ptrval=0
	narg=0
	ncarg=0		# number of 32bit C function argument
	argbyte=0
	opt_e=0
	e_strict=0
	e_alloc=0
	e_proc=0
	svreg=0
	noret=0
	super=0
	super_jsr=0
	alias=""
	for (i = 3; i <= NF && $i != "*/" && $i != ";"; i++) {
		arg[narg] = $i
		narg++
		if (argsiz[$i])
			ncarg++
	}
	if ($i == ";") {
		# process opts
		for (i++; i <= NF && $i != "*/"; i++) {
			if ($i == "e")
				opt_e = 1
			else if ($i == "estrct") {
				opt_e = 1
				e_strict = 1
			} else if ($i == "ep") {
				opt_e = 1
				e_proc = 1
			} else if ($i == "ealloc") {
				opt_e = 1
				e_alloc = 1
			} else if ($i == "sv")
				svreg = 1
			else if ($i == "noret")
				noret = 1
			else if ($i == "alias") {
				i++
				alias = $i
			} else if ($i == "super")
				super = 1
			else if ($i == "super_jsr")
				super_jsr = 1
			else {
				print FILENAME ":" NR ": unknown opt", $i
				exit(1)
			}
		}
	}
	if ($i != "*/") {
		print FILENAME ":" NR ": malformed input line:" $0
		exit(1)
	}
	# find out func name
	printf "|"
	for (i++; i <= NF; i++) {
		printf " %s", $i
		if ($i ~ /^\**DOS_[A-Z0-9_]*$/) {
			funcnam = $i
			while (funcnam ~ /^\*/) {
				funcnam = substr(funcnam, 2, length(funcnam) -1)
				ptrval = 1
			}
		}
	}
	print ""
	if (!funcnam) {
		print FILENAME ":" NR ": can't find function name"
		exit(1)
	}

	# output assembly code
	print "ENTRY_NOPROFILE(" funcnam ")"
	if (alias) {
		print "GLOBAL(" alias ")"
	}
	if (svreg)	print "\tmoveml\t%d2-%d7/%a2-%a6,%sp@-"

	# PUSH ARGS
	argoff = ncarg * 4
	if (svreg)
		argoff += 4 * 11
	for (i = narg - 1; i >= 0; i--) {
		a = arg[i]
		asz = argsiz[a]
		if (asz) {
			if (a == "l") {
				# optimize with movem
				if (arg[i-1] == "l" && arg[i-2] == "l") {
					if (arg[i-3] == "l") {
						print "\tmoveml\t%sp@(" argoff - 12 "),%d0-%d1/%a0-%a1"
						print "\tmoveml\t%d0-%d1/%a0-%a1,%sp@-"
						asz = 16
						i -= 3
					} else if (arg[i-3] == "w") {
						print "\tmoveml\t%sp@(" argoff - 12 "),%d0-%d1/%a0-%a1"
						print "\tmoveml\t%d1/%a0-%a1,%sp@-"
						print "\tmovew\t%d0,%sp@-"
						asz = 14
						i -= 3
					} else {
						print "\tmoveml\t%sp@(" argoff - 8 "),%d0-%d1/%a0"
						print "\tmoveml\t%d0-%d1/%a0,%sp@-"
						asz = 12
						i -= 2
					}
				} else {
					print "\tmovel\t%sp@(" argoff "),%sp@-"
				}
			} else if (a == "w")
				print "\tmovew\t%sp@(" argoff + 2 "),%sp@-"
			else if (a == "lb31") {
				print "\tmovel\t%sp@(" argoff "),%d0"
				print "\tbset\t#31,%d0"
				print "\tmovel\t%d0,%sp@-"
			} else if (a == "wb8") {
				print "\tmovew\t%sp@(" argoff + 2 "),%d0"
				print "\torw\t#0x100,%d0"
				print "\tmovew\t%d0,%sp@-"
			} else if (a == "wb15") {
				print "\tmovew\t%sp@(" argoff + 2 "),%d0"
				print "\torw\t#0x8000,%d0"
				print "\tmovew\t%d0,%sp@-"
			} else {
				print "??? unknown type"
				exit(1)
			}

			if (asz == 2)
				argoff -= 2
		} else if (a ~ /^[0-9][0-9]*\.w$/) {
			asz = 2
			argoff += 2
			val = substr(a, 1, length(a) - 2)
			if (val == 0)
				print "\tclrw\t%sp@-"
			else
				print "\tmovew\t#" val ",%sp@-"
		} else if (a ~ /^[0-9][0-9]*\.l$/) {
			asz = 4;
			argoff += 4
			val = substr(a, 1, length(a) - 2)
			if (val == 0)
				print "\tclrl\t%sp@-"
			else if (val <= 32767)
				print "\tpea\t" val ":w"
			else
				print "\tmovel\t#" val ",%sp@-"
		} else if (a == "drvctrl" && narg == 1) {
			# only for DOS_DRVCTRL
			asz = 2
			print "\tmoveb\t%sp@(7),%d0"
			print "\tlslw\t#8,%d0"
			print "\tmoveb\t%sp@(11),%d0"
			print "\tmovew\t%d0,%sp@-"
		} else if (a == "super" && narg == 1) {
			# only for DOS_SUPER
			print "\tmoveal\t%sp@+,%a1"
		} else {
			print FILENAME ":" NR ": unknown arg type:", a
			exit(1)
		}
		argbyte += asz
	}

	if (super_jsr) {
		print "\tmoveal\t%sp@(" argoff + 8 "),%a0	| inregs"
		print "\tmoveml\t%a0@,%d0-%d7/%a0-%a6"
	}

	if (dosno ~ /^ff[8a]./) {
		if (dosno ~ /^..8./)
			v2dosno = "ff5" substr(dosno, 4, 1)
		else
			v2dosno = "ff7" substr(dosno, 4, 1)
		print "\tcmpiw	#0x200+14,_C_LABEL(_vernum)+2	| 2.14"
#		print "\tbcss\tLv2doscall"
		print "\tbcss\t2f"
		print "\t.word\t0x" dosno
		if (!noret)
#			print "\tbras\tLedoscall"
			print "\tbras\t3f"
#		print "Lv2doscall:"
		print "2:"
		print "\t.word\t0x" v2dosno
		if (!noret)
#			print "Ledoscall:"
			print "3:"
	} else {
		print "\t.word\t0x" dosno
	}

	# no postprocess needed for dead calls
	if (noret)
		next

	if (super_jsr) {
		print "\tmovel\t%a6,%sp@"
		print "\tmoveal\t%sp@(" argoff + 12 "),%a6	| outregs"
		print "\tmovel\t%sp@+,%a6@(" 4 * 14 ")"
		print "\tmoveml\t%d0-%d7/%a0-%a5,%a6@"
	} else if (argbyte > 0) {
		# POP ARGS
		if (argbyte <= 8)
			print "\taddql\t#" argbyte ",%sp"
		else
			print "\tlea\t%sp@(" argbyte "),%sp"
	}

	if (svreg)	print "\tmoveml\t%sp@+,%d2-%d7/%a2-%a6"
	if (opt_e) {
		if (e_strict) {
			print "\tcmpil\t#0xffffff00,%d0"
			print "\tbcc\tCERROR"
		} else  {
			print "\ttstl\t%d0"
			if (super) {
#				print "\tbpls\tLnoerr"
				print "\tbpls\t5f"
				print "\tnegl\t%d0"
				print "\tmovel\t%d0,_C_LABEL(dos_errno)"
				print "\tnegl\t%d0"
#				print "Lnoerr:"
				print "5:"
			} else if (e_alloc) {
#				print "\tbpls\tLnoerr"
				print "\tbpls\t5f"
				print "\tmovel\t#" errno_nomem ",_C_LABEL(dos_errno)"
#				print "Lnoerr:"
				print "5:"
			} else if (e_proc) {
				print "\tbmi\tPRCERROR"
			} else
				print "\tbmi\tCERROR"
		}
	}
	if (super)
		print "\tjmp\t%a1@"
	else {
		if (ptrval)
			print "#ifdef __SVR4_ABI__\n\tmoveal\t%d0,%a0\n#endif"
		print "\trts"
	}
}
