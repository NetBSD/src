#!/bin/bash
#
# Inspired by https://github.com/antiagainst/SPIRV-Tools/commit/c4f1bf8ddf7764b7c11fed1ce18ceb1d36b2eaf6
#
# Script to determine if source code in a diff is properly formatted.
# On Travis it uses the commit range of the PR or push, otherwise it uses a provided range
# Exits with non 0 exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

COMMIT_RANGE="${TRAVIS_COMMIT_RANGE:-$1}"

if [ -z "${COMMIT_RANGE}" ]; then
    >&2 echo "Empty commit range, missing parameter"
    return 0
fi

>&2 echo "Commit range $COMMIT_RANGE"

FILES_TO_CHECK="$(git diff --name-only "$COMMIT_RANGE" | grep -e '\.c$' -e '\.h$' || true)"
CFV="${CLANG_FORMAT_VERSION:--6.0}"

if [ -z "${FILES_TO_CHECK}" ]; then
    >&2 echo "No source code to check for formatting"
    return 0
fi

FORMAT_DIFF=$(git diff -U0 ${COMMIT_RANGE} -- ${FILES_TO_CHECK} | clang-format-diff$CFV -p1)

if [ -z "${FORMAT_DIFF}" ]; then
    >&2 echo "All source code in the diff is properly formatted"
    return 0
else
    >&2 echo -e "Found formatting errors\n"
    echo "${FORMAT_DIFF}"
    >&2 echo -e "\nYou can save the diff above and apply it with 'git apply -p0 my_diff'"
    exit 1
fi
