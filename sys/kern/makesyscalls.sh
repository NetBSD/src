#	$NetBSD: makesyscalls.sh,v 1.125.2.2 2013/02/25 00:29:52 tls Exp $
#
# Copyright (c) 1994, 1996, 2000 Christopher G. Demetriou
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed for the NetBSD Project
#      by Christopher G. Demetriou.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#	@(#)makesyscalls.sh	8.1 (Berkeley) 6/10/93

set -e

case $# in
    2)	;;
    *)	echo "Usage: $0 config-file input-file" 1>&2
	exit 1
	;;
esac

# the config file sets the following variables:
#	sysalign	check for alignment of off_t
#	sysnames	the syscall names file
#	sysnumhdr	the syscall numbers file
#	syssw		the syscall switch file
#	sysarghdr	the syscall argument struct definitions
#	compatopts	those syscall types that are for 'compat' syscalls
#	switchname	the name for the 'struct sysent' we define
#	namesname	the name for the 'const char *[]' we define
#	constprefix	the prefix for the system call constants
#	registertype	the type for register_t
#	nsysent		the size of the sysent table
#	sys_nosys	[optional] name of function called for unsupported
#			syscalls, if not sys_nosys()
#       maxsysargs	[optiona] the maximum number or arguments
#
# NOTE THAT THIS makesyscalls.sh DOES NOT SUPPORT 'SYSLIBCOMPAT'.

# source the config file.
sys_nosys="sys_nosys"	# default is sys_nosys(), if not specified otherwise
maxsysargs=8		# default limit is 8 (32bit) arguments
rumpcalls="/dev/null"
rumpcallshdr="/dev/null"
rumpsysent="rumpsysent.tmp"
. ./$1

# tmp files:
sysdcl="sysent.dcl"
sysprotos="sys.protos"
syscompat_pref="sysent."
sysent="sysent.switch"
sysnamesbottom="sysnames.bottom"
rumptypes="rumphdr.types"
rumpprotos="rumphdr.protos"

trap "rm $sysdcl $sysprotos $sysent $sysnamesbottom $rumpsysent $rumptypes $rumpprotos" 0

# Awk program (must support nawk extensions)
# Use "awk" at Berkeley, "nawk" or "gawk" elsewhere.
awk=${AWK:-awk}

# Does this awk have a "toupper" function?
have_toupper=`$awk 'BEGIN { print toupper("true"); exit; }' 2>/dev/null`

# If this awk does not define "toupper" then define our own.
if [ "$have_toupper" = TRUE ] ; then
	# Used awk (GNU awk or nawk) provides it
	toupper=
else
	# Provide our own toupper()
	toupper='
function toupper(str) {
	_toupper_cmd = "echo "str" |tr a-z A-Z"
	_toupper_cmd | getline _toupper_str;
	close(_toupper_cmd);
	return _toupper_str;
}'
fi

# before handing it off to awk, make a few adjustments:
#	(1) insert spaces around {, }, (, ), *, and commas.
#	(2) get rid of any and all dollar signs (so that rcs id use safe)
#
# The awk script will deal with blank lines and lines that
# start with the comment character (';').

sed -e '
s/\$//g
:join
	/\\$/{a\

	N
	s/\\\n//
	b join
	}
