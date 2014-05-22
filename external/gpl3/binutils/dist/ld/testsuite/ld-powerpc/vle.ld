SECTIONS
{
  . = 0x01800000 + SIZEOF_HEADERS;
  .text : { *(.text) }
  .PPC.EMB.sdata0 : { *(.PPC.EMB.sdata0) }
  .sdata2 : { PROVIDE (_SDA2_BASE_ = 32768); *(.sdata2) }
  . = ALIGN (0x10000) + (. & (0x10000 - 1));
  .data : { *(.data) }
  .sdata : { PROVIDE (_SDA_BASE_ = 32768); *(.sdata) }
  /DISCARD/ : { *(*) }
}
