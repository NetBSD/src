SECTIONS
{
  . = 0x80000;
  A = .;
  .text :
  {
    _start = .;
    . = 0x10000;
  }
  B = .;
  .data :
  {
    . = 0x10000;
  }
  C = .;
  .bss :
  {
    . = 0x10000;
  }
  D = A - C + B;
  E = A + B - C;
  /DISCARD/ : {*(*)}
}

ASSERT(D == E, "Addition is not commutative");