2,${
	/^#/!s/\([{}()*,|]\)/ \1 /g
}
' < $2 | $awk "
$toupper
BEGIN {
	# Create a NetBSD tag that does not get expanded when checking
	# this script out of CVS.  (This part of the awk script is in a
	# shell double-quoted string, so the backslashes are eaten by
	# the shell.)
	tag = \"\$\" \"NetBSD\" \"\$\"

	# to allow nested #if/#else/#endif sets
	savedepth = 0
	# to track already processed syscalls

	sysnames = \"$sysnames\"
	sysprotos = \"$sysprotos\"
	sysnumhdr = \"$sysnumhdr\"
	sysarghdr = \"$sysarghdr\"
	sysarghdrextra = \"$sysarghdrextra\"
	rumpcalls = \"$rumpcalls\"
	rumpcallshdr = \"$rumpcallshdr\"
	rumpsysent = \"$rumpsysent\"
	switchname = \"$switchname\"
	namesname = \"$namesname\"
	constprefix = \"$constprefix\"
	registertype = \"$registertype\"
	sysalign=\"$sysalign\"
	if (!registertype) {
	    registertype = \"register_t\"
	}
	nsysent = \"$nsysent\"

	sysdcl = \"$sysdcl\"
	syscompat_pref = \"$syscompat_pref\"
	sysent = \"$sysent\"
	sysnamesbottom = \"$sysnamesbottom\"
	rumpprotos = \"$rumpprotos\"
	rumptypes = \"$rumptypes\"
	sys_nosys = \"$sys_nosys\"
	maxsysargs = \"$maxsysargs\"
	infile = \"$2\"

	compatopts = \"$compatopts\"
	"'

	if (rumpcalls != "/dev/null")
		haverumpcalls = 1

	printf "/* %s */\n\n", tag > sysdcl
	printf "/*\n * System call switch table.\n *\n" > sysdcl
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysdcl

	ncompat = split(compatopts,compat)
	for (i = 1; i <= ncompat; i++) {
		compat_upper[i] = toupper(compat[i])

		printf "\n#ifdef %s\n", compat_upper[i] > sysent
		printf "#define	%s(func) __CONCAT(%s_,func)\n", compat[i], \
		    compat[i] > sysent
		printf "#else\n" > sysent
		printf "#define	%s(func) %s\n", compat[i], sys_nosys > sysent
		printf "#endif\n" > sysent
	}

	printf "\n#define\ts(type)\tsizeof(type)\n" > sysent
	printf "#define\tn(type)\t(sizeof(type)/sizeof (%s))\n", registertype > sysent
	printf "#define\tns(type)\tn(type), s(type)\n\n", registertype > sysent
	printf "struct sysent %s[] = {\n",switchname > sysent

	printf "/* %s */\n\n", tag > sysnames
	printf "/*\n * System call names.\n *\n" > sysnames
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysnames

	printf "\n/*\n * System call prototypes.\n */\n\n" > sysprotos
	if (haverumpcalls)
		printf("#ifndef RUMP_CLIENT\n") > sysprotos

	printf "/* %s */\n\n", tag > sysnumhdr
	printf "/*\n * System call numbers.\n *\n" > sysnumhdr
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysnumhdr

	printf "/* %s */\n\n", tag > sysarghdr
	printf "/*\n * System call argument lists.\n *\n" > sysarghdr
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > sysarghdr

	printf "/* %s */\n\n", tag > rumpcalls
	printf "/*\n * System call vector and marshalling for rump.\n *\n" > rumpcalls
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > rumpcalls

	printf "/* %s */\n\n", tag > rumpcallshdr
	printf "/*\n * System call protos in rump namespace.\n *\n" > rumpcallshdr
	printf " * DO NOT EDIT-- this file is automatically generated.\n" > rumpcallshdr
}
NR == 1 {
	sub(/ $/, "")
	printf " * created from%s\n */\n\n", $0 > sysdcl
	printf "#include <sys/cdefs.h>\n__KERNEL_RCSID(0, \"%s\");\n\n", tag > sysdcl

	printf " * created from%s\n */\n\n", $0 > sysnames
	printf "#include <sys/cdefs.h>\n__KERNEL_RCSID(0, \"%s\");\n\n", tag > sysnames

	printf " * created from%s\n */\n\n", $0 > rumpcalls
	printf "#ifdef RUMP_CLIENT\n" > rumpcalls
	printf "#include \"rumpuser_port.h\"\n" > rumpcalls
	printf "#endif /* RUMP_CLIENT */\n\n" > rumpcalls
	printf "#include <sys/param.h>\n\n" > rumpcalls
	printf "#ifdef __NetBSD__\n" > rumpcalls
	printf "#include <sys/cdefs.h>\n__KERNEL_RCSID(0, \"%s\");\n\n", tag > rumpcalls

	printf "#include <sys/fstypes.h>\n" > rumpcalls
	printf "#include <sys/proc.h>\n" > rumpcalls
	printf "#endif /* __NetBSD__ */\n\n" > rumpcalls
	printf "#ifdef RUMP_CLIENT\n" > rumpcalls
	printf "#include <errno.h>\n" > rumpcalls
	printf "#include <stdint.h>\n" > rumpcalls
	printf "#include <stdlib.h>\n\n" > rumpcalls
	printf "#include <srcsys/syscall.h>\n" > rumpcalls
	printf "#include <srcsys/syscallargs.h>\n\n" > rumpcalls
	printf "#include <rump/rumpclient.h>\n\n" > rumpcalls
	printf "#define rsys_syscall(num, data, dlen, retval)\t\\\n" > rumpcalls
	printf "    rumpclient_syscall(num, data, dlen, retval)\n" > rumpcalls
	printf "#define rsys_seterrno(error) errno = error\n" > rumpcalls
	printf "#define rsys_alias(a,b)\n#else\n" > rumpcalls
	printf "#include <sys/syscall.h>\n" > rumpcalls
	printf "#include <sys/syscallargs.h>\n\n" > rumpcalls
	printf "#include <sys/syscallvar.h>\n\n" > rumpcalls
	printf "#include <rump/rumpuser.h>\n" > rumpcalls
	printf "#include \"rump_private.h\"\n\n" > rumpcalls
	printf "static int\nrsys_syscall" > rumpcalls
	printf "(int num, void *data, size_t dlen, register_t *retval)" > rumpcalls
	printf "\n{\n\tstruct sysent *callp = rump_sysent + num;\n" > rumpcalls
	printf "\tint rv;\n" > rumpcalls
	printf "\n\tKASSERT(num > 0 && num < SYS_NSYSENT);\n\n" > rumpcalls
	printf "\trump_schedule();\n" > rumpcalls
	printf "\trv = sy_call(callp, curlwp, data, retval);\n" > rumpcalls
	printf "\trump_unschedule();\n\n\treturn rv;\n}\n\n" > rumpcalls
	printf "#define rsys_seterrno(error) rumpuser_seterrno(error)\n" > rumpcalls
	printf "#define rsys_alias(a,b) __weak_alias(a,b);\n#endif\n\n" > rumpcalls

	printf "#if\tBYTE_ORDER == BIG_ENDIAN\n" > rumpcalls
	printf "#define SPARG(p,k)\t((p)->k.be.datum)\n" > rumpcalls
	printf "#else /* LITTLE_ENDIAN, I hope dearly */\n" > rumpcalls
	printf "#define SPARG(p,k)\t((p)->k.le.datum)\n" > rumpcalls
	printf "#endif\n\n" > rumpcalls
	printf "#ifndef RUMP_CLIENT\n" > rumpcalls
	printf "int rump_enosys(void);\n" > rumpcalls
	printf "int\nrump_enosys()\n{\n\n\treturn ENOSYS;\n}\n" > rumpcalls
	printf "#endif\n" > rumpcalls

	printf "\n#ifndef RUMP_CLIENT\n" > rumpsysent
	printf "#define\ts(type)\tsizeof(type)\n" > rumpsysent
	printf "#define\tn(type)\t(sizeof(type)/sizeof (%s))\n", registertype > rumpsysent
	printf "#define\tns(type)\tn(type), s(type)\n\n", registertype > rumpsysent
	printf "struct sysent rump_sysent[] = {\n" > rumpsysent

	# System call names are included by userland (kdump(1)), so
	# hide the include files from it.
	printf "#if defined(_KERNEL_OPT)\n" > sysnames

	printf "#endif /* _KERNEL_OPT */\n\n" > sysnamesbottom
	printf "const char *const %s[] = {\n",namesname > sysnamesbottom

	printf " * created from%s\n */\n\n", $0 > sysnumhdr
	printf "#ifndef _" constprefix "SYSCALL_H_\n" > sysnumhdr
	printf "#define	_" constprefix "SYSCALL_H_\n\n" > sysnumhdr

	printf " * created from%s\n */\n\n", $0 > sysarghdr
	printf "#ifndef _" constprefix "SYSCALLARGS_H_\n" > sysarghdr
	printf "#define	_" constprefix "SYSCALLARGS_H_\n\n" > sysarghdr

	printf " * created from%s\n */\n\n", $0 > rumpcallshdr
	printf "#ifndef _RUMP_RUMP_SYSCALLS_H_\n" > rumpcallshdr
	printf "#define _RUMP_RUMP_SYSCALLS_H_\n\n" > rumpcallshdr
	printf "#ifdef _KERNEL\n" > rumpcallshdr
	printf "#error Interface not supported inside kernel\n" > rumpcallshdr
	printf "#endif /* _KERNEL */\n\n" > rumpcallshdr
	printf "#include <sys/types.h> /* typedefs */\n" > rumpcallshdr
	printf "#include <sys/select.h> /* typedefs */\n" > rumpcallshdr
	printf "#include <sys/socket.h> /* typedefs */\n\n" > rumpcallshdr
	printf "#include <signal.h> /* typedefs */\n\n" > rumpcallshdr
	printf "#include <rump/rump_syscalls_compat.h>\n\n" > rumpcallshdr

	printf "%s", sysarghdrextra > sysarghdr
	# Write max number of system call arguments to both headers
	printf("#define\t%sMAXSYSARGS\t%d\n\n", constprefix, maxsysargs) \
		> sysnumhdr
	printf("#define\t%sMAXSYSARGS\t%d\n\n", constprefix, maxsysargs) \
		> sysarghdr
	printf "#undef\tsyscallarg\n" > sysarghdr
	printf "#define\tsyscallarg(x)\t\t\t\t\t\t\t\\\n" > sysarghdr
	printf "\tunion {\t\t\t\t\t\t\t\t\\\n" > sysarghdr
	printf "\t\t%s pad;\t\t\t\t\t\t\\\n", registertype > sysarghdr
	printf "\t\tstruct { x datum; } le;\t\t\t\t\t\\\n" > sysarghdr
	printf "\t\tstruct { /* LINTED zero array dimension */\t\t\\\n" \
		> sysarghdr
	printf "\t\t\tint8_t pad[  /* CONSTCOND */\t\t\t\\\n" > sysarghdr
	printf "\t\t\t\t(sizeof (%s) < sizeof (x))\t\\\n", \
		registertype > sysarghdr
	printf "\t\t\t\t? 0\t\t\t\t\t\\\n" > sysarghdr
	printf "\t\t\t\t: sizeof (%s) - sizeof (x)];\t\\\n", \
		registertype > sysarghdr
	printf "\t\t\tx datum;\t\t\t\t\t\\\n" > sysarghdr
	printf "\t\t} be;\t\t\t\t\t\t\t\\\n" > sysarghdr
	printf "\t}\n" > sysarghdr
	printf("\n#undef check_syscall_args\n") >sysarghdr
	printf("#define check_syscall_args(call) /*LINTED*/ \\\n" \
		"\ttypedef char call##_check_args" \
		    "[sizeof (struct call##_args) \\\n" \
		"\t\t<= %sMAXSYSARGS * sizeof (%s) ? 1 : -1];\n", \
		constprefix, registertype) >sysarghdr

	# compat types from syscalls.master.  this is slightly ugly,
	# but given that we have so few compats from over 17 years,
	# a more complicated solution is not currently warranted.
	uncompattypes["struct timeval50"] = "struct timeval";
	uncompattypes["struct timespec50"] = "struct timespec";
	uncompattypes["struct stat30"] = "struct stat";

	next
}
NF == 0 || $1 ~ /^;/ {
	next
}
$0 ~ /^%%$/ {
	intable = 1
	next
}
$1 ~ /^#[ 	]*include/ {
	print > sysdcl
	print > sysnames
	next
}
$1 ~ /^#/ && !intable {
	print > sysdcl
	print > sysnames
	next
}
$1 ~ /^#/ && intable {
	if ($1 ~ /^#[ 	]*if/) {
		savedepth++
		savesyscall[savedepth] = syscall
	}
	if ($1 ~ /^#[ 	]*else/) {
		if (savedepth <= 0) {
			printf("%s: line %d: unbalanced #else\n", \
			    infile, NR)
			exit 1
		}
		syscall = savesyscall[savedepth]
	}
	if ($1 ~ /^#[       ]*endif/) {
		if (savedepth <= 0) {
			printf("%s: line %d: unbalanced #endif\n", \
			    infile, NR)
			exit 1
		}
		savedepth--
	}
	print > sysent
	print > sysarghdr
	print > sysnumhdr
	print > sysprotos
	print > sysnamesbottom

	# XXX: technically we do not want to have conditionals in rump,
	# but it is easier to just let the cpp handle them than try to
	# figure out what we want here in this script
	print > rumpsysent
	next
}
syscall != $1 {
	printf "%s: line %d: syscall number out of sync at %d\n", \
	   infile, NR, syscall
	printf "line is:\n"
	print
	exit 1
}
function parserr(was, wanted) {
	printf "%s: line %d: unexpected %s (expected <%s>)\n", \
	    infile, NR, was, wanted
	printf "line is:\n"
	print
	exit 1
}
function parseline() {
	f=3			# toss number and type
	if ($2 == "INDIR")
		sycall_flags="SYCALL_INDIRECT"
	else
		sycall_flags="0"
	if ($NF != "}") {
		funcalias=$NF
		end=NF-1
	} else {
		funcalias=""
		end=NF
	}
	if ($f == "INDIR") {		# allow for "NOARG INDIR"
		sycall_flags = "SYCALL_INDIRECT | " sycall_flags
		f++
	}
	if ($f == "MODULAR") {		# registered at runtime
		modular = 1
		f++
	} else {
		modular =  0;
	}
	if ($f == "RUMP") {
		rumpable = 1
		f++
	} else {
		rumpable = 0
	}
	if ($f ~ /^[a-z0-9_]*$/) {	# allow syscall alias
		funcalias=$f
		f++
	}
	if ($f != "{")
		parserr($f, "{")
	f++
	if ($end != "}")
		parserr($end, "}")
	end--
	if ($end != ";")
		parserr($end, ";")
	end--
	if ($end != ")")
		parserr($end, ")")
	end--

	returntype = oldf = "";
	do {
		if (returntype != "" && oldf != "*")
			returntype = returntype" ";
		returntype = returntype$f;
		oldf = $f;
		f++
	} while ($f != "|" && f < (end-1))
	if (f == (end - 1)) {
		parserr($f, "function argument definition (maybe \"|\"?)");
	}
	f++

	fprefix=$f
	f++
	if ($f != "|") {
		parserr($f, "function compat delimiter (maybe \"|\"?)");
	}
	f++

	fcompat=""
	if ($f != "|") {
		fcompat=$f
		f++
	}

	if ($f != "|") {
		parserr($f, "function name delimiter (maybe \"|\"?)");
	}
	f++
	fbase=$f

	# pipe is special in how to returns its values.
	# So just generate it manually if present.
	if (rumpable == 1 && fbase == "pipe") {
		rumpable = 0;
		rumphaspipe = 1;
	}

	if (fcompat != "") {
		funcname=fprefix "___" fbase "" fcompat
	} else {
		funcname=fprefix "_" fbase
	}
	if (returntype == "quad_t" || returntype == "off_t") {
		if (sycall_flags == "0")
			sycall_flags = "SYCALL_RET_64";
		else
			sycall_flags = "SYCALL_RET_64 | " sycall_flags;
	}

	if (funcalias == "") {
		funcalias=funcname
		sub(/^([^_]+_)*sys_/, "", funcalias)
		realname=fbase
	} else {
		realname=funcalias
	}
	rumpfname=realname "" fcompat
	f++

	if ($f != "(")
		parserr($f, "(")
	f++

	argc=0;
	argalign=0;
	if (f == end) {
		if ($f != "void")
			parserr($f, "argument definition")
		isvarargs = 0;
		varargc = 0;
		argtype[0]="void";
		return
	}

	# some system calls (open() and fcntl()) can accept a variable
	# number of arguments.  If syscalls accept a variable number of
	# arguments, they must still have arguments specified for
	# the remaining argument "positions," because of the way the
	# kernel system call argument handling works.
	#
	# Indirect system calls, e.g. syscall(), are exceptions to this
	# rule, since they are handled entirely by machine-dependent code
	# and do not need argument structures built.

	isvarargs = 0;
	args64 = 0;
	ptr = 0;
	while (f <= end) {
		if ($f == "...") {
			f++;
			isvarargs = 1;
			varargc = argc;
			continue;
		}
		argc++
		argtype[argc]=""
		oldf=""
		while (f < end && $(f+1) != ",") {
			if (argtype[argc] != "" && oldf != "*")
				argtype[argc] = argtype[argc]" ";
			argtype[argc] = argtype[argc]$f;
			oldf = $f;
			f++
		}
		if (argtype[argc] == "")
			parserr($f, "argument definition")
		if (argtype[argc] == "off_t"  \
		  || argtype[argc] == "dev_t" \
		  || argtype[argc] == "time_t") {
			if ((argalign % 2) != 0 && sysalign &&
			    funcname != "sys_posix_fadvise") # XXX for now
				parserr($f, "a padding argument")
		} else {
			argalign++;
		}
		if (argtype[argc] == "quad_t" || argtype[argc] == "off_t" \
		  || argtype[argc] == "dev_t" || argtype[argc] == "time_t") {
			if (sycall_flags == "0")
				sycall_flags = "SYCALL_ARG"argc-1"_64";
			else
				sycall_flags = "SYCALL_ARG"argc-1"_64 | " sycall_flags;
			args64++;
		}
		if (index(argtype[argc], "*") != 0 && ptr == 0) {
			if (sycall_flags == "0")
				sycall_flags = "SYCALL_ARG_PTR";
			else
				sycall_flags = "SYCALL_ARG_PTR | " sycall_flags;
			ptr = 1;
		}
		argname[argc]=$f;
		f += 2;			# skip name, and any comma
	}
	if (args64 > 0)
		sycall_flags = "SYCALL_NARGS64_VAL("args64") | " sycall_flags;
	# must see another argument after varargs notice.
	if (isvarargs) {
		if (argc == varargc)
			parserr($f, "argument definition")
	} else
		varargc = argc;
}

