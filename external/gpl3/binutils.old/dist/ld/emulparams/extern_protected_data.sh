PARSE_AND_LIST_OPTIONS_NOEXTEN_PROTECTED_DATA='
  fprintf (file, _("\
  -z noextern-protected-data  Do not treat protected data symbol as external\n"));
'

PARSE_AND_LIST_ARGS_CASE_Z_NOEXTEN_PROTECTED_DATA='
      else if (strcmp (optarg, "noextern-protected-data") == 0)
	link_info.extern_protected_data = FALSE;
'


PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_NOEXTEN_PROTECTED_DATA"
PARSE_AND_LIST_ARGS_CASE_Z="$PARSE_AND_LIST_ARGS_CASE_Z $PARSE_AND_LIST_ARGS_CASE_Z_NOEXTEN_PROTECTED_DATA"
