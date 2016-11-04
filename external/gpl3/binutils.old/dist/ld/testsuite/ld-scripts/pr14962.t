NOCROSSREFS ( .text .data )
x = ABSOLUTE(x);
SECTIONS
{ 
  .text : { *(.text) }
  .data : { *(.data) }
  /DISCARD/ : { *(*) }
}
