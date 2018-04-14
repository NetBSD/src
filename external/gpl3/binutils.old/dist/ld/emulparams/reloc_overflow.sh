PARSE_AND_LIST_OPTIONS_RELOC_OVERFLOW='
  fprintf (file, _("\
  -z noreloc-overflow         Disable relocation overflow check\n"));
'
PARSE_AND_LIST_ARGS_CASE_Z_RELOC_OVERFLOW='
      else if (strcmp (optarg, "noreloc-overflow") == 0)
	link_info.no_reloc_overflow_check = TRUE;
'

PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_RELOC_OVERFLOW"
PARSE_AND_LIST_ARGS_CASE_Z="$PARSE_AND_LIST_ARGS_CASE_Z $PARSE_AND_LIST_ARGS_CASE_Z_RELOC_OVERFLOW"
