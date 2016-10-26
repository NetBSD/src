SECTIONS
{
  .text :
  {
    *(.text)
    *(.pr)
  }

  .data :
  {
    *(.data)
    *(.rw)
  }
}