function printproto(wrap) {
	printf("/* syscall: \"%s%s\" ret: \"%s\" args:", wrap, funcalias,
	    returntype) > sysnumhdr
	for (i = 1; i <= varargc; i++)
		printf(" \"%s\"", argtype[i]) > sysnumhdr
	if (isvarargs)
		printf(" \"...\"") > sysnumhdr
	printf(" */\n") > sysnumhdr
	printf("#define\t%s%s%s\t%d\n\n", constprefix, wrap, funcalias,
	    syscall) > sysnumhdr

	# rumpalooza
	if (!rumpable)
		return

	# accumulate fbases we have seen.  we want the last
	# occurence for the default __RENAME()
	seen = funcseen[fbase]
	funcseen[fbase] = rumpfname
	if (seen)
		return

	printf("%s rump_sys_%s(", returntype, realname) > rumpprotos

	for (i = 1; i < varargc; i++)
		if (argname[i] != "PAD")
			printf("%s, ", uncompattype(argtype[i])) > rumpprotos

	if (isvarargs)
		printf("%s, ...)", uncompattype(argtype[varargc]))>rumpprotos
	else
		printf("%s)", uncompattype(argtype[argc])) > rumpprotos

	printf(" __RENAME(RUMP_SYS_RENAME_%s)", toupper(fbase))> rumpprotos
	printf(";\n") > rumpprotos

	# generate forward-declares for types, apart from the
	# braindead typedef jungle we cannot easily handle here
	for (i = 1; i <= varargc; i++) {
		type=uncompattype(argtype[i])
		sub("const ", "", type)
		if (!typeseen[type] && \
		    match(type, "struct") && match(type, "\\*")) {
			typeseen[type] = 1
			sub(" *\\*", "", type);
			printf("%s;\n", type) > rumptypes
		}
	}
}

