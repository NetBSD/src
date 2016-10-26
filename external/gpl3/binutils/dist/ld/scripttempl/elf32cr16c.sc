# Linker Script for National Semiconductor's CR16C-ELF32.
#
# Copyright (C) 2014-2016 Free Software Foundation, Inc.
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

test -z "$ENTRY" && ENTRY=_start
cat <<EOF

/* Example Linker Script for linking NS CR16C or CR16CPlus
   elf32 files, which were compiled with either the near data
   model or the default data model.

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */


${RELOCATING+ENTRY(${ENTRY})}

MEMORY
{
  near_rom  : ORIGIN = 0x4,     LENGTH = 512K - 4
  near_ram  : ORIGIN = 512K,    LENGTH = 512K - 64K
  rom  	    : ORIGIN = 1M,      LENGTH = 3M
  ram 	    : ORIGIN = 4M,      LENGTH = 10M
}

SECTIONS
{
/* The heap is located in near memory, to suit both the near and
   default data models.  The heap and stack are aligned to the bus
   width, as a speed optimization for accessing  data located
   there. The alignment to 4 bytes is compatible for both the CR16C
   bus width (2 bytes) and CR16CPlus bus width (4 bytes).  */

  .text            : { __TEXT_START = .;   *(.text)                                        __TEXT_END = .; } > rom	
  .rdata           : { __RDATA_START = .;  *(.rdata_4) *(.rdata_2) *(.rdata_1)             __RDATA_END = .; } > near_rom
  .ctor ALIGN(4)   : { __CTOR_LIST = .;    *(.ctors)                                       __CTOR_END = .; } > near_rom
  .dtor ALIGN(4)   : { __DTOR_LIST = .;    *(.dtors)                                       __DTOR_END = .; } > near_rom
  .data            : { __DATA_START = .;   *(.data_4) *(.data_2) *(.data_1) *(.data)       __DATA_END = .; } > ram AT > rom
  .bss (NOLOAD)    : { __BSS_START = .;    *(.bss_4) *(.bss_2) *(.bss_1) *(.bss) *(COMMON) __BSS_END = .; } > ram
  .nrdata          : { __NRDATA_START = .; *(.nrdat_4) *(.nrdat_2) *(.nrdat_1)             __NRDATA_END =  .; } > near_rom
  .ndata           : { __NDATA_START = .;  *(.ndata_4) *(.ndata_2) *(.ndata_1)             __NDATA_END = .; } > near_ram AT > rom
  .nbss (NOLOAD)   : { __NBSS_START = .;   *(.nbss_4) *(.nbss_2) *(.nbss_1) *(.ncommon)    __NBSS_END = .; } > near_ram
  .heap (NOLOAD)   : { . = ALIGN(4); __HEAP_START = .; . += 0x2000;                        __HEAP_MAX = .; } > near_ram
  .stack (NOLOAD)  : { . = ALIGN(4); . += 0x6000; __STACK_START = .; } > ram
  .istack (NOLOAD) : { . = ALIGN(2); . += 0x100; __ISTACK_START = .; } > ram
}

__DATA_IMAGE_START = LOADADDR(.data);
__NDATA_IMAGE_START = LOADADDR(.ndata);

EOF
