PARSE_AND_LIST_OPTIONS='
  fprintf (file, _("\
  -z noextern-protected-data  Do not treat protected data symbol as external\n"));
'

PARSE_AND_LIST_ARGS_CASE_Z='
      else if (strcmp (optarg, "noextern-protected-data") == 0)
	link_info.extern_protected_data = FALSE;
'
