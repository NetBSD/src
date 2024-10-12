#	$NetBSD: t_morse.sh,v 1.3 2024/10/12 20:44:11 rillig Exp $
#
# Copyright (c) 2024 The NetBSD Foundation, Inc.
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

atf_test_case digits
digits_head()
{
	atf_set 'require.progs' '/usr/games/morse'
}
digits_body()
{
	trailing_space=' '
	morse_s_digits="\
 -----
 .----
 ..---
 ...--
 ....-
 .....
 -....
 --...
 ---..
 ----.
$trailing_space
 ...-.-
"
	atf_check -o "inline:$morse_s_digits" \
	    /usr/games/morse -s 0123456789

	morse_digits="\
 daw daw daw daw daw
 dit daw daw daw daw
 dit dit daw daw daw
 dit dit dit daw daw
 dit dit dit dit daw
 dit dit dit dit dit
 daw dit dit dit dit
 daw daw dit dit dit
 daw daw daw dit dit
 daw daw daw daw dit

 dit dit dit daw dit daw
"
	atf_check -o "inline:$morse_digits" \
	    /usr/games/morse 0123456789
}


# Before 2024-06-16, non-ASCII characters invoked undefined behavior,
# possibly crashing morse.
atf_test_case nonascii
nonascii_head()
{
	atf_set 'require.progs' '/usr/games/morse'
}
nonascii_body()
{
	expected="\

 dit dit dit daw dit daw
"
	atf_check -o "inline:$expected" \
	    /usr/games/morse äöü
}


atf_test_case roundtrip
roundtrip_head() {
	atf_set 'require.progs' '/usr/games/morse'
}
roundtrip_body()
{
	# Most punctuation is ignored during encoding.
	# Missing are: !#$%&*;<>[\]^{|}~

	input=\
' !"#$%&'\''()*+,-./'\
'0123456789:;<=>?'\
'@ABCDEFGHIJKLMNO'\
'PQRSTUVWXYZ[\]^_'\
'`abcdefghijklmno'\
'pqrstuvwxyz{|}~'

	expected=\
' "'\''()+,-./'\
'0123456789:=?'\
'@ABCDEFGHIJKLMNO'\
'PQRSTUVWXYZ_'\
'ABCDEFGHIJKLMNO'\
'PQRSTUVWXYZ \n'

	atf_check -o 'save:roundtrip.morse' \
	    /usr/games/morse -s "$input"
	atf_check -o "inline:$expected" \
	    /usr/games/morse -d < roundtrip.morse
}


atf_init_test_cases()
{
	atf_add_test_case digits
	atf_add_test_case nonascii
	atf_add_test_case roundtrip
}
