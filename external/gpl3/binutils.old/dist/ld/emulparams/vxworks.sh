# If you change this file, please also look at files which source this one:
# armelf_vxworks.sh elf32ebmipvxworks.sh elf32elmipvxworks.sh
# elf_i386_vxworks.sh elf32ppcvxworks.sh elf32_sparc_vxworks.sh
# shelf_vxworks.sh

# The Diab tools use a different init/fini convention.  Initialization code
# is place in sections named ".init$NN".  These sections are then concatenated
# into the .init section.  It is important that .init$00 be first and .init$99
# be last. The other sections should be sorted, but the current linker script
# parse does not seem to allow that with the SORT keyword in this context.
INIT_START='_init = .;
	    KEEP (*(.init$00));
	    KEEP (*(.init$0[1-9]));
	    KEEP (*(.init$[1-8][0-9]));
	    KEEP (*(.init$9[0-8]));'
INIT_END='KEEP (*(.init$99));'
FINI_START='_fini = .;
	    KEEP (*(.fini$00));
	    KEEP (*(.fini$0[1-9]));
	    KEEP (*(.fini$[1-8][0-9]));
	    KEEP (*(.fini$9[0-8]));'
FINI_END="KEEP (*(.fini\$99));
	  PROVIDE (${SYMPREFIX}_etext = .);"

OTHER_READWRITE_SECTIONS=".tls_data ${RELOCATING-0} : {${RELOCATING+
    __wrs_rtp_tls_data_start = .;
    ___wrs_rtp_tls_data_start = .;}
    *(.tls_data${RELOCATING+ .tls_data.*})
  }${RELOCATING+
  __wrs_rtp_tls_data_size = . - __wrs_rtp_tls_data_start;
  ___wrs_rtp_tls_data_size = . - __wrs_rtp_tls_data_start;
  __wrs_rtp_tls_data_align = ALIGNOF(.tls_data);
  ___wrs_rtp_tls_data_align = ALIGNOF(.tls_data);}
  .tls_vars ${RELOCATING-0} : {${RELOCATING+
    __wrs_rtp_tls_vars_start = .;
    ___wrs_rtp_tls_vars_start = .;}
    *(.tls_vars${RELOCATING+ .tls_vars.*})
  }${RELOCATING+
  __wrs_rtp_tls_vars_size = SIZEOF(.tls_vars);
  ___wrs_rtp_tls_vars_size = SIZEOF(.tls_vars);}"

TEXT_START_ADDR="(DEFINED (__wrs_rtp_base) ? __wrs_rtp_base : 0)"
ETEXT_NAME=etext_unrelocated
OTHER_END_SYMBOLS="PROVIDE (${SYMPREFIX}_ehdr = ${TEXT_START_ADDR});"
DATA_END_SYMBOLS=".edata : { PROVIDE (${SYMPREFIX}_edata = .); }"
VXWORKS_BASE_EM_FILE=$EXTRA_EM_FILE
EXTRA_EM_FILE=vxworks
unset EMBEDDED
