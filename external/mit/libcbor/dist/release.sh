#!/usr/bin/env bash

# Guides my forgetful self through the release process.
# Usage release.sh VERSION

set -e

function prompt() {
	echo "$1 Confirm with 'Yes'"
	read check
	if [ "$check" != "Yes" ]; then
		echo "Aborting..."
		exit 1
	fi
}
# http://stackoverflow.com/questions/59895/getting-the-source-directory-of-a-bash-script-from-within
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
OUTDIR=$(mktemp -d)
TAG_NAME="v$1"

cd $DIR

echo ">>>>> Checking changelog"
grep -A 5 -F $1 CHANGELOG.md || true
prompt "Is the changelog correct and complete?"

echo ">>>>> Checking Doxyfile"
grep PROJECT_NUMBER Doxyfile
prompt "Is the Doxyfile version correct?"

echo ">>>>> Checking CMakeLists"
grep -A 2 'SET(CBOR_VERSION_MAJOR' CMakeLists.txt
prompt "Is the CMake version correct?"

echo ">>>>> Checking docs"
grep 'version =\|release =' doc/source/conf.py
prompt "Are the versions correct?"

set -x
pushd doc
make clean
popd
doxygen
cd doc
make html
cd build

cp -r html libcbor_docs_html
tar -zcf libcbor_docs.tar.gz libcbor_docs_html

cp -r doxygen/html libcbor_api_docs_html
cp -r doxygen/html $DIR/docs/doxygen
tar -zcf libcbor_api_docs.tar.gz libcbor_api_docs_html

mv libcbor_docs.tar.gz libcbor_api_docs.tar.gz $OUTDIR

cd $OUTDIR

cmake $DIR -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=ON
make
ctest

cd $DIR
git add docs/doxygen
git commit -m "[Release] Add current API documentation"

prompt "Will proceed to tag the release with $TAG_NAME."
git tag $TAG_NAME
git push origin

set +x

echo "Release ready in $OUTDIR"
echo "Add the release to GitHub at https://github.com/PJK/libcbor/releases/new *now*"
prompt "Have you added the release to https://github.com/PJK/libcbor/releases/tag/$TAG_NAME?"

set -x

pushd docs
erb index.html.erb > index.html
git add index.html
git commit -m "[Release] Update website to $TAG_NAME"
git push

set +x

echo "Update the Hombrew tap (https://github.com/PJK/homebrew-libcbor) *now*"
prompt "Have you updated the tap?"