Contents of the .debug_info.dwo section:

  Compilation Unit @ offset 0x0:
   Length:        0x178 \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Section contributions:
    .debug_abbrev.dwo:       0x0  0x154
    .debug_line.dwo:         0x0  0x40
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x0  0x14
 <0><b>: Abbrev Number: 12 \(DW_TAG_compile_unit\)
    <c>   DW_AT_producer    : GNU C\+\+ 4.7.x-google 20120720 \(prerelease\)
    <37>   DW_AT_language    : 4	\(C\+\+\)
    <38>   DW_AT_name        : dwp_test_main.cc
    <49>   DW_AT_comp_dir    : /home/ccoutant/opensource/binutils-git/binutils/gold/testsuite
    <88>   DW_AT_GNU_dwo_id  : 0xe5ba51d95c9aebc8
 <1><90>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <91>   DW_AT_byte_size   : 4
    <92>   DW_AT_encoding    : 5	\(signed\)
    <93>   DW_AT_name        : int
 <1><97>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <98>   DW_AT_byte_size   : 1
    <99>   DW_AT_encoding    : 2	\(boolean\)
    <9a>   DW_AT_name        : bool
 <1><9f>: Abbrev Number: 13 \(DW_TAG_subprogram\)
    <a0>   DW_AT_external    : 1
    <a0>   DW_AT_name        : main
    <a5>   DW_AT_decl_file   : 1
    <a6>   DW_AT_decl_line   : 30
    <a7>   DW_AT_type        : <0x90>
    <ab>   DW_AT_low_pc      : \(addr_index: 0x0\): <no .debug_addr section>
    <ac>   DW_AT_high_pc     : 0x304
    <b4>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <b6>   DW_AT_GNU_all_tail_call_sites: 1
    <b6>   DW_AT_sibling     : <0x11b>
 <2><ba>: Abbrev Number: 14 \(DW_TAG_lexical_block\)
    <bb>   DW_AT_low_pc      : \(addr_index: 0x1\): <no .debug_addr section>
    <bc>   DW_AT_high_pc     : 0x2fa
 <3><c4>: Abbrev Number: 15 \(DW_TAG_variable\)
    <c5>   DW_AT_name        : c1
    <c8>   DW_AT_decl_file   : 1
    <c9>   DW_AT_decl_line   : 32
    <ca>   DW_AT_type        : signature: 0xb5faa2a4b7a919c4
    <d2>   DW_AT_location    : 2 byte block: 91 60 	\(DW_OP_fbreg: -32\)
 <3><d5>: Abbrev Number: 15 \(DW_TAG_variable\)
    <d6>   DW_AT_name        : c2
    <d9>   DW_AT_decl_file   : 1
    <da>   DW_AT_decl_line   : 33
    <db>   DW_AT_type        : signature: 0xab98c7bc886f5266
    <e3>   DW_AT_location    : 2 byte block: 91 50 	\(DW_OP_fbreg: -48\)
 <3><e6>: Abbrev Number: 16 \(DW_TAG_variable\)
    <e7>   DW_AT_name        : __PRETTY_FUNCTION__
    <fb>   DW_AT_type        : <0x13f>
    <ff>   DW_AT_artificial  : 1
    <ff>   DW_AT_location    : 2 byte block: fb 2 	\(DW_OP_GNU_addr_index <0x2>\)
 <3><102>: Abbrev Number: 14 \(DW_TAG_lexical_block\)
    <103>   DW_AT_low_pc      : \(addr_index: 0x3\): <no .debug_addr section>
    <104>   DW_AT_high_pc     : 0x2f
 <4><10c>: Abbrev Number: 17 \(DW_TAG_variable\)
    <10d>   DW_AT_name        : i
    <10f>   DW_AT_decl_file   : 1
    <110>   DW_AT_decl_line   : 37
    <111>   DW_AT_type        : <0x90>
    <115>   DW_AT_location    : 2 byte block: 91 6c 	\(DW_OP_fbreg: -20\)
 <4><118>: Abbrev Number: 0
 <3><119>: Abbrev Number: 0
 <2><11a>: Abbrev Number: 0
 <1><11b>: Abbrev Number: 18 \(DW_TAG_array_type\)
    <11c>   DW_AT_type        : <0x137>
    <120>   DW_AT_sibling     : <0x12b>
 <2><124>: Abbrev Number: 19 \(DW_TAG_subrange_type\)
    <125>   DW_AT_type        : <0x12b>
    <129>   DW_AT_upper_bound : 10
 <2><12a>: Abbrev Number: 0
 <1><12b>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <12c>   DW_AT_byte_size   : 8
    <12d>   DW_AT_encoding    : 7	\(unsigned\)
    <12e>   DW_AT_name        : sizetype
 <1><137>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <138>   DW_AT_byte_size   : 1
    <139>   DW_AT_encoding    : 6	\(signed char\)
    <13a>   DW_AT_name        : char
 <1><13f>: Abbrev Number: 20 \(DW_TAG_const_type\)
    <140>   DW_AT_type        : <0x11b>
 <1><144>: Abbrev Number: 21 \(DW_TAG_variable\)
    <145>   DW_AT_name        : c3
    <148>   DW_AT_decl_file   : 2
    <149>   DW_AT_decl_line   : 57
    <14a>   DW_AT_type        : signature: 0xb534bdc1f01629bb
    <152>   DW_AT_external    : 1
    <152>   DW_AT_declaration : 1
 <1><152>: Abbrev Number: 22 \(DW_TAG_variable\)
    <153>   DW_AT_name        : v3
    <156>   DW_AT_decl_file   : 2
    <157>   DW_AT_decl_line   : 60
    <158>   DW_AT_type        : <0x90>
    <15c>   DW_AT_external    : 1
    <15c>   DW_AT_declaration : 1
 <1><15c>: Abbrev Number: 18 \(DW_TAG_array_type\)
    <15d>   DW_AT_type        : <0x137>
    <161>   DW_AT_sibling     : <0x167>
 <2><165>: Abbrev Number: 23 \(DW_TAG_subrange_type\)
 <2><166>: Abbrev Number: 0
 <1><167>: Abbrev Number: 22 \(DW_TAG_variable\)
    <168>   DW_AT_name        : v4
    <16b>   DW_AT_decl_file   : 2
    <16c>   DW_AT_decl_line   : 61
    <16d>   DW_AT_type        : <0x15c>
    <171>   DW_AT_external    : 1
    <171>   DW_AT_declaration : 1
 <1><171>: Abbrev Number: 22 \(DW_TAG_variable\)
    <172>   DW_AT_name        : v5
    <175>   DW_AT_decl_file   : 2
    <176>   DW_AT_decl_line   : 62
    <177>   DW_AT_type        : <0x15c>
    <17b>   DW_AT_external    : 1
    <17b>   DW_AT_declaration : 1
 <1><17b>: Abbrev Number: 0
  Compilation Unit @ offset 0x17c:
   Length:        0x5af \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Section contributions:
    .debug_abbrev.dwo:       0x154  0x21d
    .debug_line.dwo:         0x40  0x3d
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x14  0x44
 <0><187>: Abbrev Number: 12 \(DW_TAG_compile_unit\)
    <188>   DW_AT_producer    : GNU C\+\+ 4.7.x-google 20120720 \(prerelease\)
    <1b3>   DW_AT_language    : 4	\(C\+\+\)
    <1b4>   DW_AT_name        : dwp_test_1.cc
    <1c2>   DW_AT_comp_dir    : /home/ccoutant/opensource/binutils-git/binutils/gold/testsuite
    <201>   DW_AT_GNU_dwo_id  : 0x52f9c6092fdc3727
 <1><209>: Abbrev Number: 13 \(DW_TAG_class_type\)
    <20a>   DW_AT_name        : C1
    <20d>   DW_AT_signature   : signature: 0xb5faa2a4b7a919c4
    <215>   DW_AT_declaration : 1
    <215>   DW_AT_sibling     : <0x242>
 <2><219>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <21a>   DW_AT_external    : 1
    <21a>   DW_AT_name        : \(indexed string: 0x0\): testcase1
    <21b>   DW_AT_decl_file   : 1
    <21c>   DW_AT_decl_line   : 28
    <21d>   DW_AT_linkage_name: \(indexed string: 0xc\): _ZN2C19testcase1Ev
    <21e>   DW_AT_type        : <0x249>
    <222>   DW_AT_accessibility: 1	\(public\)
    <223>   DW_AT_declaration : 1
 <2><223>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <224>   DW_AT_external    : 1
    <224>   DW_AT_name        : \(indexed string: 0x1\): testcase2
    <225>   DW_AT_decl_file   : 1
    <226>   DW_AT_decl_line   : 31
    <227>   DW_AT_linkage_name: \(indexed string: 0xd\): _ZN2C19testcase2Ev
    <228>   DW_AT_type        : <0x249>
    <22c>   DW_AT_accessibility: 1	\(public\)
    <22d>   DW_AT_declaration : 1
 <2><22d>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <22e>   DW_AT_external    : 1
    <22e>   DW_AT_name        : \(indexed string: 0x4\): testcase3
    <22f>   DW_AT_decl_file   : 1
    <230>   DW_AT_decl_line   : 32
    <231>   DW_AT_linkage_name: \(indexed string: 0xe\): _ZN2C19testcase3Ev
    <232>   DW_AT_type        : <0x249>
    <236>   DW_AT_accessibility: 1	\(public\)
    <237>   DW_AT_declaration : 1
 <2><237>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <238>   DW_AT_external    : 1
    <238>   DW_AT_name        : \(indexed string: 0xa\): testcase4
    <239>   DW_AT_decl_file   : 1
    <23a>   DW_AT_decl_line   : 33
    <23b>   DW_AT_linkage_name: \(indexed string: 0xf\): _ZN2C19testcase4Ev
    <23c>   DW_AT_type        : <0x249>
    <240>   DW_AT_accessibility: 1	\(public\)
    <241>   DW_AT_declaration : 1
 <2><241>: Abbrev Number: 0
 <1><242>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <243>   DW_AT_byte_size   : 4
    <244>   DW_AT_encoding    : 5	\(signed\)
    <245>   DW_AT_name        : int
 <1><249>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <24a>   DW_AT_byte_size   : 1
    <24b>   DW_AT_encoding    : 2	\(boolean\)
    <24c>   DW_AT_name        : bool
 <1><251>: Abbrev Number: 15 \(DW_TAG_pointer_type\)
    <252>   DW_AT_byte_size   : 8
    <253>   DW_AT_type        : signature: 0xb5faa2a4b7a919c4
 <1><25b>: Abbrev Number: 13 \(DW_TAG_class_type\)
    <25c>   DW_AT_name        : C2
    <25f>   DW_AT_signature   : signature: 0xab98c7bc886f5266
    <267>   DW_AT_declaration : 1
    <267>   DW_AT_sibling     : <0x294>
 <2><26b>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <26c>   DW_AT_external    : 1
    <26c>   DW_AT_name        : \(indexed string: 0x0\): testcase1
    <26d>   DW_AT_decl_file   : 1
    <26e>   DW_AT_decl_line   : 40
    <26f>   DW_AT_linkage_name: \(indexed string: 0x7\): _ZN2C29testcase1Ev
    <270>   DW_AT_type        : <0x249>
    <274>   DW_AT_accessibility: 1	\(public\)
    <275>   DW_AT_declaration : 1
 <2><275>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <276>   DW_AT_external    : 1
    <276>   DW_AT_name        : \(indexed string: 0x1\): testcase2
    <277>   DW_AT_decl_file   : 1
    <278>   DW_AT_decl_line   : 41
    <279>   DW_AT_linkage_name: \(indexed string: 0x8\): _ZN2C29testcase2Ev
    <27a>   DW_AT_type        : <0x249>
    <27e>   DW_AT_accessibility: 1	\(public\)
    <27f>   DW_AT_declaration : 1
 <2><27f>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <280>   DW_AT_external    : 1
    <280>   DW_AT_name        : \(indexed string: 0x4\): testcase3
    <281>   DW_AT_decl_file   : 1
    <282>   DW_AT_decl_line   : 42
    <283>   DW_AT_linkage_name: \(indexed string: 0x9\): _ZN2C29testcase3Ev
    <284>   DW_AT_type        : <0x249>
    <288>   DW_AT_accessibility: 1	\(public\)
    <289>   DW_AT_declaration : 1
 <2><289>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <28a>   DW_AT_external    : 1
    <28a>   DW_AT_name        : \(indexed string: 0xa\): testcase4
    <28b>   DW_AT_decl_file   : 1
    <28c>   DW_AT_decl_line   : 43
    <28d>   DW_AT_linkage_name: \(indexed string: 0xb\): _ZN2C29testcase4Ev
    <28e>   DW_AT_type        : <0x249>
    <292>   DW_AT_accessibility: 1	\(public\)
    <293>   DW_AT_declaration : 1
 <2><293>: Abbrev Number: 0
 <1><294>: Abbrev Number: 15 \(DW_TAG_pointer_type\)
    <295>   DW_AT_byte_size   : 8
    <296>   DW_AT_type        : signature: 0xab98c7bc886f5266
 <1><29e>: Abbrev Number: 13 \(DW_TAG_class_type\)
    <29f>   DW_AT_name        : C3
    <2a2>   DW_AT_signature   : signature: 0xb534bdc1f01629bb
    <2aa>   DW_AT_declaration : 1
    <2aa>   DW_AT_sibling     : <0x2cd>
 <2><2ae>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <2af>   DW_AT_external    : 1
    <2af>   DW_AT_name        : \(indexed string: 0x0\): testcase1
    <2b0>   DW_AT_decl_file   : 1
    <2b1>   DW_AT_decl_line   : 50
    <2b2>   DW_AT_linkage_name: \(indexed string: 0x2\): _ZN2C39testcase1Ev
    <2b3>   DW_AT_type        : <0x249>
    <2b7>   DW_AT_accessibility: 1	\(public\)
    <2b8>   DW_AT_declaration : 1
 <2><2b8>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <2b9>   DW_AT_external    : 1
    <2b9>   DW_AT_name        : \(indexed string: 0x1\): testcase2
    <2ba>   DW_AT_decl_file   : 1
    <2bb>   DW_AT_decl_line   : 51
    <2bc>   DW_AT_linkage_name: \(indexed string: 0x3\): _ZN2C39testcase2Ev
    <2bd>   DW_AT_type        : <0x249>
    <2c1>   DW_AT_accessibility: 1	\(public\)
    <2c2>   DW_AT_declaration : 1
 <2><2c2>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <2c3>   DW_AT_external    : 1
    <2c3>   DW_AT_name        : \(indexed string: 0x4\): testcase3
    <2c4>   DW_AT_decl_file   : 1
    <2c5>   DW_AT_decl_line   : 52
    <2c6>   DW_AT_linkage_name: \(indexed string: 0x5\): _ZN2C39testcase3Ev
    <2c7>   DW_AT_type        : <0x249>
    <2cb>   DW_AT_accessibility: 1	\(public\)
    <2cc>   DW_AT_declaration : 1
 <2><2cc>: Abbrev Number: 0
 <1><2cd>: Abbrev Number: 15 \(DW_TAG_pointer_type\)
    <2ce>   DW_AT_byte_size   : 8
    <2cf>   DW_AT_type        : signature: 0xb534bdc1f01629bb
 <1><2d7>: Abbrev Number: 16 \(DW_TAG_subprogram\)
    <2d8>   DW_AT_external    : 1
    <2d8>   DW_AT_name        : f13i
    <2dd>   DW_AT_decl_file   : 1
    <2de>   DW_AT_decl_line   : 70
    <2df>   DW_AT_linkage_name: _Z4f13iv
    <2e8>   DW_AT_low_pc      : \(addr_index: 0x0\): <no .debug_addr section>
    <2e9>   DW_AT_high_pc     : 0x6
    <2f1>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <2f3>   DW_AT_GNU_all_call_sites: 1
 <1><2f3>: Abbrev Number: 17 \(DW_TAG_subprogram\)
    <2f4>   DW_AT_specification: <0x219>
    <2f8>   DW_AT_decl_file   : 2
    <2f9>   DW_AT_decl_line   : 30
    <2fa>   DW_AT_low_pc      : \(addr_index: 0x1\): <no .debug_addr section>
    <2fb>   DW_AT_high_pc     : 0x20
    <303>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <305>   DW_AT_object_pointer: <0x30d>
    <309>   DW_AT_GNU_all_tail_call_sites: 1
    <309>   DW_AT_sibling     : <0x317>
 <2><30d>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <30e>   DW_AT_name        : \(indexed string: 0x10\): this
    <30f>   DW_AT_type        : <0x317>
    <313>   DW_AT_artificial  : 1
    <313>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><316>: Abbrev Number: 0
 <1><317>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <318>   DW_AT_type        : <0x251>
 <1><31c>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <31d>   DW_AT_specification: <0x223>
    <321>   DW_AT_decl_file   : 2
    <322>   DW_AT_decl_line   : 38
    <323>   DW_AT_low_pc      : \(addr_index: 0x2\): <no .debug_addr section>
    <324>   DW_AT_high_pc     : 0x18
    <32c>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <32e>   DW_AT_object_pointer: <0x336>
    <332>   DW_AT_GNU_all_call_sites: 1
    <332>   DW_AT_sibling     : <0x340>
 <2><336>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <337>   DW_AT_name        : \(indexed string: 0x10\): this
    <338>   DW_AT_type        : <0x317>
    <33c>   DW_AT_artificial  : 1
    <33c>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><33f>: Abbrev Number: 0
 <1><340>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <341>   DW_AT_specification: <0x22d>
    <345>   DW_AT_decl_file   : 2
    <346>   DW_AT_decl_line   : 46
    <347>   DW_AT_low_pc      : \(addr_index: 0x3\): <no .debug_addr section>
    <348>   DW_AT_high_pc     : 0x18
    <350>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <352>   DW_AT_object_pointer: <0x35a>
    <356>   DW_AT_GNU_all_call_sites: 1
    <356>   DW_AT_sibling     : <0x364>
 <2><35a>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <35b>   DW_AT_name        : \(indexed string: 0x10\): this
    <35c>   DW_AT_type        : <0x317>
    <360>   DW_AT_artificial  : 1
    <360>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><363>: Abbrev Number: 0
 <1><364>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <365>   DW_AT_specification: <0x237>
    <369>   DW_AT_decl_file   : 2
    <36a>   DW_AT_decl_line   : 54
    <36b>   DW_AT_low_pc      : \(addr_index: 0x4\): <no .debug_addr section>
    <36c>   DW_AT_high_pc     : 0x16
    <374>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <376>   DW_AT_object_pointer: <0x37e>
    <37a>   DW_AT_GNU_all_call_sites: 1
    <37a>   DW_AT_sibling     : <0x388>
 <2><37e>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <37f>   DW_AT_name        : \(indexed string: 0x10\): this
    <380>   DW_AT_type        : <0x317>
    <384>   DW_AT_artificial  : 1
    <384>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><387>: Abbrev Number: 0
 <1><388>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <389>   DW_AT_specification: <0x26b>
    <38d>   DW_AT_decl_file   : 2
    <38e>   DW_AT_decl_line   : 62
    <38f>   DW_AT_low_pc      : \(addr_index: 0x5\): <no .debug_addr section>
    <390>   DW_AT_high_pc     : 0x16
    <398>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <39a>   DW_AT_object_pointer: <0x3a2>
    <39e>   DW_AT_GNU_all_call_sites: 1
    <39e>   DW_AT_sibling     : <0x3ac>
 <2><3a2>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <3a3>   DW_AT_name        : \(indexed string: 0x10\): this
    <3a4>   DW_AT_type        : <0x3ac>
    <3a8>   DW_AT_artificial  : 1
    <3a8>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><3ab>: Abbrev Number: 0
 <1><3ac>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <3ad>   DW_AT_type        : <0x294>
 <1><3b1>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <3b2>   DW_AT_specification: <0x275>
    <3b6>   DW_AT_decl_file   : 2
    <3b7>   DW_AT_decl_line   : 72
    <3b8>   DW_AT_low_pc      : \(addr_index: 0x6\): <no .debug_addr section>
    <3b9>   DW_AT_high_pc     : 0x1b
    <3c1>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <3c3>   DW_AT_object_pointer: <0x3cb>
    <3c7>   DW_AT_GNU_all_call_sites: 1
    <3c7>   DW_AT_sibling     : <0x3d5>
 <2><3cb>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <3cc>   DW_AT_name        : \(indexed string: 0x10\): this
    <3cd>   DW_AT_type        : <0x3ac>
    <3d1>   DW_AT_artificial  : 1
    <3d1>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><3d4>: Abbrev Number: 0
 <1><3d5>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <3d6>   DW_AT_specification: <0x27f>
    <3da>   DW_AT_decl_file   : 2
    <3db>   DW_AT_decl_line   : 82
    <3dc>   DW_AT_low_pc      : \(addr_index: 0x7\): <no .debug_addr section>
    <3dd>   DW_AT_high_pc     : 0x1b
    <3e5>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <3e7>   DW_AT_object_pointer: <0x3ef>
    <3eb>   DW_AT_GNU_all_call_sites: 1
    <3eb>   DW_AT_sibling     : <0x3f9>
 <2><3ef>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <3f0>   DW_AT_name        : \(indexed string: 0x10\): this
    <3f1>   DW_AT_type        : <0x3ac>
    <3f5>   DW_AT_artificial  : 1
    <3f5>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><3f8>: Abbrev Number: 0
 <1><3f9>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <3fa>   DW_AT_specification: <0x289>
    <3fe>   DW_AT_decl_file   : 2
    <3ff>   DW_AT_decl_line   : 92
    <400>   DW_AT_low_pc      : \(addr_index: 0x8\): <no .debug_addr section>
    <401>   DW_AT_high_pc     : 0x19
    <409>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <40b>   DW_AT_object_pointer: <0x413>
    <40f>   DW_AT_GNU_all_call_sites: 1
    <40f>   DW_AT_sibling     : <0x41d>
 <2><413>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <414>   DW_AT_name        : \(indexed string: 0x10\): this
    <415>   DW_AT_type        : <0x3ac>
    <419>   DW_AT_artificial  : 1
    <419>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><41c>: Abbrev Number: 0
 <1><41d>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <41e>   DW_AT_specification: <0x2ae>
    <422>   DW_AT_decl_file   : 2
    <423>   DW_AT_decl_line   : 102
    <424>   DW_AT_low_pc      : \(addr_index: 0x9\): <no .debug_addr section>
    <425>   DW_AT_high_pc     : 0x19
    <42d>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <42f>   DW_AT_object_pointer: <0x437>
    <433>   DW_AT_GNU_all_call_sites: 1
    <433>   DW_AT_sibling     : <0x441>
 <2><437>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <438>   DW_AT_name        : \(indexed string: 0x10\): this
    <439>   DW_AT_type        : <0x441>
    <43d>   DW_AT_artificial  : 1
    <43d>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><440>: Abbrev Number: 0
 <1><441>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <442>   DW_AT_type        : <0x2cd>
 <1><446>: Abbrev Number: 17 \(DW_TAG_subprogram\)
    <447>   DW_AT_specification: <0x2b8>
    <44b>   DW_AT_decl_file   : 2
    <44c>   DW_AT_decl_line   : 112
    <44d>   DW_AT_low_pc      : \(addr_index: 0xa\): <no .debug_addr section>
    <44e>   DW_AT_high_pc     : 0x1f
    <456>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <458>   DW_AT_object_pointer: <0x460>
    <45c>   DW_AT_GNU_all_tail_call_sites: 1
    <45c>   DW_AT_sibling     : <0x46a>
 <2><460>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <461>   DW_AT_name        : \(indexed string: 0x10\): this
    <462>   DW_AT_type        : <0x441>
    <466>   DW_AT_artificial  : 1
    <466>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><469>: Abbrev Number: 0
 <1><46a>: Abbrev Number: 21 \(DW_TAG_subprogram\)
    <46b>   DW_AT_external    : 1
    <46b>   DW_AT_name        : f11a
    <470>   DW_AT_decl_file   : 2
    <471>   DW_AT_decl_line   : 120
    <472>   DW_AT_linkage_name: _Z4f11av
    <47b>   DW_AT_type        : <0x242>
    <47f>   DW_AT_low_pc      : \(addr_index: 0xb\): <no .debug_addr section>
    <480>   DW_AT_high_pc     : 0xb
    <488>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <48a>   DW_AT_GNU_all_call_sites: 1
 <1><48a>: Abbrev Number: 17 \(DW_TAG_subprogram\)
    <48b>   DW_AT_specification: <0x2c2>
    <48f>   DW_AT_decl_file   : 2
    <490>   DW_AT_decl_line   : 126
    <491>   DW_AT_low_pc      : \(addr_index: 0xc\): <no .debug_addr section>
    <492>   DW_AT_high_pc     : 0x20
    <49a>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <49c>   DW_AT_object_pointer: <0x4a4>
    <4a0>   DW_AT_GNU_all_tail_call_sites: 1
    <4a0>   DW_AT_sibling     : <0x4ae>
 <2><4a4>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <4a5>   DW_AT_name        : \(indexed string: 0x10\): this
    <4a6>   DW_AT_type        : <0x441>
    <4aa>   DW_AT_artificial  : 1
    <4aa>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><4ad>: Abbrev Number: 0
 <1><4ae>: Abbrev Number: 22 \(DW_TAG_subprogram\)
    <4af>   DW_AT_external    : 1
    <4af>   DW_AT_name        : t12
    <4b3>   DW_AT_decl_file   : 2
    <4b4>   DW_AT_decl_line   : 134
    <4b5>   DW_AT_linkage_name: _Z3t12v
    <4bd>   DW_AT_type        : <0x249>
    <4c1>   DW_AT_low_pc      : \(addr_index: 0xd\): <no .debug_addr section>
    <4c2>   DW_AT_high_pc     : 0x19
    <4ca>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <4cc>   DW_AT_GNU_all_tail_call_sites: 1
 <1><4cc>: Abbrev Number: 22 \(DW_TAG_subprogram\)
    <4cd>   DW_AT_external    : 1
    <4cd>   DW_AT_name        : t13
    <4d1>   DW_AT_decl_file   : 2
    <4d2>   DW_AT_decl_line   : 142
    <4d3>   DW_AT_linkage_name: _Z3t13v
    <4db>   DW_AT_type        : <0x249>
    <4df>   DW_AT_low_pc      : \(addr_index: 0xe\): <no .debug_addr section>
    <4e0>   DW_AT_high_pc     : 0x14
    <4e8>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <4ea>   DW_AT_GNU_all_tail_call_sites: 1
 <1><4ea>: Abbrev Number: 23 \(DW_TAG_subprogram\)
    <4eb>   DW_AT_external    : 1
    <4eb>   DW_AT_name        : t14
    <4ef>   DW_AT_decl_file   : 2
    <4f0>   DW_AT_decl_line   : 150
    <4f1>   DW_AT_linkage_name: _Z3t14v
    <4f9>   DW_AT_type        : <0x249>
    <4fd>   DW_AT_low_pc      : \(addr_index: 0xf\): <no .debug_addr section>
    <4fe>   DW_AT_high_pc     : 0x61
    <506>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <508>   DW_AT_GNU_all_tail_call_sites: 1
    <508>   DW_AT_sibling     : <0x532>
 <2><50c>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <50d>   DW_AT_low_pc      : \(addr_index: 0x10\): <no .debug_addr section>
    <50e>   DW_AT_high_pc     : 0x57
 <3><516>: Abbrev Number: 25 \(DW_TAG_variable\)
    <517>   DW_AT_name        : s1
    <51a>   DW_AT_decl_file   : 2
    <51b>   DW_AT_decl_line   : 152
    <51c>   DW_AT_type        : <0x532>
    <520>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <3><523>: Abbrev Number: 25 \(DW_TAG_variable\)
    <524>   DW_AT_name        : s2
    <527>   DW_AT_decl_file   : 2
    <528>   DW_AT_decl_line   : 153
    <529>   DW_AT_type        : <0x532>
    <52d>   DW_AT_location    : 2 byte block: 91 60 	\(DW_OP_fbreg: -32\)
 <3><530>: Abbrev Number: 0
 <2><531>: Abbrev Number: 0
 <1><532>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <533>   DW_AT_byte_size   : 8
    <534>   DW_AT_type        : <0x538>
 <1><538>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <539>   DW_AT_type        : <0x53d>
 <1><53d>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <53e>   DW_AT_byte_size   : 1
    <53f>   DW_AT_encoding    : 6	\(signed char\)
    <540>   DW_AT_name        : char
 <1><545>: Abbrev Number: 23 \(DW_TAG_subprogram\)
    <546>   DW_AT_external    : 1
    <546>   DW_AT_name        : t15
    <54a>   DW_AT_decl_file   : 2
    <54b>   DW_AT_decl_line   : 163
    <54c>   DW_AT_linkage_name: _Z3t15v
    <554>   DW_AT_type        : <0x249>
    <558>   DW_AT_low_pc      : \(addr_index: 0x11\): <no .debug_addr section>
    <559>   DW_AT_high_pc     : 0x5d
    <561>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <563>   DW_AT_GNU_all_tail_call_sites: 1
    <563>   DW_AT_sibling     : <0x58d>
 <2><567>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <568>   DW_AT_low_pc      : \(addr_index: 0x12\): <no .debug_addr section>
    <569>   DW_AT_high_pc     : 0x53
 <3><571>: Abbrev Number: 25 \(DW_TAG_variable\)
    <572>   DW_AT_name        : s1
    <575>   DW_AT_decl_file   : 2
    <576>   DW_AT_decl_line   : 165
    <577>   DW_AT_type        : <0x58d>
    <57b>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <3><57e>: Abbrev Number: 25 \(DW_TAG_variable\)
    <57f>   DW_AT_name        : s2
    <582>   DW_AT_decl_file   : 2
    <583>   DW_AT_decl_line   : 166
    <584>   DW_AT_type        : <0x58d>
    <588>   DW_AT_location    : 2 byte block: 91 60 	\(DW_OP_fbreg: -32\)
 <3><58b>: Abbrev Number: 0
 <2><58c>: Abbrev Number: 0
 <1><58d>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <58e>   DW_AT_byte_size   : 8
    <58f>   DW_AT_type        : <0x593>
 <1><593>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <594>   DW_AT_type        : <0x598>
 <1><598>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <599>   DW_AT_byte_size   : 4
    <59a>   DW_AT_encoding    : 5	\(signed\)
    <59b>   DW_AT_name        : wchar_t
 <1><5a3>: Abbrev Number: 22 \(DW_TAG_subprogram\)
    <5a4>   DW_AT_external    : 1
    <5a4>   DW_AT_name        : t16
    <5a8>   DW_AT_decl_file   : 2
    <5a9>   DW_AT_decl_line   : 176
    <5aa>   DW_AT_linkage_name: _Z3t16v
    <5b2>   DW_AT_type        : <0x249>
    <5b6>   DW_AT_low_pc      : \(addr_index: 0x13\): <no .debug_addr section>
    <5b7>   DW_AT_high_pc     : 0x13
    <5bf>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <5c1>   DW_AT_GNU_all_tail_call_sites: 1
 <1><5c1>: Abbrev Number: 26 \(DW_TAG_subprogram\)
    <5c2>   DW_AT_external    : 1
    <5c2>   DW_AT_name        : t17
    <5c6>   DW_AT_decl_file   : 2
    <5c7>   DW_AT_decl_line   : 184
    <5c8>   DW_AT_linkage_name: _Z3t17v
    <5d0>   DW_AT_type        : <0x249>
    <5d4>   DW_AT_low_pc      : \(addr_index: 0x14\): <no .debug_addr section>
    <5d5>   DW_AT_high_pc     : 0x5f
    <5dd>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <5df>   DW_AT_GNU_all_call_sites: 1
    <5df>   DW_AT_sibling     : <0x612>
 <2><5e3>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <5e4>   DW_AT_low_pc      : \(addr_index: 0x15\): <no .debug_addr section>
    <5e5>   DW_AT_high_pc     : 0x59
 <3><5ed>: Abbrev Number: 25 \(DW_TAG_variable\)
    <5ee>   DW_AT_name        : c
    <5f0>   DW_AT_decl_file   : 2
    <5f1>   DW_AT_decl_line   : 186
    <5f2>   DW_AT_type        : <0x53d>
    <5f6>   DW_AT_location    : 2 byte block: 91 6f 	\(DW_OP_fbreg: -17\)
 <3><5f9>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <5fa>   DW_AT_low_pc      : \(addr_index: 0x16\): <no .debug_addr section>
    <5fb>   DW_AT_high_pc     : 0x50
 <4><603>: Abbrev Number: 25 \(DW_TAG_variable\)
    <604>   DW_AT_name        : i
    <606>   DW_AT_decl_file   : 2
    <607>   DW_AT_decl_line   : 187
    <608>   DW_AT_type        : <0x242>
    <60c>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <4><60f>: Abbrev Number: 0
 <3><610>: Abbrev Number: 0
 <2><611>: Abbrev Number: 0
 <1><612>: Abbrev Number: 23 \(DW_TAG_subprogram\)
    <613>   DW_AT_external    : 1
    <613>   DW_AT_name        : t18
    <617>   DW_AT_decl_file   : 2
    <618>   DW_AT_decl_line   : 199
    <619>   DW_AT_linkage_name: _Z3t18v
    <621>   DW_AT_type        : <0x249>
    <625>   DW_AT_low_pc      : \(addr_index: 0x17\): <no .debug_addr section>
    <626>   DW_AT_high_pc     : 0x5f
    <62e>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <630>   DW_AT_GNU_all_tail_call_sites: 1
    <630>   DW_AT_sibling     : <0x67a>
 <2><634>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <635>   DW_AT_low_pc      : \(addr_index: 0x18\): <no .debug_addr section>
    <636>   DW_AT_high_pc     : 0x55
 <3><63e>: Abbrev Number: 25 \(DW_TAG_variable\)
    <63f>   DW_AT_name        : c
    <641>   DW_AT_decl_file   : 2
    <642>   DW_AT_decl_line   : 201
    <643>   DW_AT_type        : <0x53d>
    <647>   DW_AT_location    : 2 byte block: 91 6f 	\(DW_OP_fbreg: -17\)
 <3><64a>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <64b>   DW_AT_low_pc      : \(addr_index: 0x19\): <no .debug_addr section>
    <64c>   DW_AT_high_pc     : 0x4c
 <4><654>: Abbrev Number: 25 \(DW_TAG_variable\)
    <655>   DW_AT_name        : i
    <657>   DW_AT_decl_file   : 2
    <658>   DW_AT_decl_line   : 202
    <659>   DW_AT_type        : <0x242>
    <65d>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <4><660>: Abbrev Number: 24 \(DW_TAG_lexical_block\)
    <661>   DW_AT_low_pc      : \(addr_index: 0x1a\): <no .debug_addr section>
    <662>   DW_AT_high_pc     : 0x34
 <5><66a>: Abbrev Number: 25 \(DW_TAG_variable\)
    <66b>   DW_AT_name        : s
    <66d>   DW_AT_decl_file   : 2
    <66e>   DW_AT_decl_line   : 204
    <66f>   DW_AT_type        : <0x532>
    <673>   DW_AT_location    : 2 byte block: 91 60 	\(DW_OP_fbreg: -32\)
 <5><676>: Abbrev Number: 0
 <4><677>: Abbrev Number: 0
 <3><678>: Abbrev Number: 0
 <2><679>: Abbrev Number: 0
 <1><67a>: Abbrev Number: 27 \(DW_TAG_variable\)
    <67b>   DW_AT_name        : c3
    <67e>   DW_AT_decl_file   : 1
    <67f>   DW_AT_decl_line   : 57
    <680>   DW_AT_type        : signature: 0xb534bdc1f01629bb
    <688>   DW_AT_external    : 1
    <688>   DW_AT_declaration : 1
 <1><688>: Abbrev Number: 28 \(DW_TAG_variable\)
    <689>   DW_AT_name        : v2
    <68c>   DW_AT_decl_file   : 1
    <68d>   DW_AT_decl_line   : 59
    <68e>   DW_AT_type        : <0x242>
    <692>   DW_AT_external    : 1
    <692>   DW_AT_declaration : 1
 <1><692>: Abbrev Number: 28 \(DW_TAG_variable\)
    <693>   DW_AT_name        : v3
    <696>   DW_AT_decl_file   : 1
    <697>   DW_AT_decl_line   : 60
    <698>   DW_AT_type        : <0x242>
    <69c>   DW_AT_external    : 1
    <69c>   DW_AT_declaration : 1
 <1><69c>: Abbrev Number: 29 \(DW_TAG_array_type\)
    <69d>   DW_AT_type        : <0x53d>
    <6a1>   DW_AT_sibling     : <0x6a7>
 <2><6a5>: Abbrev Number: 30 \(DW_TAG_subrange_type\)
 <2><6a6>: Abbrev Number: 0
 <1><6a7>: Abbrev Number: 28 \(DW_TAG_variable\)
    <6a8>   DW_AT_name        : v4
    <6ab>   DW_AT_decl_file   : 1
    <6ac>   DW_AT_decl_line   : 61
    <6ad>   DW_AT_type        : <0x69c>
    <6b1>   DW_AT_external    : 1
    <6b1>   DW_AT_declaration : 1
 <1><6b1>: Abbrev Number: 28 \(DW_TAG_variable\)
    <6b2>   DW_AT_name        : v5
    <6b5>   DW_AT_decl_file   : 1
    <6b6>   DW_AT_decl_line   : 62
    <6b7>   DW_AT_type        : <0x69c>
    <6bb>   DW_AT_external    : 1
    <6bb>   DW_AT_declaration : 1
 <1><6bb>: Abbrev Number: 29 \(DW_TAG_array_type\)
    <6bc>   DW_AT_type        : <0x532>
    <6c0>   DW_AT_sibling     : <0x6c6>
 <2><6c4>: Abbrev Number: 30 \(DW_TAG_subrange_type\)
 <2><6c5>: Abbrev Number: 0
 <1><6c6>: Abbrev Number: 28 \(DW_TAG_variable\)
    <6c7>   DW_AT_name        : t17data
    <6cf>   DW_AT_decl_file   : 1
    <6d0>   DW_AT_decl_line   : 83
    <6d1>   DW_AT_type        : <0x6bb>
    <6d5>   DW_AT_external    : 1
    <6d5>   DW_AT_declaration : 1
 <1><6d5>: Abbrev Number: 31 \(DW_TAG_variable\)
    <6d6>   DW_AT_name        : p6
    <6d9>   DW_AT_decl_file   : 2
    <6da>   DW_AT_decl_line   : 69
    <6db>   DW_AT_type        : <0x6e2>
    <6df>   DW_AT_external    : 1
    <6df>   DW_AT_location    : 2 byte block: fb 1b 	\(DW_OP_GNU_addr_index <0x1b>\)
 <1><6e2>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <6e3>   DW_AT_byte_size   : 8
    <6e4>   DW_AT_type        : <0x242>
 <1><6e8>: Abbrev Number: 31 \(DW_TAG_variable\)
    <6e9>   DW_AT_name        : p7
    <6ec>   DW_AT_decl_file   : 2
    <6ed>   DW_AT_decl_line   : 79
    <6ee>   DW_AT_type        : <0x6e2>
    <6f2>   DW_AT_external    : 1
    <6f2>   DW_AT_location    : 2 byte block: fb 1c 	\(DW_OP_GNU_addr_index <0x1c>\)
 <1><6f5>: Abbrev Number: 31 \(DW_TAG_variable\)
    <6f6>   DW_AT_name        : p8
    <6f9>   DW_AT_decl_file   : 2
    <6fa>   DW_AT_decl_line   : 89
    <6fb>   DW_AT_type        : <0x702>
    <6ff>   DW_AT_external    : 1
    <6ff>   DW_AT_location    : 2 byte block: fb 1d 	\(DW_OP_GNU_addr_index <0x1d>\)
 <1><702>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <703>   DW_AT_byte_size   : 8
    <704>   DW_AT_type        : <0x53d>
 <1><708>: Abbrev Number: 31 \(DW_TAG_variable\)
    <709>   DW_AT_name        : p9
    <70c>   DW_AT_decl_file   : 2
    <70d>   DW_AT_decl_line   : 99
    <70e>   DW_AT_type        : <0x702>
    <712>   DW_AT_external    : 1
    <712>   DW_AT_location    : 2 byte block: fb 1e 	\(DW_OP_GNU_addr_index <0x1e>\)
 <1><715>: Abbrev Number: 9 \(DW_TAG_subroutine_type\)
    <716>   DW_AT_type        : <0x242>
 <1><71a>: Abbrev Number: 31 \(DW_TAG_variable\)
    <71b>   DW_AT_name        : pfn
    <71f>   DW_AT_decl_file   : 2
    <720>   DW_AT_decl_line   : 109
    <721>   DW_AT_type        : <0x728>
    <725>   DW_AT_external    : 1
    <725>   DW_AT_location    : 2 byte block: fb 1f 	\(DW_OP_GNU_addr_index <0x1f>\)
 <1><728>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <729>   DW_AT_byte_size   : 8
    <72a>   DW_AT_type        : <0x715>
 <1><72e>: Abbrev Number: 0
  Compilation Unit @ offset 0x72f:
   Length:        0xcb \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Section contributions:
    .debug_abbrev.dwo:       0x371  0xbd
    .debug_line.dwo:         0x7d  0x3e
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x0  0x0
 <0><73a>: Abbrev Number: 10 \(DW_TAG_compile_unit\)
    <73b>   DW_AT_producer    : GNU C\+\+ 4.7.x-google 20120720 \(prerelease\)
    <766>   DW_AT_language    : 4	\(C\+\+\)
    <767>   DW_AT_name        : dwp_test_1b.cc
    <776>   DW_AT_comp_dir    : /home/ccoutant/opensource/binutils-git/binutils/gold/testsuite
    <7b5>   DW_AT_GNU_dwo_id  : 0xbd6ec13ea247eff6
 <1><7bd>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <7be>   DW_AT_byte_size   : 4
    <7bf>   DW_AT_encoding    : 5	\(signed\)
    <7c0>   DW_AT_name        : int
 <1><7c4>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <7c5>   DW_AT_byte_size   : 1
    <7c6>   DW_AT_encoding    : 2	\(boolean\)
    <7c7>   DW_AT_name        : bool
 <1><7cc>: Abbrev Number: 11 \(DW_TAG_subprogram\)
    <7cd>   DW_AT_external    : 1
    <7cd>   DW_AT_name        : t16a
    <7d2>   DW_AT_decl_file   : 1
    <7d3>   DW_AT_decl_line   : 32
    <7d4>   DW_AT_linkage_name: _Z4t16av
    <7dd>   DW_AT_type        : <0x7c4>
    <7e1>   DW_AT_low_pc      : \(addr_index: 0x0\): <no .debug_addr section>
    <7e2>   DW_AT_high_pc     : 0x13
    <7ea>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <7ec>   DW_AT_GNU_all_tail_call_sites: 1
 <1><7ec>: Abbrev Number: 12 \(DW_TAG_variable\)
    <7ed>   DW_AT_name        : c3
    <7f0>   DW_AT_decl_file   : 1
    <7f1>   DW_AT_decl_line   : 29
    <7f2>   DW_AT_type        : signature: 0xb534bdc1f01629bb
    <7fa>   DW_AT_external    : 1
    <7fa>   DW_AT_location    : 2 byte block: fb 1 	\(DW_OP_GNU_addr_index <0x1>\)
 <1><7fd>: Abbrev Number: 0
  Compilation Unit @ offset 0x7fe:
   Length:        0x329 \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Section contributions:
    .debug_abbrev.dwo:       0x42e  0x1f2
    .debug_line.dwo:         0xbb  0x3d
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x58  0x18
 <0><809>: Abbrev Number: 12 \(DW_TAG_compile_unit\)
    <80a>   DW_AT_producer    : GNU C\+\+ 4.7.x-google 20120720 \(prerelease\)
    <835>   DW_AT_language    : 4	\(C\+\+\)
    <836>   DW_AT_name        : dwp_test_2.cc
    <844>   DW_AT_comp_dir    : /home/ccoutant/opensource/binutils-git/binutils/gold/testsuite
    <883>   DW_AT_GNU_dwo_id  : 0xcf0cab718ce0f8b9
 <1><88b>: Abbrev Number: 13 \(DW_TAG_class_type\)
    <88c>   DW_AT_name        : C1
    <88f>   DW_AT_signature   : signature: 0xb5faa2a4b7a919c4
    <897>   DW_AT_declaration : 1
    <897>   DW_AT_sibling     : <0x8b7>
 <2><89b>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <89c>   DW_AT_external    : 1
    <89c>   DW_AT_name        : t1a
    <8a0>   DW_AT_decl_file   : 1
    <8a1>   DW_AT_decl_line   : 29
    <8a2>   DW_AT_linkage_name: \(indexed string: 0x4\): _ZN2C13t1aEv
    <8a3>   DW_AT_type        : <0x8be>
    <8a7>   DW_AT_accessibility: 1	\(public\)
    <8a8>   DW_AT_declaration : 1
 <2><8a8>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <8a9>   DW_AT_external    : 1
    <8a9>   DW_AT_name        : t1_2
    <8ae>   DW_AT_decl_file   : 1
    <8af>   DW_AT_decl_line   : 30
    <8b0>   DW_AT_linkage_name: \(indexed string: 0x5\): _ZN2C14t1_2Ev
    <8b1>   DW_AT_type        : <0x8b7>
    <8b5>   DW_AT_accessibility: 1	\(public\)
    <8b6>   DW_AT_declaration : 1
 <2><8b6>: Abbrev Number: 0
 <1><8b7>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <8b8>   DW_AT_byte_size   : 4
    <8b9>   DW_AT_encoding    : 5	\(signed\)
    <8ba>   DW_AT_name        : int
 <1><8be>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <8bf>   DW_AT_byte_size   : 1
    <8c0>   DW_AT_encoding    : 2	\(boolean\)
    <8c1>   DW_AT_name        : bool
 <1><8c6>: Abbrev Number: 15 \(DW_TAG_pointer_type\)
    <8c7>   DW_AT_byte_size   : 8
    <8c8>   DW_AT_type        : signature: 0xb5faa2a4b7a919c4
 <1><8d0>: Abbrev Number: 13 \(DW_TAG_class_type\)
    <8d1>   DW_AT_name        : C3
    <8d4>   DW_AT_signature   : signature: 0xb534bdc1f01629bb
    <8dc>   DW_AT_declaration : 1
    <8dc>   DW_AT_sibling     : <0x8ed>
 <2><8e0>: Abbrev Number: 14 \(DW_TAG_subprogram\)
    <8e1>   DW_AT_external    : 1
    <8e1>   DW_AT_name        : f4
    <8e4>   DW_AT_decl_file   : 1
    <8e5>   DW_AT_decl_line   : 53
    <8e6>   DW_AT_linkage_name: \(indexed string: 0x3\): _ZN2C32f4Ev
    <8e7>   DW_AT_type        : <0x8fc>
    <8eb>   DW_AT_accessibility: 1	\(public\)
    <8ec>   DW_AT_declaration : 1
 <2><8ec>: Abbrev Number: 0
 <1><8ed>: Abbrev Number: 15 \(DW_TAG_pointer_type\)
    <8ee>   DW_AT_byte_size   : 8
    <8ef>   DW_AT_type        : signature: 0xb534bdc1f01629bb
 <1><8f7>: Abbrev Number: 9 \(DW_TAG_subroutine_type\)
    <8f8>   DW_AT_type        : <0x8be>
 <1><8fc>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <8fd>   DW_AT_byte_size   : 8
    <8fe>   DW_AT_type        : <0x8f7>
 <1><902>: Abbrev Number: 16 \(DW_TAG_subprogram\)
    <903>   DW_AT_external    : 1
    <903>   DW_AT_name        : f13i
    <908>   DW_AT_decl_file   : 1
    <909>   DW_AT_decl_line   : 70
    <90a>   DW_AT_linkage_name: _Z4f13iv
    <913>   DW_AT_low_pc      : \(addr_index: 0x0\): <no .debug_addr section>
    <914>   DW_AT_high_pc     : 0x6
    <91c>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <91e>   DW_AT_GNU_all_call_sites: 1
 <1><91e>: Abbrev Number: 17 \(DW_TAG_subprogram\)
    <91f>   DW_AT_specification: <0x8a8>
    <923>   DW_AT_decl_file   : 2
    <924>   DW_AT_low_pc      : \(addr_index: 0x1\): <no .debug_addr section>
    <925>   DW_AT_high_pc     : 0xf
    <92d>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <92f>   DW_AT_object_pointer: <0x937>
    <933>   DW_AT_GNU_all_call_sites: 1
    <933>   DW_AT_sibling     : <0x945>
 <2><937>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <938>   DW_AT_name        : this
    <93d>   DW_AT_type        : <0x945>
    <941>   DW_AT_artificial  : 1
    <941>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><944>: Abbrev Number: 0
 <1><945>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <946>   DW_AT_type        : <0x8c6>
 <1><94a>: Abbrev Number: 20 \(DW_TAG_subprogram\)
    <94b>   DW_AT_specification: <0x89b>
    <94f>   DW_AT_decl_file   : 2
    <950>   DW_AT_decl_line   : 36
    <951>   DW_AT_low_pc      : \(addr_index: 0x2\): <no .debug_addr section>
    <952>   DW_AT_high_pc     : 0x20
    <95a>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <95c>   DW_AT_object_pointer: <0x964>
    <960>   DW_AT_GNU_all_tail_call_sites: 1
    <960>   DW_AT_sibling     : <0x972>
 <2><964>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <965>   DW_AT_name        : this
    <96a>   DW_AT_type        : <0x945>
    <96e>   DW_AT_artificial  : 1
    <96e>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><971>: Abbrev Number: 0
 <1><972>: Abbrev Number: 21 \(DW_TAG_subprogram\)
    <973>   DW_AT_external    : 1
    <973>   DW_AT_name        : f10
    <977>   DW_AT_decl_file   : 2
    <978>   DW_AT_decl_line   : 72
    <979>   DW_AT_linkage_name: _Z3f10v
    <981>   DW_AT_type        : <0x8b7>
    <985>   DW_AT_low_pc      : \(addr_index: 0x3\): <no .debug_addr section>
    <986>   DW_AT_high_pc     : 0xb
    <98e>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <990>   DW_AT_GNU_all_call_sites: 1
 <1><990>: Abbrev Number: 22 \(DW_TAG_subprogram\)
    <991>   DW_AT_external    : 1
    <991>   DW_AT_name        : f11b
    <996>   DW_AT_decl_file   : 2
    <997>   DW_AT_decl_line   : 80
    <998>   DW_AT_linkage_name: _Z4f11bPFivE
    <9a5>   DW_AT_type        : <0x8b7>
    <9a9>   DW_AT_low_pc      : \(addr_index: 0x4\): <no .debug_addr section>
    <9aa>   DW_AT_high_pc     : 0x14
    <9b2>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <9b4>   DW_AT_GNU_all_tail_call_sites: 1
    <9b4>   DW_AT_sibling     : <0x9c7>
 <2><9b8>: Abbrev Number: 23 \(DW_TAG_formal_parameter\)
    <9b9>   DW_AT_name        : pfn
    <9bd>   DW_AT_decl_file   : 2
    <9be>   DW_AT_decl_line   : 80
    <9bf>   DW_AT_type        : <0x9cc>
    <9c3>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><9c6>: Abbrev Number: 0
 <1><9c7>: Abbrev Number: 9 \(DW_TAG_subroutine_type\)
    <9c8>   DW_AT_type        : <0x8b7>
 <1><9cc>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <9cd>   DW_AT_byte_size   : 8
    <9ce>   DW_AT_type        : <0x9c7>
 <1><9d2>: Abbrev Number: 24 \(DW_TAG_subprogram\)
    <9d3>   DW_AT_specification: <0x8e0>
    <9d7>   DW_AT_decl_file   : 2
    <9d8>   DW_AT_decl_line   : 88
    <9d9>   DW_AT_low_pc      : \(addr_index: 0x5\): <no .debug_addr section>
    <9da>   DW_AT_high_pc     : 0xf
    <9e2>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <9e4>   DW_AT_object_pointer: <0x9ec>
    <9e8>   DW_AT_GNU_all_call_sites: 1
    <9e8>   DW_AT_sibling     : <0x9fa>
 <2><9ec>: Abbrev Number: 18 \(DW_TAG_formal_parameter\)
    <9ed>   DW_AT_name        : this
    <9f2>   DW_AT_type        : <0x9fa>
    <9f6>   DW_AT_artificial  : 1
    <9f6>   DW_AT_location    : 2 byte block: 91 68 	\(DW_OP_fbreg: -24\)
 <2><9f9>: Abbrev Number: 0
 <1><9fa>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <9fb>   DW_AT_type        : <0x8ed>
 <1><9ff>: Abbrev Number: 25 \(DW_TAG_subroutine_type\)
 <1><a00>: Abbrev Number: 21 \(DW_TAG_subprogram\)
    <a01>   DW_AT_external    : 1
    <a01>   DW_AT_name        : f13
    <a05>   DW_AT_decl_file   : 2
    <a06>   DW_AT_decl_line   : 96
    <a07>   DW_AT_linkage_name: _Z3f13v
    <a0f>   DW_AT_type        : <0xa1e>
    <a13>   DW_AT_low_pc      : \(addr_index: 0x6\): <no .debug_addr section>
    <a14>   DW_AT_high_pc     : 0xb
    <a1c>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <a1e>   DW_AT_GNU_all_call_sites: 1
 <1><a1e>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <a1f>   DW_AT_byte_size   : 8
    <a20>   DW_AT_type        : <0x9ff>
 <1><a24>: Abbrev Number: 21 \(DW_TAG_subprogram\)
    <a25>   DW_AT_external    : 1
    <a25>   DW_AT_name        : f14
    <a29>   DW_AT_decl_file   : 2
    <a2a>   DW_AT_decl_line   : 104
    <a2b>   DW_AT_linkage_name: _Z3f14v
    <a33>   DW_AT_type        : <0xa42>
    <a37>   DW_AT_low_pc      : \(addr_index: 0x7\): <no .debug_addr section>
    <a38>   DW_AT_high_pc     : 0xb
    <a40>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <a42>   DW_AT_GNU_all_call_sites: 1
 <1><a42>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <a43>   DW_AT_byte_size   : 8
    <a44>   DW_AT_type        : <0xa48>
 <1><a48>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <a49>   DW_AT_type        : <0xa4d>
 <1><a4d>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <a4e>   DW_AT_byte_size   : 1
    <a4f>   DW_AT_encoding    : 6	\(signed char\)
    <a50>   DW_AT_name        : char
 <1><a55>: Abbrev Number: 21 \(DW_TAG_subprogram\)
    <a56>   DW_AT_external    : 1
    <a56>   DW_AT_name        : f15
    <a5a>   DW_AT_decl_file   : 2
    <a5b>   DW_AT_decl_line   : 112
    <a5c>   DW_AT_linkage_name: _Z3f15v
    <a64>   DW_AT_type        : <0xa73>
    <a68>   DW_AT_low_pc      : \(addr_index: 0x8\): <no .debug_addr section>
    <a69>   DW_AT_high_pc     : 0xb
    <a71>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <a73>   DW_AT_GNU_all_call_sites: 1
 <1><a73>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <a74>   DW_AT_byte_size   : 8
    <a75>   DW_AT_type        : <0xa79>
 <1><a79>: Abbrev Number: 19 \(DW_TAG_const_type\)
    <a7a>   DW_AT_type        : <0xa7e>
 <1><a7e>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <a7f>   DW_AT_byte_size   : 4
    <a80>   DW_AT_encoding    : 5	\(signed\)
    <a81>   DW_AT_name        : wchar_t
 <1><a89>: Abbrev Number: 26 \(DW_TAG_subprogram\)
    <a8a>   DW_AT_external    : 1
    <a8a>   DW_AT_name        : f18
    <a8e>   DW_AT_decl_file   : 2
    <a8f>   DW_AT_decl_line   : 127
    <a90>   DW_AT_linkage_name: _Z3f18i
    <a98>   DW_AT_type        : <0xa42>
    <a9c>   DW_AT_low_pc      : \(addr_index: 0x9\): <no .debug_addr section>
    <a9d>   DW_AT_high_pc     : 0x44
    <aa5>   DW_AT_frame_base  : 1 byte block: 9c 	\(DW_OP_call_frame_cfa\)
    <aa7>   DW_AT_GNU_all_call_sites: 1
    <aa7>   DW_AT_sibling     : <0xab8>
 <2><aab>: Abbrev Number: 23 \(DW_TAG_formal_parameter\)
    <aac>   DW_AT_name        : i
    <aae>   DW_AT_decl_file   : 2
    <aaf>   DW_AT_decl_line   : 127
    <ab0>   DW_AT_type        : <0x8b7>
    <ab4>   DW_AT_location    : 2 byte block: 91 6c 	\(DW_OP_fbreg: -20\)
 <2><ab7>: Abbrev Number: 0
 <1><ab8>: Abbrev Number: 27 \(DW_TAG_variable\)
    <ab9>   DW_AT_name        : v2
    <abc>   DW_AT_decl_file   : 2
    <abd>   DW_AT_decl_line   : 43
    <abe>   DW_AT_type        : <0x8b7>
    <ac2>   DW_AT_external    : 1
    <ac2>   DW_AT_location    : 2 byte block: fb a 	\(DW_OP_GNU_addr_index <0xa>\)
 <1><ac5>: Abbrev Number: 27 \(DW_TAG_variable\)
    <ac6>   DW_AT_name        : v3
    <ac9>   DW_AT_decl_file   : 2
    <aca>   DW_AT_decl_line   : 48
    <acb>   DW_AT_type        : <0x8b7>
    <acf>   DW_AT_external    : 1
    <acf>   DW_AT_location    : 2 byte block: fb b 	\(DW_OP_GNU_addr_index <0xb>\)
 <1><ad2>: Abbrev Number: 28 \(DW_TAG_array_type\)
    <ad3>   DW_AT_type        : <0xa4d>
    <ad7>   DW_AT_sibling     : <0xae2>
 <2><adb>: Abbrev Number: 29 \(DW_TAG_subrange_type\)
    <adc>   DW_AT_type        : <0xae2>
    <ae0>   DW_AT_upper_bound : 12
 <2><ae1>: Abbrev Number: 0
 <1><ae2>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <ae3>   DW_AT_byte_size   : 8
    <ae4>   DW_AT_encoding    : 7	\(unsigned\)
    <ae5>   DW_AT_name        : sizetype
 <1><aee>: Abbrev Number: 27 \(DW_TAG_variable\)
    <aef>   DW_AT_name        : v4
    <af2>   DW_AT_decl_file   : 2
    <af3>   DW_AT_decl_line   : 52
    <af4>   DW_AT_type        : <0xad2>
    <af8>   DW_AT_external    : 1
    <af8>   DW_AT_location    : 2 byte block: fb c 	\(DW_OP_GNU_addr_index <0xc>\)
 <1><afb>: Abbrev Number: 27 \(DW_TAG_variable\)
    <afc>   DW_AT_name        : v5
    <aff>   DW_AT_decl_file   : 2
    <b00>   DW_AT_decl_line   : 57
    <b01>   DW_AT_type        : <0xad2>
    <b05>   DW_AT_external    : 1
    <b05>   DW_AT_location    : 2 byte block: fb d 	\(DW_OP_GNU_addr_index <0xd>\)
 <1><b08>: Abbrev Number: 28 \(DW_TAG_array_type\)
    <b09>   DW_AT_type        : <0xa42>
    <b0d>   DW_AT_sibling     : <0xb18>
 <2><b11>: Abbrev Number: 29 \(DW_TAG_subrange_type\)
    <b12>   DW_AT_type        : <0xae2>
    <b16>   DW_AT_upper_bound : 4
 <2><b17>: Abbrev Number: 0
 <1><b18>: Abbrev Number: 27 \(DW_TAG_variable\)
    <b19>   DW_AT_name        : t17data
    <b21>   DW_AT_decl_file   : 2
    <b22>   DW_AT_decl_line   : 119
    <b23>   DW_AT_type        : <0xb08>
    <b27>   DW_AT_external    : 1
    <b27>   DW_AT_location    : 2 byte block: fb e 	\(DW_OP_GNU_addr_index <0xe>\)
 <1><b2a>: Abbrev Number: 0

