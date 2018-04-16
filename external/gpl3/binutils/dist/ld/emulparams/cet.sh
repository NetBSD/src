PARSE_AND_LIST_OPTIONS_CET='
  fprintf (file, _("\
  -z ibtplt                   Generate IBT-enabled PLT entries\n\
  -z ibt                      Generate GNU_PROPERTY_X86_FEATURE_1_IBT\n\
  -z shstk                    Generate GNU_PROPERTY_X86_FEATURE_1_SHSTK\n"));
'
PARSE_AND_LIST_ARGS_CASE_Z_CET='
      else if (strcmp (optarg, "ibtplt") == 0)
	link_info.ibtplt = TRUE;
      else if (strcmp (optarg, "ibt") == 0)
	link_info.ibt = TRUE;
      else if (strcmp (optarg, "shstk") == 0)
	link_info.shstk = TRUE;
'

PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_CET"
PARSE_AND_LIST_ARGS_CASE_Z="$PARSE_AND_LIST_ARGS_CASE_Z $PARSE_AND_LIST_ARGS_CASE_Z_CET"
