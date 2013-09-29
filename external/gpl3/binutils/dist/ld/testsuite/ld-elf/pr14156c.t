SECTIONS {
  .foo : { *(SORT_NONE(.foo)) }
  /DISCARD/ : { *(.*) }
}
