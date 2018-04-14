TEXT_START_ADDR=0x8000
SHLIB_TEXT_START_ADDR=0x8000
SHLIB_DATA_ADDR=0x400000

. ${srcdir}/emulparams/shelf.sh

# Use only two underscores for the constructor/destructor symbols
CTOR_START='__ctors = .;'
CTOR_END='__ctors_end = .;'
DTOR_START='__dtors = .;'
DTOR_END='__dtors_end = .;'

# Suppress the .stack section.
unset STACK_ADDR
OTHER_SYMBOLS="PROVIDE (_stack = 0x30000);"
test -n "$CREATE_SHLIB" && unset OTHER_SYMBOLS

OUTPUT_FORMAT="elf32-shl-symbian"
SCRIPT_NAME=elf32sh-symbian
