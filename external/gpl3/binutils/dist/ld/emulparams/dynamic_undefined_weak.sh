PARSE_AND_LIST_OPTIONS_NODYNAMIC_UNDEFINED_WEAK='
  fprintf (file, _("\
  -z nodynamic-undefined-weak Do not treat undefined weak symbol as dynamic\n"));
'

PARSE_AND_LIST_ARGS_CASE_Z_NODYNAMIC_UNDEFINED_WEAK='
      else if (strcmp (optarg, "nodynamic-undefined-weak") == 0)
	link_info.dynamic_undefined_weak = FALSE;
'

PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_NODYNAMIC_UNDEFINED_WEAK"
PARSE_AND_LIST_ARGS_CASE_Z="$PARSE_AND_LIST_ARGS_CASE_Z $PARSE_AND_LIST_ARGS_CASE_Z_NODYNAMIC_UNDEFINED_WEAK"
