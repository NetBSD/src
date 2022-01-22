<!--
    $NetBSD: HACKING.md,v 1.1 2022/01/22 08:09:39 pho Exp $
-->

# How to add support for a new API version

1. Update `_REFUSE_MAJOR_VERSION_` and/or `_REFUSE_MINOR_VERSION_` in
   `fuse.h`.
2. If the new API introduces some new typedefs, enums, constants,
   functions, or struct fields, define them *unconditionally*. Trying
   to conditionalize them will only increase complexity of the
   implementation without necessity.
3. If the new API removes some existing declarations, move them to
   `./refuse/legacy.[ch]`. There is no need to conditionally hide
   them.
4. If the new API doesn't change any of existing declarations, you are
   lucky. You are **tremendously** lucky. But if it does, things get
   interesting... (Spoiler: this happens all the time. All, the,
   time. As if there still weren't enough API-breaking changes in the
   history of FUSE API.)

## If it breaks API compatibility by changing function prototypes or whatever

1. Create a new header `./refuse/v${VERSION}.h`. Don't forget to add it
   in `./refuse/Makefile.inc` and `../../distrib/sets/lists/comp/mi`.
2. Include it from `./fuse.h` and add a new conditionalized section at
   the bottom of it. The whole point of the section is to choose
   correct symbols (or types, or whatever) and expose them without a
   version postfix.

### If it changes `struct fuse_operations`

1. Add `#define _FUSE_OP_VERSION__` for the new API version in the
   conditionalized section.
2. Define `struct fuse_operations_v${VERSION}` in `v${VERSION}.h`.
3. Update `./refuse/fs.c`. This is the abstraction layer which absorbs
   all the subtle differences in `struct fuse_operations` between
   versions. Every function has to be updated for the new version.

### If it changes anything else that are already versioned

1. Declare them in `./refuse/v${VERSION}.h` with a version postfix.
2. Update the conditionalized section for the version in `./fuse.h` so
   that it exposes the correct definition to user code.
3. Create `./refuse/v${VERSION}.c` and implement version-specific
   functions in it. Don't forget to add it to `./refuse/Makefile.inc`.

### If it changes something that have never been changed before

1. Move them from the unconditionalized part of the implementation to
   `./refuse/v${VERSION}.[ch]`.
2. Add a version postfix to them.
3. Update every single conditionalized section in `./fuse.h` so that
   they will be conditionally exposed without a version postfix
   depending the value of `FUSE_USE_VERSION`. If you cannot just
   `#define` them but need to introduce some functions, inline
   functions are preferred over function macros because the latter
   lack types and are therefore error-prone. Preprocessor conditionals
   are already error-prone so don't make them worse.