function printrumpsysent(insysent, compatwrap) {
	if (!insysent) {
		eno[0] = "rump_enosys"
		eno[1] = "sys_nomodule"
		flags[0] = "SYCALL_NOSYS"
		flags[1] = "0"
		printf("\t{ 0, 0, %s,\n\t    (sy_call_t *)%s }, \t"	\
		    "/* %d = %s */\n",					\
		    flags[modular], eno[modular], syscall, funcalias)	\
		    > rumpsysent
		return
	}

	printf("\t{ ") > rumpsysent
	if (argc == 0) {
		printf("0, 0, ") > rumpsysent
	} else {
		printf("ns(struct %ssys_%s_args), ", compatwrap_, funcalias) > rumpsysent
	}

	if (compatwrap == "") {
		if (modular)
			rfn = "(sy_call_t *)sys_nomodule"
		else
			rfn = "(sy_call_t *)" funcname
	} else {
		rfn = "(sy_call_t *)" compatwrap "_" funcname
	}

	printf("0,\n\t    %s },", rfn) > rumpsysent
	for (i = 0; i < (33 - length(rfn)) / 8; i++)
		printf("\t") > rumpsysent
	printf("/* %d = %s%s */\n", syscall, compatwrap_, funcalias) > rumpsysent
}

function iscompattype(type) {
	for (var in uncompattypes) {
		if (match(type, var)) {
			return 1
		}
	}

	return 0
}

