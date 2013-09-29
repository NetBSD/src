PARSE_AND_LIST_PROLOGUE='
#define OPTION_LD_GENERATED_UNWIND_INFO	301
#define OPTION_NO_LD_GENERATED_UNWIND_INFO 302
'

PARSE_AND_LIST_LONGOPTS='
  {"ld-generated-unwind-info", no_argument, NULL,
   OPTION_LD_GENERATED_UNWIND_INFO},
  {"no-ld-generated-unwind-info", no_argument, NULL,
   OPTION_NO_LD_GENERATED_UNWIND_INFO},
'

PARSE_AND_LIST_OPTIONS='
  fprintf (file, _("\
  --ld-generated-unwind-info  Generate exception handling info for PLT.\n\
  --no-ld-generated-unwind-info Don'\''t do so.\n"
		   ));
'

PARSE_AND_LIST_ARGS_CASES='
    case OPTION_LD_GENERATED_UNWIND_INFO:
      link_info.no_ld_generated_unwind_info = FALSE;
      break;

    case OPTION_NO_LD_GENERATED_UNWIND_INFO:
      link_info.no_ld_generated_unwind_info = TRUE;
      break;
'
