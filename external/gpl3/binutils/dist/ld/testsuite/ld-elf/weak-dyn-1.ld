SECTIONS
{
  . = 0x800000;
  PROVIDE (bar = .);
  .data : {
    *(.data)
  }
}
