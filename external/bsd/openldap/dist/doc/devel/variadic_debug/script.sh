#!/bin/bash

set -e

PATCH_DIR=doc/devel/variadic_debug

SPATCH=${SPATCH:-spatch}
SPATCH_OPTS=( --macro-file-builtins "$PATCH_DIR/macros.h" )
#SPATCH_OPTS+=( --timeout 300 )

SED_TRANSFORMATIONS=()

# split out multipart strings back to original form (one per line)
SED_TRANSFORMATIONS+=( -e 's/^\(+\s*\)\(.*"\) \(".*\)"$/\1\2\n+\1\3/' )

# re-add whitespace around parentheses
SED_TRANSFORMATIONS+=( -e 's/^\(+.*Debug[0-3]\?(\)\s*/\1 /' )
SED_TRANSFORMATIONS+=( -e 's/^\(+.*[^ ]\));$/\1 );/' )

# strip trailing whitespace copied from source on affected lines
SED_TRANSFORMATIONS+=( -e 's/^\(+.*\)\s\+$/\1/' )

# fix whitespace errors in source we touch
SED_TRANSFORMATIONS+=( -e 's/^\(+.*\)    \t/\1\t\t/' )
SED_TRANSFORMATIONS+=( -e 's/^\(+\t*\) \{1,3\}\t/\1\t/' )

normalise() {
    patch="$1"
    shift

    # iterate until we've reached fixpoint
    while ! cmp "$patch" "${patch}.new" 2>/dev/null; do
        if [ -e "${patch}.new" ]; then
            mv -- "${patch}.new" "$patch"
        fi
        sed "${SED_TRANSFORMATIONS[@]}" -- "$patch" >"${patch}.new"
    done
    rediff "$patch" >"${patch}.new"
    mv -- "${patch}.new" "$patch"
}

git add "$PATCH_DIR"
git commit -m "ITS#8731 Add the documentation and scripts"

git am "$PATCH_DIR/00-fixes.patch"
git am "$PATCH_DIR/01-logging.patch"
git am "$PATCH_DIR/02-manual.patch"

$SPATCH "${SPATCH_OPTS[@]}" -sp_file "$PATCH_DIR/03-libldap_Debug.cocci" \
    -dir libraries/libldap \
    >"$PATCH_DIR/03-libldap_Debug.patch"
normalise "$PATCH_DIR/03-libldap_Debug.patch"
git apply --index --directory libraries/libldap "$PATCH_DIR/03-libldap_Debug.patch"
git commit -m "ITS#8731 Apply $PATCH_DIR/03-libldap_Debug.cocci"

$SPATCH "${SPATCH_OPTS[@]}" -sp_file "$PATCH_DIR/04-variadic.cocci" \
    -dir . \
    >"$PATCH_DIR/04-variadic.patch"
normalise "$PATCH_DIR/04-variadic.patch"
git apply --index "$PATCH_DIR/04-variadic.patch"
git commit -m "ITS#8731 Apply $PATCH_DIR/04-variadic.cocci"

git am "$PATCH_DIR/05-back-sql.patch"
git am "$PATCH_DIR/06-nssov.patch"

$SPATCH "${SPATCH_OPTS[@]}" -sp_file "$PATCH_DIR/07-shortcut.cocci" \
    -dir . \
    >"$PATCH_DIR/07-shortcut.patch"
normalise "$PATCH_DIR/07-shortcut.patch"
git apply --index "$PATCH_DIR/07-shortcut.patch"
git commit -m "ITS#8731 Apply $PATCH_DIR/07-shortcut.cocci"

git am "$PATCH_DIR/08-snprintf-manual.patch"