Contents of the .debug_types.dwo section:

  Compilation Unit @ offset 0x0:
   Length:        0xf7 \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Signature:     0xb534bdc1f01629bb
   Type Offset:   0x25
   Section contributions:
    .debug_abbrev.dwo:       0x0  0x154
    .debug_line.dwo:         0x0  0x40
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x0  0x14
 <0><17>: Abbrev Number: 1 \(DW_TAG_type_unit\)
    <18>   DW_AT_language    : 4	\(C\+\+\)
    <19>   DW_AT_GNU_odr_signature: 0x880a5c4d6e59da8a
    <21>   DW_AT_stmt_list   : 0x0
 <1><25>: Abbrev Number: 2 \(DW_TAG_class_type\)
    <26>   DW_AT_name        : C3
    <29>   DW_AT_byte_size   : 4
    <2a>   DW_AT_decl_file   : 2
    <2b>   DW_AT_decl_line   : 47
    <2c>   DW_AT_sibling     : <0xda>
 <2><30>: Abbrev Number: 3 \(DW_TAG_member\)
    <31>   DW_AT_name        : \(indexed string: 0x3\): member1
    <32>   DW_AT_decl_file   : 2
    <33>   DW_AT_decl_line   : 54
    <34>   DW_AT_type        : <0xda>
    <38>   DW_AT_data_member_location: 0
    <39>   DW_AT_accessibility: 1	\(public\)
 <2><3a>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <3b>   DW_AT_external    : 1
    <3b>   DW_AT_name        : \(indexed string: 0x0\): testcase1
    <3c>   DW_AT_decl_file   : 2
    <3d>   DW_AT_decl_line   : 50
    <3e>   DW_AT_linkage_name: _ZN2C39testcase1Ev
    <51>   DW_AT_type        : <0xe1>
    <55>   DW_AT_accessibility: 1	\(public\)
    <56>   DW_AT_declaration : 1
    <56>   DW_AT_object_pointer: <0x5e>
    <5a>   DW_AT_sibling     : <0x64>
 <3><5e>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <5f>   DW_AT_type        : <0xe9>
    <63>   DW_AT_artificial  : 1
 <3><63>: Abbrev Number: 0
 <2><64>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <65>   DW_AT_external    : 1
    <65>   DW_AT_name        : \(indexed string: 0x1\): testcase2
    <66>   DW_AT_decl_file   : 2
    <67>   DW_AT_decl_line   : 51
    <68>   DW_AT_linkage_name: _ZN2C39testcase2Ev
    <7b>   DW_AT_type        : <0xe1>
    <7f>   DW_AT_accessibility: 1	\(public\)
    <80>   DW_AT_declaration : 1
    <80>   DW_AT_object_pointer: <0x88>
    <84>   DW_AT_sibling     : <0x8e>
 <3><88>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <89>   DW_AT_type        : <0xe9>
    <8d>   DW_AT_artificial  : 1
 <3><8d>: Abbrev Number: 0
 <2><8e>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <8f>   DW_AT_external    : 1
    <8f>   DW_AT_name        : \(indexed string: 0x2\): testcase3
    <90>   DW_AT_decl_file   : 2
    <91>   DW_AT_decl_line   : 52
    <92>   DW_AT_linkage_name: _ZN2C39testcase3Ev
    <a5>   DW_AT_type        : <0xe1>
    <a9>   DW_AT_accessibility: 1	\(public\)
    <aa>   DW_AT_declaration : 1
    <aa>   DW_AT_object_pointer: <0xb2>
    <ae>   DW_AT_sibling     : <0xb8>
 <3><b2>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <b3>   DW_AT_type        : <0xe9>
    <b7>   DW_AT_artificial  : 1
 <3><b7>: Abbrev Number: 0
 <2><b8>: Abbrev Number: 6 \(DW_TAG_subprogram\)
    <b9>   DW_AT_external    : 1
    <b9>   DW_AT_name        : f4
    <bc>   DW_AT_decl_file   : 2
    <bd>   DW_AT_decl_line   : 53
    <be>   DW_AT_linkage_name: _ZN2C32f4Ev
    <ca>   DW_AT_type        : <0xef>
    <ce>   DW_AT_accessibility: 1	\(public\)
    <cf>   DW_AT_declaration : 1
    <cf>   DW_AT_object_pointer: <0xd3>
 <3><d3>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <d4>   DW_AT_type        : <0xe9>
    <d8>   DW_AT_artificial  : 1
 <3><d8>: Abbrev Number: 0
 <2><d9>: Abbrev Number: 0
 <1><da>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <db>   DW_AT_byte_size   : 4
    <dc>   DW_AT_encoding    : 5	\(signed\)
    <dd>   DW_AT_name        : int
 <1><e1>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <e2>   DW_AT_byte_size   : 1
    <e3>   DW_AT_encoding    : 2	\(boolean\)
    <e4>   DW_AT_name        : bool
 <1><e9>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <ea>   DW_AT_byte_size   : 8
    <eb>   DW_AT_type        : <0x25>
 <1><ef>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <f0>   DW_AT_byte_size   : 8
    <f1>   DW_AT_type        : <0xf5>
 <1><f5>: Abbrev Number: 9 \(DW_TAG_subroutine_type\)
    <f6>   DW_AT_type        : <0xe1>
 <1><fa>: Abbrev Number: 0
  Compilation Unit @ offset 0xfb:
   Length:        0xf1 \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Signature:     0xab98c7bc886f5266
   Type Offset:   0x25
   Section contributions:
    .debug_abbrev.dwo:       0x0  0x154
    .debug_line.dwo:         0x0  0x40
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x0  0x14
 <0><112>: Abbrev Number: 1 \(DW_TAG_type_unit\)
    <113>   DW_AT_language    : 4	\(C\+\+\)
    <114>   DW_AT_GNU_odr_signature: 0xae4af0d8bfcef94b
    <11c>   DW_AT_stmt_list   : 0x0
 <1><120>: Abbrev Number: 2 \(DW_TAG_class_type\)
    <121>   DW_AT_name        : C2
    <124>   DW_AT_byte_size   : 4
    <125>   DW_AT_decl_file   : 2
    <126>   DW_AT_decl_line   : 37
    <127>   DW_AT_sibling     : <0x1da>
 <2><12b>: Abbrev Number: 3 \(DW_TAG_member\)
    <12c>   DW_AT_name        : \(indexed string: 0x3\): member1
    <12d>   DW_AT_decl_file   : 2
    <12e>   DW_AT_decl_line   : 44
    <12f>   DW_AT_type        : <0x1da>
    <133>   DW_AT_data_member_location: 0
    <134>   DW_AT_accessibility: 1	\(public\)
 <2><135>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <136>   DW_AT_external    : 1
    <136>   DW_AT_name        : \(indexed string: 0x0\): testcase1
    <137>   DW_AT_decl_file   : 2
    <138>   DW_AT_decl_line   : 40
    <139>   DW_AT_linkage_name: _ZN2C29testcase1Ev
    <14c>   DW_AT_type        : <0x1e1>
    <150>   DW_AT_accessibility: 1	\(public\)
    <151>   DW_AT_declaration : 1
    <151>   DW_AT_object_pointer: <0x159>
    <155>   DW_AT_sibling     : <0x15f>
 <3><159>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <15a>   DW_AT_type        : <0x1e9>
    <15e>   DW_AT_artificial  : 1
 <3><15e>: Abbrev Number: 0
 <2><15f>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <160>   DW_AT_external    : 1
    <160>   DW_AT_name        : \(indexed string: 0x1\): testcase2
    <161>   DW_AT_decl_file   : 2
    <162>   DW_AT_decl_line   : 41
    <163>   DW_AT_linkage_name: _ZN2C29testcase2Ev
    <176>   DW_AT_type        : <0x1e1>
    <17a>   DW_AT_accessibility: 1	\(public\)
    <17b>   DW_AT_declaration : 1
    <17b>   DW_AT_object_pointer: <0x183>
    <17f>   DW_AT_sibling     : <0x189>
 <3><183>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <184>   DW_AT_type        : <0x1e9>
    <188>   DW_AT_artificial  : 1
 <3><188>: Abbrev Number: 0
 <2><189>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <18a>   DW_AT_external    : 1
    <18a>   DW_AT_name        : \(indexed string: 0x2\): testcase3
    <18b>   DW_AT_decl_file   : 2
    <18c>   DW_AT_decl_line   : 42
    <18d>   DW_AT_linkage_name: _ZN2C29testcase3Ev
    <1a0>   DW_AT_type        : <0x1e1>
    <1a4>   DW_AT_accessibility: 1	\(public\)
    <1a5>   DW_AT_declaration : 1
    <1a5>   DW_AT_object_pointer: <0x1ad>
    <1a9>   DW_AT_sibling     : <0x1b3>
 <3><1ad>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <1ae>   DW_AT_type        : <0x1e9>
    <1b2>   DW_AT_artificial  : 1
 <3><1b2>: Abbrev Number: 0
 <2><1b3>: Abbrev Number: 10 \(DW_TAG_subprogram\)
    <1b4>   DW_AT_external    : 1
    <1b4>   DW_AT_name        : \(indexed string: 0x4\): testcase4
    <1b5>   DW_AT_decl_file   : 2
    <1b6>   DW_AT_decl_line   : 43
    <1b7>   DW_AT_linkage_name: _ZN2C29testcase4Ev
    <1ca>   DW_AT_type        : <0x1e1>
    <1ce>   DW_AT_accessibility: 1	\(public\)
    <1cf>   DW_AT_declaration : 1
    <1cf>   DW_AT_object_pointer: <0x1d3>
 <3><1d3>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <1d4>   DW_AT_type        : <0x1e9>
    <1d8>   DW_AT_artificial  : 1
 <3><1d8>: Abbrev Number: 0
 <2><1d9>: Abbrev Number: 0
 <1><1da>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <1db>   DW_AT_byte_size   : 4
    <1dc>   DW_AT_encoding    : 5	\(signed\)
    <1dd>   DW_AT_name        : int
 <1><1e1>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <1e2>   DW_AT_byte_size   : 1
    <1e3>   DW_AT_encoding    : 2	\(boolean\)
    <1e4>   DW_AT_name        : bool
 <1><1e9>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <1ea>   DW_AT_byte_size   : 8
    <1eb>   DW_AT_type        : <0x120>
 <1><1ef>: Abbrev Number: 0
  Compilation Unit @ offset 0x1f0:
   Length:        0x141 \(32-bit\)
   Version:       4
   Abbrev Offset: 0x0
   Pointer Size:  8
   Signature:     0xb5faa2a4b7a919c4
   Type Offset:   0x25
   Section contributions:
    .debug_abbrev.dwo:       0x0  0x154
    .debug_line.dwo:         0x0  0x40
    .debug_loc.dwo:          0x0  0x0
    .debug_str_offsets.dwo:  0x0  0x14
 <0><207>: Abbrev Number: 1 \(DW_TAG_type_unit\)
    <208>   DW_AT_language    : 4	\(C\+\+\)
    <209>   DW_AT_GNU_odr_signature: 0xc7fbeb753b05ade3
    <211>   DW_AT_stmt_list   : 0x0
 <1><215>: Abbrev Number: 2 \(DW_TAG_class_type\)
    <216>   DW_AT_name        : C1
    <219>   DW_AT_byte_size   : 4
    <21a>   DW_AT_decl_file   : 2
    <21b>   DW_AT_decl_line   : 25
    <21c>   DW_AT_sibling     : <0x31f>
 <2><220>: Abbrev Number: 3 \(DW_TAG_member\)
    <221>   DW_AT_name        : \(indexed string: 0x3\): member1
    <222>   DW_AT_decl_file   : 2
    <223>   DW_AT_decl_line   : 34
    <224>   DW_AT_type        : <0x31f>
    <228>   DW_AT_data_member_location: 0
    <229>   DW_AT_accessibility: 1	\(public\)
 <2><22a>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <22b>   DW_AT_external    : 1
    <22b>   DW_AT_name        : \(indexed string: 0x0\): testcase1
    <22c>   DW_AT_decl_file   : 2
    <22d>   DW_AT_decl_line   : 28
    <22e>   DW_AT_linkage_name: _ZN2C19testcase1Ev
    <241>   DW_AT_type        : <0x326>
    <245>   DW_AT_accessibility: 1	\(public\)
    <246>   DW_AT_declaration : 1
    <246>   DW_AT_object_pointer: <0x24e>
    <24a>   DW_AT_sibling     : <0x254>
 <3><24e>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <24f>   DW_AT_type        : <0x32e>
    <253>   DW_AT_artificial  : 1
 <3><253>: Abbrev Number: 0
 <2><254>: Abbrev Number: 11 \(DW_TAG_subprogram\)
    <255>   DW_AT_external    : 1
    <255>   DW_AT_name        : t1a
    <259>   DW_AT_decl_file   : 2
    <25a>   DW_AT_decl_line   : 29
    <25b>   DW_AT_linkage_name: _ZN2C13t1aEv
    <268>   DW_AT_type        : <0x326>
    <26c>   DW_AT_accessibility: 1	\(public\)
    <26d>   DW_AT_declaration : 1
    <26d>   DW_AT_object_pointer: <0x275>
    <271>   DW_AT_sibling     : <0x27b>
 <3><275>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <276>   DW_AT_type        : <0x32e>
    <27a>   DW_AT_artificial  : 1
 <3><27a>: Abbrev Number: 0
 <2><27b>: Abbrev Number: 11 \(DW_TAG_subprogram\)
    <27c>   DW_AT_external    : 1
    <27c>   DW_AT_name        : t1_2
    <281>   DW_AT_decl_file   : 2
    <282>   DW_AT_decl_line   : 30
    <283>   DW_AT_linkage_name: _ZN2C14t1_2Ev
    <291>   DW_AT_type        : <0x31f>
    <295>   DW_AT_accessibility: 1	\(public\)
    <296>   DW_AT_declaration : 1
    <296>   DW_AT_object_pointer: <0x29e>
    <29a>   DW_AT_sibling     : <0x2a4>
 <3><29e>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <29f>   DW_AT_type        : <0x32e>
    <2a3>   DW_AT_artificial  : 1
 <3><2a3>: Abbrev Number: 0
 <2><2a4>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <2a5>   DW_AT_external    : 1
    <2a5>   DW_AT_name        : \(indexed string: 0x1\): testcase2
    <2a6>   DW_AT_decl_file   : 2
    <2a7>   DW_AT_decl_line   : 31
    <2a8>   DW_AT_linkage_name: _ZN2C19testcase2Ev
    <2bb>   DW_AT_type        : <0x326>
    <2bf>   DW_AT_accessibility: 1	\(public\)
    <2c0>   DW_AT_declaration : 1
    <2c0>   DW_AT_object_pointer: <0x2c8>
    <2c4>   DW_AT_sibling     : <0x2ce>
 <3><2c8>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <2c9>   DW_AT_type        : <0x32e>
    <2cd>   DW_AT_artificial  : 1
 <3><2cd>: Abbrev Number: 0
 <2><2ce>: Abbrev Number: 4 \(DW_TAG_subprogram\)
    <2cf>   DW_AT_external    : 1
    <2cf>   DW_AT_name        : \(indexed string: 0x2\): testcase3
    <2d0>   DW_AT_decl_file   : 2
    <2d1>   DW_AT_decl_line   : 32
    <2d2>   DW_AT_linkage_name: _ZN2C19testcase3Ev
    <2e5>   DW_AT_type        : <0x326>
    <2e9>   DW_AT_accessibility: 1	\(public\)
    <2ea>   DW_AT_declaration : 1
    <2ea>   DW_AT_object_pointer: <0x2f2>
    <2ee>   DW_AT_sibling     : <0x2f8>
 <3><2f2>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <2f3>   DW_AT_type        : <0x32e>
    <2f7>   DW_AT_artificial  : 1
 <3><2f7>: Abbrev Number: 0
 <2><2f8>: Abbrev Number: 10 \(DW_TAG_subprogram\)
    <2f9>   DW_AT_external    : 1
    <2f9>   DW_AT_name        : \(indexed string: 0x4\): testcase4
    <2fa>   DW_AT_decl_file   : 2
    <2fb>   DW_AT_decl_line   : 33
    <2fc>   DW_AT_linkage_name: _ZN2C19testcase4Ev
    <30f>   DW_AT_type        : <0x326>
    <313>   DW_AT_accessibility: 1	\(public\)
    <314>   DW_AT_declaration : 1
    <314>   DW_AT_object_pointer: <0x318>
 <3><318>: Abbrev Number: 5 \(DW_TAG_formal_parameter\)
    <319>   DW_AT_type        : <0x32e>
    <31d>   DW_AT_artificial  : 1
 <3><31d>: Abbrev Number: 0
 <2><31e>: Abbrev Number: 0
 <1><31f>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <320>   DW_AT_byte_size   : 4
    <321>   DW_AT_encoding    : 5	\(signed\)
    <322>   DW_AT_name        : int
 <1><326>: Abbrev Number: 7 \(DW_TAG_base_type\)
    <327>   DW_AT_byte_size   : 1
    <328>   DW_AT_encoding    : 2	\(boolean\)
    <329>   DW_AT_name        : bool
 <1><32e>: Abbrev Number: 8 \(DW_TAG_pointer_type\)
    <32f>   DW_AT_byte_size   : 8
    <330>   DW_AT_type        : <0x215>
 <1><334>: Abbrev Number: 0

#pass
