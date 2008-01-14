#! /bin/sh
#	$NetBSD: tpann.sh,v 1.1 2008/01/14 12:47:59 yamt Exp $

# /*-
#  * Copyright (c)2008 YAMAMOTO Takashi,
#  * All rights reserved.
#  *
#  * Redistribution and use in source and binary forms, with or without
#  * modification, are permitted provided that the following conditions
#  * are met:
#  * 1. Redistributions of source code must retain the above copyright
#  *    notice, this list of conditions and the following disclaimer.
#  * 2. Redistributions in binary form must reproduce the above copyright
#  *    notice, this list of conditions and the following disclaimer in the
#  *    documentation and/or other materials provided with the distribution.
#  *
#  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  * SUCH DAMAGE.
#  */

# usage: tprof -c sleep 1 | sh fmt.sh

OBJ=/netbsd

if [ ! "${SIZEOF_PTR}" ]; then
	SIZEOF_PTR=4
fi

(hexdump -v -e "/${SIZEOF_PTR} \"%x\n\""|sort|uniq -c|sed -e 's/^/SAMPLE: /'
objdump -d  --disassemble-zeroes ${OBJ}) | \
awk '
/^SAMPLE:/ { samples[$3 ":"] = $2; next }
/^[0-9a-f]*:/ { printf("%8d  %s\n", samples[$1], $0); next }
// { print }
'
