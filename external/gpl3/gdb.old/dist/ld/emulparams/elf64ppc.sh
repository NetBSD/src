source_sh ${srcdir}/emulparams/elf32ppccommon.sh
source_sh ${srcdir}/emulparams/plt_unwind.sh
EXTRA_EM_FILE=ppc64elf
ELFSIZE=64
OUTPUT_FORMAT="elf64-powerpc"
TEXT_START_ADDR=0x10000000
#SEGMENT_SIZE=0x10000000
ARCH=powerpc:common64
unset EXECUTABLE_SYMBOLS
unset SDATA_START_SYMBOLS
unset SDATA2_START_SYMBOLS
unset SBSS_START_SYMBOLS
unset SBSS_END_SYMBOLS
unset OTHER_END_SYMBOLS
unset OTHER_RELRO_SECTIONS
OTHER_TEXT_SECTIONS="*(.sfpr .glink)"
OTHER_SDATA_SECTIONS="
  .tocbss	${RELOCATING-0} :${RELOCATING+ ALIGN(8)} { *(.tocbss)}"

if test x${RELOCATING+set} = xset; then
  GOT="
  .got		: ALIGN(256) { *(.got .toc) }"
else
  GOT="
  .got		0 : { *(.got) }
  .toc		0 : { *(.toc) }"
fi
# Put .opd relocs first so ld.so will process them before any ifunc relocs.
INITIAL_RELOC_SECTIONS="
  .rela.opd	${RELOCATING-0} : { *(.rela.opd) }"
OTHER_GOT_RELOC_SECTIONS="
  .rela.toc	${RELOCATING-0} : { *(.rela.toc) }
  .rela.toc1	${RELOCATING-0} : { *(.rela.toc1) }
  .rela.tocbss	${RELOCATING-0} : { *(.rela.tocbss) }
  .rela.branch_lt	${RELOCATING-0} : { *(.rela.branch_lt) }"
OTHER_RELRO_SECTIONS_2="
  .opd		${RELOCATING-0} :${RELOCATING+ ALIGN(8)} { KEEP (*(.opd)) }
  .toc1		${RELOCATING-0} :${RELOCATING+ ALIGN(8)} { *(.toc1) }
  .branch_lt	${RELOCATING-0} :${RELOCATING+ ALIGN(8)} { *(.branch_lt) }"
INITIAL_READWRITE_SECTIONS="
  .toc		${RELOCATING-0} :${RELOCATING+ ALIGN(8)} { *(.toc) }"
# Put .got before .data
DATA_GOT=" "
# Always make .got read-only after relocation
SEPARATE_GOTPLT=0
# Also put .sdata before .data
DATA_SDATA=" "
# and .plt/.iplt before .data
DATA_PLT=
PLT_BEFORE_GOT=" "
