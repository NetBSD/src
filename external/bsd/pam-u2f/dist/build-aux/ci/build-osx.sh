#!/usr/bin/env bash
set -ex

# Link to the same OpenSSL version as libfido2.
OPENSSL="$(brew deps libfido2 | grep openssl)"
LIBFIDO2_PKGCONF="$(brew --prefix libfido2)/lib/pkgconfig"
OPENSSL_PKGCONF="$(brew --prefix "${OPENSSL}")/lib/pkgconfig"
export PKG_CONFIG_PATH="${LIBFIDO2_PKGCONF}:${OPENSSL_PKGCONF}"

./autogen.sh
./configure --disable-silent-rules --disable-man
make -j $(sysctl -n hw.logicalcpu)
make check
