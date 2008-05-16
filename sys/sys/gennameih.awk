#!/usr/bin/awk -F
#
#	$NetBSD: gennameih.awk,v 1.1.34.1 2008/05/16 02:25:51 yamt Exp $
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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

function getrcsid(idstr) {
	sub("^[^$]*\\$", "", idstr);
	sub("\\$.*", "", idstr);

	return idstr;
}

NR == 1 {
	printf "/*\t\$NetBSD\$\t*/\n\n"

	myvers="$NetBSD: gennameih.awk,v 1.1.34.1 2008/05/16 02:25:51 yamt Exp $"

	print  "/*"
	print  " * WARNING: GENERATED FILE.  DO NOT EDIT"
	print  " * (edit namei.src and run make namei)"
	printf " *   by:   %s\n", getrcsid(myvers);
	printf " *   from: %s\n", getrcsid($0);
	print  " */"

	next

}

/^NAMEIFL/ {
	sub("NAMEIFL", "#define", $0);
	print $0;

	sub("^", "NAMEI_", $2)
	nameifl[i++] = "#define " $2 "\t" $3;
	next
}

{
	print $0
}

END {
	printf "\n/* Definitions match above, but with NAMEI_ prefix */\n"

	# print flags in the same order
	for (j = 0; j < i; j++) {
		print nameifl[j]
	}

	printf "\n#endif /* !_SYS_NAMEI_H_ */\n"
}
