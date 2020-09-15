PARSE_AND_LIST_OPTIONS_CET='
  fprintf (file, _("\
  -z ibtplt                   Generate IBT-enabled PLT entries\n"));
  fprintf (file, _("\
  -z ibt                      Generate GNU_PROPERTY_X86_FEATURE_1_IBT\n"));
  fprintf (file, _("\
  -z shstk                    Generate GNU_PROPERTY_X86_FEATURE_1_SHSTK\n"));
  fprintf (file, _("\
  -z cet-report=[none|warning|error] (default: none)\n\
                              Report missing IBT and SHSTK properties\n"));
'
PARSE_AND_LIST_ARGS_CASE_Z_CET='
      else if (strcmp (optarg, "ibtplt") == 0)
	params.ibtplt = TRUE;
      else if (strcmp (optarg, "ibt") == 0)
	params.ibt = TRUE;
      else if (strcmp (optarg, "shstk") == 0)
	params.shstk = TRUE;
      else if (strncmp (optarg, "cet-report=", 11) == 0)
	{
	  if (strcmp (optarg + 11, "none") == 0)
	    params.cet_report = cet_report_none;
	  else if (strcmp (optarg + 11, "warning") == 0)
	    params.cet_report = (cet_report_warning
				 | cet_report_ibt
				 | cet_report_shstk);
	  else if (strcmp (optarg + 11, "error") == 0)
	    params.cet_report = (cet_report_error
				 | cet_report_ibt
				 | cet_report_shstk);
	  else
	    einfo (_("%F%P: invalid option for -z cet-report=: %s\n"),
		   optarg + 11);
	}
'

PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_CET"
PARSE_AND_LIST_ARGS_CASE_Z="$PARSE_AND_LIST_ARGS_CASE_Z $PARSE_AND_LIST_ARGS_CASE_Z_CET"