function uncompattype(type) {
	for (var in uncompattypes) {
		if (match(type, var)) {
			sub(var, uncompattypes[var], type)
			return type
		}
	}

	return type
}

function putent(type, compatwrap) {
	# output syscall declaration for switch table.
	if (compatwrap == "")
		compatwrap_ = ""
	else
		compatwrap_ = compatwrap "_"
	if (argc == 0)
		arg_type = "void";
	else {
		arg_type = "struct " compatwrap_ funcname "_args";
	}
	proto = "int\t" compatwrap_ funcname "(struct lwp *, const " \
	    arg_type " *, register_t *);\n"
	if (sysmap[proto] != 1) {
		sysmap[proto] = 1;
		print proto > sysprotos;
	}

	# output syscall switch entry
	printf("\t{ ") > sysent
	if (argc == 0) {
		printf("0, 0, ") > sysent
	} else {
		printf("ns(struct %s%s_args), ", compatwrap_, funcname) > sysent
	}
	if (modular) 
		wfn = "(sy_call_t *)sys_nomodule";
	else if (compatwrap == "")
		wfn = "(sy_call_t *)" funcname;
	else
		wfn = "(sy_call_t *)" compatwrap "(" funcname ")";
	printf("%s,\n\t    %s },", sycall_flags, wfn) > sysent
	for (i = 0; i < (33 - length(wfn)) / 8; i++)
		printf("\t") > sysent
	printf("/* %d = %s%s */\n", syscall, compatwrap_, funcalias) > sysent

	# output syscall name for names table
	printf("\t/* %3d */\t\"%s%s\",\n", syscall, compatwrap_, funcalias) \
	    > sysnamesbottom

	# output syscall number of header, if appropriate
	if (type == "STD" || type == "NOARGS" || type == "INDIR" || \
	    type == "NOERR") {
		# output a prototype, to be used to generate lint stubs in
		# libc.
		printproto("")
	} else if (type == "COMPAT" || type == "EXTERN") {
		# Just define the syscall number with a comment.  These
		# may be used by compatibility stubs in libc.
		printproto(compatwrap_)
	}

	# output syscall argument structure, if it has arguments
	if (argc != 0) {
		printf("\n") > sysarghdr
		if (haverumpcalls && !rumpable)
			printf("#ifndef RUMP_CLIENT\n") > sysarghdr
		printf("struct %s%s_args", compatwrap_, funcname) > sysarghdr
		if (type != "NOARGS") {
			print " {" >sysarghdr;
			for (i = 1; i <= argc; i++)
				printf("\tsyscallarg(%s) %s;\n", argtype[i],
				    argname[i]) > sysarghdr
			printf "}" >sysarghdr;
		}
		printf(";\n") > sysarghdr
		if (type != "NOARGS" && type != "INDIR") {
			printf("check_syscall_args(%s%s)\n", compatwrap_,
			    funcname) >sysarghdr
		}
		if (haverumpcalls && !rumpable)
			printf("#endif /* !RUMP_CLIENT */\n") > sysarghdr
	}

	if (!rumpable) {
		if (funcname == "sys_pipe" && rumphaspipe == 1)
			insysent = 1
		else
			insysent = 0
	} else {
		insysent = 1
	}
	printrumpsysent(insysent, compatwrap)

	# output rump marshalling code if necessary
	if (!rumpable) {
		return
	}

	# need a local prototype, we export the re-re-named one in .h
	printf("\n%s rump___sysimpl_%s(", returntype, rumpfname) \
	    > rumpcalls
	for (i = 1; i < argc; i++) {
		if (argname[i] != "PAD")
			printf("%s, ", uncompattype(argtype[i])) > rumpcalls
	}
	printf("%s);", uncompattype(argtype[argc])) > rumpcalls

	printf("\n%s\nrump___sysimpl_%s(", returntype, rumpfname) > rumpcalls
	for (i = 1; i < argc; i++) {
		if (argname[i] != "PAD")
			printf("%s %s, ", uncompattype(argtype[i]), \
			    argname[i]) > rumpcalls
	}
	printf("%s %s)\n", uncompattype(argtype[argc]), argname[argc]) \
	    > rumpcalls
	printf("{\n\tregister_t retval[2] = {0, 0};\n") > rumpcalls
	if (returntype != "void") {
		if (type != "NOERR") {
			printf("\tint error = 0;\n") > rumpcalls
		}
		# assume rumpcalls return only integral types
		printf("\t%s rv = -1;\n", returntype) > rumpcalls
	}

	argarg = "NULL"
	argsize = 0;
	if (argc) {
		argarg = "&callarg"
		argsize = "sizeof(callarg)"
		printf("\tstruct %s%s_args callarg;\n\n",compatwrap_,funcname) \
		    > rumpcalls
		for (i = 1; i <= argc; i++) {
			if (argname[i] == "PAD") {
				printf("\tSPARG(&callarg, %s) = 0;\n", \
				    argname[i]) > rumpcalls
			} else {
				if (iscompattype(argtype[i])) {
					printf("\tSPARG(&callarg, %s) = "    \
					"(%s)%s;\n", argname[i], argtype[i], \
					argname[i]) > rumpcalls
				} else {
					printf("\tSPARG(&callarg, %s) = %s;\n",\
					    argname[i], argname[i]) > rumpcalls
				}
			}
		}
		printf("\n") > rumpcalls
	} else {
		printf("\n") > rumpcalls
	}
	printf("\t") > rumpcalls
	if (returntype != "void" && type != "NOERR")
		printf("error = ") > rumpcalls
	printf("rsys_syscall(%s%s%s, " \
	    "%s, %s, retval);\n", constprefix, compatwrap_, funcalias, \
	    argarg, argsize) > rumpcalls
	if (type != "NOERR") {
		printf("\trsys_seterrno(error);\n") > rumpcalls
		printf("\tif (error == 0) {\n") > rumpcalls
		indent="\t\t"
		ending="\t}\n"
	} else {
		indent="\t"
		ending=""
	}
	if (returntype != "void") {
		printf("%sif (sizeof(%s) > sizeof(register_t))\n", \
		    indent, returntype) > rumpcalls
		printf("%s\trv = *(%s *)retval;\n", \
		    indent, returntype) > rumpcalls
		printf("%selse\n", indent, indent) > rumpcalls
		printf("%s\trv = *retval;\n", indent, returntype) > rumpcalls
		printf("%s", ending) > rumpcalls
		printf("\treturn rv;\n") > rumpcalls
	}
	printf("}\n") > rumpcalls
	printf("rsys_alias(%s%s,rump_enosys)\n", \
	    compatwrap_, funcname) > rumpcalls

}
$2 == "STD" || $2 == "NODEF" || $2 == "NOARGS" || $2 == "INDIR" \
    || $2 == "NOERR" {
	parseline()
	putent($2, "")
	syscall++
	next
}
$2 == "OBSOL" || $2 == "UNIMPL" || $2 == "EXCL" || $2 == "IGNORED" {
	if ($2 == "OBSOL")
		comment="obsolete"
	else if ($2 == "EXCL")
		comment="excluded"
	else if ($2 == "IGNORED")
		comment="ignored"
	else
		comment="unimplemented"
	for (i = 3; i <= NF; i++)
		comment=comment " " $i

	if ($2 == "IGNORED")
		sys_stub = "(sy_call_t *)nullop";
	else
		sys_stub = sys_nosys;

	printf("\t{ 0, 0, 0,\n\t    %s },\t\t\t/* %d = %s */\n", \
	    sys_stub, syscall, comment) > sysent
	printf("\t{ 0, 0, SYCALL_NOSYS,\n\t    %s },\t\t/* %d = %s */\n", \
	    "(sy_call_t *)rump_enosys", syscall, comment) > rumpsysent
	printf("\t/* %3d */\t\"#%d (%s)\",\n", syscall, syscall, comment) \
	    > sysnamesbottom
	if ($2 != "UNIMPL")
		printf("\t\t\t\t/* %d is %s */\n", syscall, comment) > sysnumhdr
	syscall++
	next
}
$2 == "EXTERN" {
	parseline()
	putent("EXTERN", "")
	syscall++
	next
}
{
	for (i = 1; i <= ncompat; i++) {
		if ($2 == compat_upper[i]) {
			parseline();
			putent("COMPAT", compat[i])
			syscall++
			next
		}
	}
	printf("%s: line %d: unrecognized keyword %s\n", infile, NR, $2)
	exit 1
}
END {
	# output pipe() syscall with its special retval[2] handling
	if (rumphaspipe) {
		printf("int rump_sys_pipe(int *);\n") > rumpprotos
		printf("\nint rump_sys_pipe(int *);\n") > rumpcalls
		printf("int\nrump_sys_pipe(int *fd)\n{\n") > rumpcalls
		printf("\tregister_t retval[2] = {0, 0};\n") > rumpcalls
		printf("\tint error = 0;\n") > rumpcalls
		printf("\n\terror = rsys_syscall(SYS_pipe, ") > rumpcalls
		printf("NULL, 0, retval);\n") > rumpcalls
		printf("\tif (error) {\n") > rumpcalls
		printf("\t\trsys_seterrno(error);\n") > rumpcalls
		printf("\t} else {\n\t\tfd[0] = retval[0];\n") > rumpcalls
		printf("\t\tfd[1] = retval[1];\n\t}\n") > rumpcalls
		printf("\treturn error ? -1 : 0;\n}\n") > rumpcalls
	}

	# print default rump syscall interfaces
	for (var in funcseen) {
		printf("#ifndef RUMP_SYS_RENAME_%s\n", \
		    toupper(var)) > rumpcallshdr
		printf("#define RUMP_SYS_RENAME_%s rump___sysimpl_%s\n", \
		    toupper(var), funcseen[var]) > rumpcallshdr
		printf("#endif\n\n") > rumpcallshdr
	}

	maxsyscall = syscall
	if (nsysent) {
		if (syscall > nsysent) {
			printf("%s: line %d: too many syscalls [%d > %d]\n", infile, NR, syscall, nsysent)
			exit 1
		}
		while (syscall < nsysent) {
			printf("\t{ 0, 0, 0,\n\t    %s },\t\t\t/* %d = filler */\n", \
			    sys_nosys, syscall) > sysent
			printf("\t{ 0, 0, SYCALL_NOSYS,\n\t    %s },\t\t/* %d = filler */\n", \
			    "(sy_call_t *)rump_enosys", syscall) > rumpsysent
			printf("\t/* %3d */\t\"# filler\",\n", syscall) \
			    > sysnamesbottom
			syscall++
		}
	}
	printf("};\n") > sysent
	printf("};\n") > rumpsysent
	printf("CTASSERT(__arraycount(rump_sysent) == SYS_NSYSENT);\n") > rumpsysent
	printf("__strong_alias(sysent,rump_sysent);\n") > rumpsysent
	printf("#endif /* RUMP_CLIENT */\n") > rumpsysent
	if (haverumpcalls)
		printf("#endif /* !RUMP_CLIENT */\n") > sysprotos
	printf("};\n") > sysnamesbottom
	printf("#define\t%sMAXSYSCALL\t%d\n", constprefix, maxsyscall) > sysnumhdr
	if (nsysent)
		printf("#define\t%sNSYSENT\t%d\n", constprefix, nsysent) > sysnumhdr
} '

cat $sysprotos >> $sysarghdr
echo "#endif /* _${constprefix}SYSCALL_H_ */" >> $sysnumhdr
echo "#endif /* _${constprefix}SYSCALLARGS_H_ */" >> $sysarghdr
printf "\n#endif /* _RUMP_RUMP_SYSCALLS_H_ */\n" >> $rumpprotos
cat $sysdcl $sysent > $syssw
cat $sysnamesbottom >> $sysnames
cat $rumpsysent >> $rumpcalls

touch $rumptypes
cat $rumptypes >> $rumpcallshdr
echo >> $rumpcallshdr
cat $rumpprotos >> $rumpcallshdr

#chmod 444 $sysnames $sysnumhdr $syssw
