#! /usr/bin/awk -f
#	$NetBSD: devlist2h.awk,v 1.1 1998/07/23 19:30:44 christos Exp $
#
# Copyright (c) 1998, Christos Zoulas
# Copyright (c) 1995, 1996 Christopher G. Demetriou
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
#      This product includes software developed by Christopher G. Demetriou.
#      This product includes software developed by Christos Zoulas
# 4. The name of the author(s) may not be used to endorse or promote products
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
#
function collectline(f, line) {
	oparen = 0
	line = ""
	while (f <= NF) {
		if ($f == "#") {
			line = line "("
			oparen = 1
			f++
			continue
		}
		if (oparen) {
			line = line $f
			if (f < NF)
				line = line " "
			f++
			continue
		}
		line = line $f
		if (f < NF)
			line = line " "
		f++
	}
	if (oparen)
		line = line ")"
	return line
}
function checkdecl() {
	done = 1
	if (!decl) {
		decl = 1;
		printf("struct isapnp_devinfo {\n") > hfile
		printf("\tconst char *const *devlogic;\n") > hfile
		printf("\tconst char *const *devcompat;\n") > hfile
		printf("};\n\n") > hfile
		printf("#include <sys/param.h>\n") > cfile
		printf("#include <dev/isapnp/isapnpdevs.h>\n\n") > cfile
	}
}
BEGIN {
	decl = done = ncompat = nlogicals = ndriver = 0
	cfile="isapnpdevs.c"
	hfile="isapnpdevs.h"
}
NR == 1 {
	VERSION = $0
	gsub("\\$", "", VERSION)

	printf("/*\t\$NetBSD\$\t*/\n\n") > cfile
	printf("/*\n") > cfile
	printf(" * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.\n") \
	    > cfile
	printf(" *\n") > cfile
	printf(" * generated from:\n") > cfile
	printf(" *\t%s\n", VERSION) > cfile
	printf(" */\n") > cfile

	printf("/*\t\$NetBSD\$\t*/\n\n") > hfile
	printf("/*\n") > hfile
	printf(" * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.\n") \
	    > hfile
	printf(" *\n") > hfile
	printf(" * generated from:\n") > hfile
	printf(" *\t%s\n", VERSION) > hfile
	printf(" */\n") > hfile
	printf("\n") > hfile
	next
}
$1 == "driver" {
	checkdecl()
	ndriver++

	driverindex[$2] = ndriver;
	driver[ndriver, 1] = $2;
	driver[ndriver, 2] = collectline(3, line);
	printf("/* %s */\n", driver[ndriver, 2]) > hfile
	printf("extern const struct isapnp_devinfo isapnp_%s_devinfo;\n",
	    driver[ndriver, 1]) > hfile
	next
}
$1 == "devlogic" {
	checkdecl()
	nlogicals++

	logicals[nlogicals, 1] = $2;
	logicals[nlogicals, 2] = $3;
	logicals[nlogicals, 3] = collectline(4, line);
	next
}
$1 == "devcompat" {
	checkdecl()
	ncompats++

	compats[ncompats, 1] = $2;
	compats[ncompats, 2] = $3;
	compats[ncompats, 3] = collectline(4, line);
	next
}
{
	if (!done) {
		if ($0 == "")
			blanklines++
		print $0 > hfile
		if (blanklines < 2)
			print $0 > cfile
	}
}
END {
	# print out the match tables

	printf("\n") > cfile

	for (i = 1; i <= ndriver; i++) {
		printf("/* %s */\n", driver[i, 2]) > cfile
		printf("static const char *isapnp_%s_devlogic[] = {\n",
		    driver[i, 1]) > cfile
		for (j = 1; j <= nlogicals; j++) {
			if (logicals[j, 1] == driver[i, 1]) {
				printf("\t\"%s\",\t/* %s */\n", logicals[j, 2],
				    logicals[j, 3]) > cfile
			}
		}
		printf("\tNULL\n};\n") > cfile
		printf("static const char *isapnp_%s_devcompat[] = {\n",
		    driver[i, 1]) > cfile
		for (j = 1; j <= ncompats; j++) {
			if (compats[j, 1] == driver[i, 1]) {
				printf("\t\"%s\",\t/* %s */\n", compats[j, 2],
				    compats[j, 3]) > cfile
			}
		}
		printf("\tNULL\n};\n") > cfile
		printf("const struct isapnp_devinfo isapnp_%s_devinfo = {\n",
		    driver[i, 1]) > cfile
		printf("\tisapnp_%s_devlogic, isapnp_%s_devcompat\n};\n",
		    driver[i, 1], driver[i, 1]) > cfile
		printf("\n") > cfile;

	}
}
