Hello from testplugin.
.*: LDPT_MESSAGE func@0x.*
.*: LDPT_API_VERSION value        0x1 \(1\)
.*: LDPT_GNU_LD_VERSION value       0x.*
.*: LDPT_LINKER_OUTPUT value        0x1 \(1\)
.*: LDPT_OUTPUT_NAME 'tmpdir/main.x'
.*: LDPT_REGISTER_CLAIM_FILE_HOOK func@0x.*
.*: LDPT_REGISTER_ALL_SYMBOLS_READ_HOOK func@0x.*
.*: LDPT_REGISTER_CLEANUP_HOOK func@0x.*
.*: LDPT_ADD_SYMBOLS func@0x.*
.*: LDPT_GET_INPUT_FILE func@0x.*
.*: LDPT_GET_VIEW func@0x.*
.*: LDPT_RELEASE_INPUT_FILE func@0x.*
.*: LDPT_GET_SYMBOLS func@0x.*
.*: LDPT_GET_SYMBOLS_V2 func@0x.*
.*: LDPT_ADD_INPUT_FILE func@0x.*
.*: LDPT_ADD_INPUT_LIBRARY func@0x.*
.*: LDPT_SET_EXTRA_LIBRARY_PATH func@0x.*
.*: LDPT_OPTION 'registerclaimfile'
.*: LDPT_NULL value        0x0 \(0\)
hook called: claim_file tmpdir/main.o \[@0/.* not claimed
hook called: claim_file .*/ld/testsuite/ld-plugin/func.c \[@0/.* not claimed
.*/ld/testsuite/ld-plugin/func.c: file format not recognized; treating as linker script
.*/ld/testsuite/ld-plugin/func.c:5: syntax error
