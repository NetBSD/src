#!/usr/bin/env bash
set -euxo pipefail

CORPUS_URL="https://storage.googleapis.com/yubico-pam-u2f/corpus.tgz"

LIBCBOR_URL="git://github.com/pjk/libcbor"
LIBCBOR_TAG="v0.8.0"
LIBCBOR_CFLAGS="-fsanitize=address,alignment,bounds"
LIBFIDO2_URL="git://github.com/Yubico/libfido2"
LIBFIDO2_TAG="1.7.0"
LIBFIDO2_CFLAGS="-fsanitize=address,alignment,bounds"

COMMON_CFLAGS="-g2 -fno-omit-frame-pointer"
PAM_U2F_CFLAGS="-fsanitize=address,pointer-compare,pointer-subtract"
PAM_U2F_CFLAGS="${PAM_U2F_CFLAGS},undefined,bounds"
PAM_U2F_CFLAGS="${PAM_U2F_CFLAGS},leak"
PAM_U2F_CFLAGS="${PAM_U2F_CFLAGS} -fno-sanitize-recover=all"

${CC} --version
WORKDIR="${WORKDIR:-$(pwd)}"
FAKEROOT="${FAKEROOT:-$(mktemp -d)}"

export LD_LIBRARY_PATH="${FAKEROOT}/lib"
export PKG_CONFIG_PATH="${FAKEROOT}/lib/pkgconfig"
export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"
export ASAN_OPTIONS="detect_leaks=1:detect_invalid_pointer_pairs=2"

pushd "${FAKEROOT}" &>/dev/null

git clone --depth 1 "${LIBFIDO2_URL}" -b "${LIBFIDO2_TAG}"
git clone --depth 1 "${LIBCBOR_URL}" -b "${LIBCBOR_TAG}"

# libcbor (with libfido2 patch)
patch -d libcbor -p0 -s <libfido2/fuzz/README
pushd libcbor &>/dev/null
mkdir build
cmake -B build \
	-DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_PREFIX="${FAKEROOT}" \
	-DCMAKE_C_FLAGS_DEBUG="${LIBCBOR_CFLAGS} ${COMMON_CFLAGS}" \
	-DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=OFF \
	-DWITH_EXAMPLES=OFF
make VERBOSE=1 -j $(nproc) -C build all install
popd &>/dev/null # libcbor

# libfido2 (with fuzzing support)
pushd libfido2 &>/dev/null
mkdir build
cmake -B build \
	-DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_PREFIX="${FAKEROOT}" \
	-DCMAKE_C_FLAGS_DEBUG="${LIBFIDO2_CFLAGS} ${COMMON_CFLAGS}" \
	-DCMAKE_BUILD_TYPE=Debug -DFUZZ=1 -DLIBFUZZER=0 -DBUILD_EXAMPLES=0 \
	-DBUILD_TOOLS=0 -DBUILD_MANPAGES=0
make VERBOSE=1 -j $(nproc) -C build all install
popd &>/dev/null # libfido2

# pam-u2f
mkdir build
pushd build &>/dev/null
autoreconf -i "${WORKDIR}"
"${WORKDIR}"/configure --enable-fuzzing --disable-silent-rules \
	--disable-man CFLAGS="${PAM_U2F_CFLAGS} ${COMMON_CFLAGS}"
make -j $(nproc)

# fuzz
curl --retry 4 -s -o corpus.tgz "${CORPUS_URL}"
tar xzf corpus.tgz
fuzz/fuzz_format_parsers corpus/format_parsers \
	-reload=30 -print_pcs=1 -print_funcs=30 -timeout=10 -runs=1
fuzz/fuzz_auth corpus/auth \
	-reload=30 -print_pcs=1 -print_funcs=30 -timeout=10 -runs=1

popd &>/dev/null # fakeroot
