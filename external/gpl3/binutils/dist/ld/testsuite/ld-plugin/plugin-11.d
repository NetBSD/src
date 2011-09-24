Hello from testplugin.
tv\[0\]: LDPT_MESSAGE func@0x.*
tv\[1\]: LDPT_API_VERSION value        0x1 \(1\)
tv\[2\]: LDPT_GNU_LD_VERSION value       0x.*
tv\[3\]: LDPT_LINKER_OUTPUT value        0x1 \(1\)
tv\[4\]: LDPT_OUTPUT_NAME 'tmpdir/main.x'
tv\[5\]: LDPT_REGISTER_CLAIM_FILE_HOOK func@0x.*
tv\[6\]: LDPT_REGISTER_ALL_SYMBOLS_READ_HOOK func@0x.*
tv\[7\]: LDPT_REGISTER_CLEANUP_HOOK func@0x.*
tv\[8\]: LDPT_ADD_SYMBOLS func@0x.*
tv\[9\]: LDPT_GET_INPUT_FILE func@0x.*
tv\[10\]: LDPT_RELEASE_INPUT_FILE func@0x.*
tv\[11\]: LDPT_GET_SYMBOLS func@0x.*
tv\[12\]: LDPT_ADD_INPUT_FILE func@0x.*
tv\[13\]: LDPT_ADD_INPUT_LIBRARY func@0x.*
tv\[14\]: LDPT_SET_EXTRA_LIBRARY_PATH func@0x.*
tv\[15\]: LDPT_OPTION 'registerclaimfile'
tv\[16\]: LDPT_OPTION 'registerallsymbolsread'
tv\[17\]: LDPT_OPTION 'registercleanup'
tv\[18\]: LDPT_OPTION 'claim:tmpdir/func.o'
tv\[19\]: LDPT_OPTION 'sym:_?func::0:0:0'
tv\[20\]: LDPT_OPTION 'sym:_?func2::0:0:0'
tv\[21\]: LDPT_OPTION 'dumpresolutions'
tv\[22\]: LDPT_OPTION 'add:tmpdir/func.o'
tv\[23\]: LDPT_OPTION 'claim:tmpdir/libtext.a'
tv\[24\]: LDPT_OPTION 'sym:_?text::0:0:0'
tv\[25\]: LDPT_OPTION 'add:tmpdir/text.o'
tv\[26\]: LDPT_NULL value        0x0 \(0\)
#...
hook called: claim_file tmpdir/main.o \[@0/.* not claimed
hook called: claim_file tmpdir/func.o \[@0/.* CLAIMED
#...
hook called: claim_file tmpdir/libtext.a \[@.* CLAIMED
#...
hook called: all symbols read.
Sym: '_?func' Resolution: LDPR_PREVAILING_DEF
Sym: '_?func2' Resolution: LDPR_PREVAILING_DEF_IRONLY
Sym: '_?text' Resolution: LDPR_PREVAILING_DEF
hook called: cleanup.
#...
