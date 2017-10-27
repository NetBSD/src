#	$NetBSD: algorithms.sh,v 1.6 2017/10/27 04:31:50 ozaki-r Exp $
#
# Copyright (c) 2017 Internet Initiative Japan Inc.
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

ESP_ENCRYPTION_ALGORITHMS="des-cbc 3des-cbc null blowfish-cbc cast128-cbc \
    des-deriv rijndael-cbc aes-ctr camellia-cbc aes-gcm-16 aes-gmac"
ESP_ENCRYPTION_ALGORITHMS_MINIMUM="null rijndael-cbc"

# Valid key lengths of ESP encription algorithms
#    des-cbc         64
#    3des-cbc        192
#    null            0 to 2048     XXX only accept 0 length
#    blowfish-cbc    40 to 448
#    cast128-cbc     40 to 128
#    des-deriv       64
#    3des-deriv      192           XXX not implemented
#    rijndael-cbc    128/192/256
#    twofish-cbc     0 to 256      XXX not supported
#    aes-ctr         160/224/288
#    camellia-cbc    128/192/256
#    aes-gcm-16      160/224/288
#    aes-gmac        160/224/288
valid_keys_descbc="64"
invalid_keys_descbc="56 72"
valid_keys_3descbc="192"
invalid_keys_3descbc="184 200"
#valid_keys_null="0 2048"
valid_keys_null="0"
invalid_keys_null="8"
valid_keys_blowfishcbc="40 448"
invalid_keys_blowfishcbc="32 456"
valid_keys_cast128cbc="40 128"
invalid_keys_cast128cbc="32 136"
valid_keys_desderiv="64"
invalid_keys_desderiv="56 72"
#valid_keys_3desderiv="192"
#invalid_keys_3desderiv="184 200"
valid_keys_rijndaelcbc="128 192 256"
invalid_keys_rijndaelcbc="120 136 184 200 248 264"
#valid_keys_twofishcbc="0 256"
#invalid_keys_twofishcbc="264"
valid_keys_aesctr="160 224 288"
invalid_keys_aesctr="152 168 216 232 280 296"
valid_keys_camelliacbc="128 192 256"
invalid_keys_camelliacbc="120 136 184 200 248 264"
valid_keys_aesgcm16="160 224 288"
invalid_keys_aesgcm16="152 168 216 232 280 296"
valid_keys_aesgmac="160 224 288"
invalid_keys_aesgmac="152 168 216 232 280 296"

AH_AUTHENTICATION_ALGORITHMS="hmac-md5 hmac-sha1 keyed-md5 keyed-sha1 null \
    hmac-sha256 hmac-sha384 hmac-sha512 hmac-ripemd160 aes-xcbc-mac"
AH_AUTHENTICATION_ALGORITHMS_MINIMUM="null hmac-sha512"

# Valid key lengths of AH authentication algorithms
#    hmac-md5        128
#    hmac-sha1       160
#    keyed-md5       128
#    keyed-sha1      160
#    null            0 to 2048
#    hmac-sha256     256
#    hmac-sha384     384
#    hmac-sha512     512
#    hmac-ripemd160  160
#    aes-xcbc-mac    128
#    tcp-md5         8 to 640  XXX not enabled in rump kernels
valid_keys_hmacmd5="128"
invalid_keys_hmacmd5="120 136"
valid_keys_hmacsha1="160"
invalid_keys_hmacsha1="152 168"
valid_keys_keyedmd5="128"
invalid_keys_keyedmd5="120 136"
valid_keys_keyedsha1="160"
invalid_keys_keyedsha1="152 168"
#valid_keys_null="0 2048"
valid_keys_null="0"
invalid_keys_null="8"
valid_keys_hmacsha256="256"
invalid_keys_hmacsha256="248 264"
valid_keys_hmacsha384="384"
invalid_keys_hmacsha384="376 392"
valid_keys_hmacsha512="512"
invalid_keys_hmacsha512="504 520"
valid_keys_hmacripemd160="160"
invalid_keys_hmacripemd160="152 168"
valid_keys_aesxcbcmac="128"
invalid_keys_aesxcbcmac="120 136"
#valid_keys_tcpmd5="8 640"
#invalid_keys_tcpmd5="648"

IPCOMP_COMPRESSION_ALGORITHMS="deflate"
IPCOMP_COMPRESSION_ALGORITHMS_MINIMUM="deflate"
valid_keys_deflate="0"
invalid_keys_deflate="8"
minlen_deflate="90"

get_one_valid_keylen()
{
	local algo=$1
	local _algo=$(echo $algo | sed 's/-//g')
	local len=
	local keylengths=

	eval keylengths="\$valid_keys_${_algo}"

	for len in $(echo $keylengths); do
		break;
	done

	echo $len
}

get_valid_keylengths()
{
	local algo=$1
	local _algo=$(echo $algo | sed 's/-//g')

	eval keylengths="\$valid_keys_${_algo}"
	echo $keylengths
}

get_invalid_keylengths()
{
	local algo=$1
	local _algo=$(echo $algo | sed 's/-//g')

	eval keylengths="\$invalid_keys_${_algo}"
	echo $keylengths
}

generate_key()
{
	local keylen=$(($1 / 8))
	local key=

	while [ $keylen -gt 0 ]; do
		key="${key}a"
		keylen=$((keylen - 1))
	done
	if [ ! -z "$key" ]; then
		key="\"$key\""
	fi

	echo $key
}

generate_algo_args()
{
	local proto=$1
	local algo=$2
	local keylen=$(get_one_valid_keylen $algo)
	local key=$(generate_key $keylen)

	if [ $proto = esp -o $proto = "esp-udp" ]; then
		echo "-E $algo $key"
	elif [ $proto = ah ]; then
		echo "-A $algo $key"
	else
		echo "-C $algo $key"
	fi
}

get_minlen()
{
	local algo=$1
	local minlen=

	eval minlen="\$minlen_${algo}"
	echo $minlen
}
