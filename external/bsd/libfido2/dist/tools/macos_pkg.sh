#!/bin/bash -e
# Copyright (c) 2019 Yubico AB. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

if [[ "$#" -ne 2 ]]; then
	echo usage: $0 version directory 1>&2
	exit 1
fi

V=$1
D=$2

FIDO_PATH=$(realpath ${D}/lib/libfido2.${V}.dylib)
CBOR_PATH=$(otool -L "${FIDO_PATH}" | grep cbor | awk '{ print $1 }')
CRYPTO_PATH=$(otool -L "${FIDO_PATH}" | grep crypto | awk '{ print $1 }')

cp -p "${CBOR_PATH}" "${CRYPTO_PATH}" "${D}/lib"
chmod 755 "${D}/lib/"*dylib
rm "${D}/lib/pkgconfig/libfido2.pc"
rmdir "${D}/lib/pkgconfig"

CBOR_NAME=$(echo "${CBOR_PATH}" | grep -o 'libcbor.*dylib')
CRYPTO_NAME=$(echo "${CRYPTO_PATH}" | grep -o 'libcrypto.*dylib')
FIDO_NAME="libfido2.${V}.dylib"

install_name_tool -id "@loader_path/${CBOR_NAME}" "${D}/lib/${CBOR_NAME}"
install_name_tool -id "@loader_path/${CRYPTO_NAME}" "${D}/lib/${CRYPTO_NAME}"
install_name_tool -id "@loader_path/libfido2.${V}.dylib" "${FIDO_PATH}"

install_name_tool -change "${CBOR_PATH}" "@loader_path/${CBOR_NAME}" \
	"${FIDO_PATH}"
install_name_tool -change "${CRYPTO_PATH}" "@loader_path/${CRYPTO_NAME}" \
	"${FIDO_PATH}"

for f in $(find "${D}/bin" -type f); do
	FIDO_PATH=$(otool -L "${f}" | grep libfido2 | awk '{ print $1 }')
	install_name_tool -change "${CBOR_PATH}" \
		"@executable_path/../lib/${CBOR_NAME}" "${f}"
	install_name_tool -change "${CRYPTO_PATH}" \
		"@executable_path/../lib/${CRYPTO_NAME}" "${f}"
	install_name_tool -change "${FIDO_PATH}" \
		"@executable_path/../lib/${FIDO_NAME}" "${f}"
done
