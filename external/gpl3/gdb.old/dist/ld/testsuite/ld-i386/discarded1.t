ENTRY(_start)
SECTIONS
{
  .text : {*(.text)}
  .data : {*(.data)}
  /DISCARD/ : {*(*)}
}
