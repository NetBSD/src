#	$NetBSD: makedev2spec.awk,v 1.3 2002/03/31 01:09:44 bjh21 Exp $
#
# Copyright (c) 2002 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Luke Mewburn of Wasabi Systems.
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
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# makedev2spec.awk --
#	Generate mtree(8) specfile from MAKEDEV.wrapper output.
#	Typical usage is:
#		MAKEDEVSCRIPT=.../MAKEDEV sh MAKEDEV.wrapper some_devs |
#		    awk -f makedev2spec.awk > specfile
#

BEGIN \
{
	errexit = 0;
	prefix = "./dev/";
}


$1 == "ln" \
{
	if ($2 != "-fs" || NF != 4)
		err("Usage: ln -fs from to");
	delete gname[$4];
	delete uname[$4];
	type[$4] = "type=link link=" $3;
	mode[$4] = 0755;
	next;
}

$1 == "mkdir" \
{
	if (NF != 2)
		err("Usage: mkdir dir");
	type[$2] = "type=dir";
	next;
}

$1 == "mknod" \
{
	if (NF != 5)
		err("usage; mknod name type major minor");
	if ($3 !~ /^[bc]$/)
		err("unknown " $1 " type " $3);
	type[$2] = "type=" ($3 == "b" ? "block" : "char" ) \
		    " device=netbsd," $4 "," $5;
	next;
}


$1 == "rm" \
{
	if (NF < 2)
		err("Usage: rm [-f] file [...]");
	for (i = 2; i <= NF; i++) {
		if ($i == "-f")
			continue;
		else if ($i ~ /^-/)
			err("Unsupported " $1 " option " $i);
		n = split(glob($i), globs, " ");
		for (j = 1; j <= n; j++) {
			delete type[globs[j]];
			delete gname[globs[j]];
			delete mode[globs[j]];
			delete uname[globs[j]];
		}
	}
	next;
}

# XXX: doesn't change symlinks - need to fix
$1 == "chgrp" \
{
	if (NF < 3)
		err("Usage: chgrp group file [...]");
	for (i = 3; i <= NF; i++) {
		n = split(glob($i), globs, " ");
		for (j = 1; j <= n; j++) {
			gname[globs[j]] = $2;
		}
	}
	next;
}

# XXX: doesn't change symlinks - need to fix
$1 == "chmod" \
{
	if (NF < 3)
		err("Usage: chmod mode file [...]");
	for (i = 3; i <= NF; i++) {
		n = split(glob($i), globs, " ");
		for (j = 1; j <= n; j++) {
			mode[globs[j]] = $2;
		}
	}
	next;
}

# XXX: doesn't change symlinks - need to fix
$1 == "chown" \
{
	if (NF < 3)
		err("Usage: chown user[:group] file [...]");
	user = $2;
	if ((n = match(user, /[\.:]/)) > 0) {
		group = substr(user, n + 1);
		user = substr(user, 1, n-1);
	} else {
		group = "";
	}
	for (i = 3; i <= NF; i++) {
		n = split(glob($i), globs, " ");
		for (j = 1; j <= n; j++) {
			uname[globs[j]] = user;
			if (group != "")
				gname[globs[j]] = group;
		}
	}
	next;
}

{
	err("Unknown keyword '" $1 "'");
}



END \
{
	if (errexit == 0) {
		for (i in type) {
			printf("%s%-15s %s mode=0%s uname=%s gname=%s\n",
			    prefix, i, type[i],
			    mode[i]  ? mode[i]  : "600",
			    uname[i] ? uname[i] : "root",
			    gname[i] ? gname[i] : "wheel");
		}
	}
}


function err(msg) \
{
	printf("%s at line %d of input.\n", msg, NR) >"/dev/stderr";
	errexit=1;
	exit 1;
}

function glob(g,  l, result) \
{
	if (g !~ /[?*[]/)
		return g;
	result = "";
	gsub(/\./, "\\.", g);
	gsub(/\?/, ".", g);
	gsub(/\*/, ".*", g);
	g = "^" g "$";
#print "glob() got a glob " g  >"/dev/stderr";
	for (l in type) {
#print "glob() checking " l " and " g >"/dev/stderr";
		if (l ~ g) {
			result = l " " result;
#print "glob(" $0 ") matched " l " and " g >"/dev/stderr";
		}
	}
#print "glob(" $0 ") returned " result >"/dev/stderr";
	return result;
}
