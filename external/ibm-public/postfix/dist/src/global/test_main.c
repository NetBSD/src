/*	$NetBSD: test_main.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	test_main 3
/* SUMMARY
/*	test main program
/* SYNOPSIS
/*	#include <test_main.h>
/*
/*	NORETURN test_main(argc, argv, test_driver, key, value, ...)
/*	int	argc;
/*	char	**argv;
/*	void	(*test_driver)(int argc, char **argv);
/*	int	key;
/* DESCRIPTION
/*	This module implements a test main program for stand-alone
/*	module tests.
/*
/*	test_main() should be called from a main program. It does
/*	generic command-line options processing, and initializes
/*	configurable parameters. After calling the test_driver()
/*	function, the test_main() function terminates.
/*
/*	Arguments:
/* .IP "void (*test_driver)(int argc, char **argv)"
/*	A pointer to a function that is called after processing
/*	command-line options and initializing configuration parameters.
/*	The argc and argv specify the process name and non-option
/*	command-line arguments.
/* .PP
/*	Optional test_main() arguments are specified as a null-terminated
/*	list with macros that have zero or more arguments:
/* .IP "CA_TEST_MAIN_INT_TABLE(CONFIG_INT_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_TEST_MAIN_LONG_TABLE(CONFIG_LONG_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_TEST_MAIN_STR_TABLE(CONFIG_STR_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_TEST_MAIN_BOOL_TABLE(CONFIG_BOOL_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_TEST_MAIN_TIME_TABLE(CONFIG_TIME_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_TEST_MAIN_RAW_TABLE(CONFIG_RAW_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed. Raw parameters are not subjected to $name
/*	evaluation.
/* .IP "CA_TEST_MAIN_NINT_TABLE(CONFIG_NINT_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_TEST_MAIN_NBOOL_TABLE(CONFIG_NBOOL_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* DIAGNOSTICS
/*	Problems and transactions are logged stderr.
/* BUGS
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <dict.h>
#include <msg.h>
#include <msg_vstream.h>
#include <mymalloc.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_conf.h>
#include <mail_dict.h>
#include <mail_task.h>
#include <mail_version.h>

 /*
  * Test library.
  */
#include <test_main.h>

/* test_driver_main - the real main program */

NORETURN test_main(int argc, char **argv, TEST_DRIVER_FN test_driver,...)
{
    const char *myname = "test_driver_main";
    va_list ap;
    int     ch;
    int     key;
    int     test_driver_argc;
    char  **test_driver_argv;

    /*
     * Set up logging.
     */
    var_procname = mystrdup(basename(argv[0]));
    msg_vstream_init(mail_task(var_procname), VSTREAM_ERR);

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "c:v")) > 0) {
	switch (ch) {
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    msg_fatal("invalid option: %c. Usage: %s [-c config_dir] [-v]",
		      optopt, argv[0]);
	    break;
	}
    }

    /*
     * Initialize generic parameters.
     */
    set_mail_conf_str(VAR_PROCNAME, var_procname);
    set_mail_conf_str(VAR_SERVNAME, var_procname);
    mail_conf_read();

    /*
     * Register higher-level dictionaries and initialize the support for
     * dynamically-loaded dictionaries.
     */
    mail_dict_init();

    /*
     * Application-specific initialization.
     */
    va_start(ap, test_driver);
    while ((key = va_arg(ap, int)) != 0) {
	switch (key) {
	case TEST_MAIN_INT_TABLE:
	    get_mail_conf_int_table(va_arg(ap, CONFIG_INT_TABLE *));
	    break;
	case TEST_MAIN_LONG_TABLE:
	    get_mail_conf_long_table(va_arg(ap, CONFIG_LONG_TABLE *));
	    break;
	case TEST_MAIN_STR_TABLE:
	    get_mail_conf_str_table(va_arg(ap, CONFIG_STR_TABLE *));
	    break;
	case TEST_MAIN_BOOL_TABLE:
	    get_mail_conf_bool_table(va_arg(ap, CONFIG_BOOL_TABLE *));
	    break;
	case TEST_MAIN_TIME_TABLE:
	    get_mail_conf_time_table(va_arg(ap, CONFIG_TIME_TABLE *));
	    break;
	case TEST_MAIN_RAW_TABLE:
	    get_mail_conf_raw_table(va_arg(ap, CONFIG_RAW_TABLE *));
	    break;
	case TEST_MAIN_NINT_TABLE:
	    get_mail_conf_nint_table(va_arg(ap, CONFIG_NINT_TABLE *));
	    break;
	case TEST_MAIN_NBOOL_TABLE:
	    get_mail_conf_nbool_table(va_arg(ap, CONFIG_NBOOL_TABLE *));
	    break;
	default:
	    msg_panic("%s: unknown argument type: %d", myname, key);
	}
    }
    va_end(ap);

    /*
     * Set up call-back info.
     */
    test_driver_argv = argv + optind - 1;
    if (test_driver_argv != argv)
	test_driver_argv[0] = argv[0];
    test_driver_argc = argc - optind + 1;

    /*
     * Call the test driver and terminate (if they didn't terminate already).
     */
    test_driver(test_driver_argc, test_driver_argv);
    exit(0);
}
