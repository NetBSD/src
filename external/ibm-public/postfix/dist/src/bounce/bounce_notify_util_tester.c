/*	$NetBSD: bounce_notify_util_tester.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <sys/stat.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <dsn_mask.h>
#include <mail_params.h>
#include <record.h>
#include <rec_type.h>
#include <hfrom_format.h>

 /*
  * Bounce service.
  */
#include <bounce_service.h>
#include <bounce_template.h>

 /*
  * Testing library.
  */
#include <test_main.h>

#define TEST_ENCODING	"7bit"
#define NO_SMTPUTF8	(0)
#define TEST_DSN_ENVID	"TEST-ENVID"
#define TEST_RECIPIENT	"test-recipient"

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* test_driver - test driver */

static void test_driver(int argc, char **argv)
{
    BOUNCE_TEMPLATES *bounce_templates;
    BOUNCE_INFO *bounce_info;
    VSTRING *message_buf;
    VSTREAM *message_stream;
    VSTRING *rec_buf;
    int     rec_type;

    /*
     * Sanity checks.
     */
    if (argc != 4)
	msg_fatal("usage: %s [options] service queue_name queue_id", argv[0]);

    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    bounce_hfrom_format = 
	hfrom_format_parse(VAR_HFROM_FORMAT, var_hfrom_format);

    /*
     * Write one message to VSTRING.
     */
    message_buf = vstring_alloc(100);
    if ((message_stream = vstream_memopen(message_buf, O_WRONLY)) == 0)
	msg_fatal("vstream_memopen O_WRONLY: %m");
    bounce_templates = bounce_templates_create();
    bounce_info = bounce_mail_init(argv[1], argv[2], argv[3],
				 TEST_ENCODING, NO_SMTPUTF8, TEST_DSN_ENVID,
				   bounce_templates->failure);
    if (bounce_header(message_stream, bounce_info, TEST_RECIPIENT,
		      NO_POSTMASTER_COPY) < 0)
	msg_fatal("bounce_header: %m");
    if (bounce_diagnostic_log(message_stream, bounce_info,
			      DSN_NOTIFY_OVERRIDE) <= 0)
	msg_fatal("bounce_diagnostic_log: %m");
    if (bounce_header_dsn(message_stream, bounce_info) != 0)
	msg_fatal("bounce_header_dsn: %m");
    if (bounce_diagnostic_dsn(message_stream, bounce_info,
			      DSN_NOTIFY_OVERRIDE) <= 0)
	msg_fatal("bounce_diagnostic_dsn: %m");
    bounce_original(message_stream, bounce_info, DSN_RET_FULL);
    if (vstream_fclose(message_stream) != 0)
	msg_fatal("vstream_fclose: %m");
    bounce_mail_free(bounce_info);

    /*
     * Render the bounce message in human-readable form.
     */
    if ((message_stream = vstream_memopen(message_buf, O_RDONLY)) == 0)
	msg_fatal("vstream_memopen O_RDONLY: %m");
    rec_buf = vstring_alloc(100);
    while ((rec_type = rec_get(message_stream, rec_buf, 0)) > 0) {
	switch (rec_type) {
	case REC_TYPE_CONT:
	    vstream_printf("%.*s", (int) LEN(rec_buf), STR(rec_buf));
	    break;
	case REC_TYPE_NORM:
	    vstream_printf("%.*s\n", (int) LEN(rec_buf), STR(rec_buf));
	    break;
	default:
	    msg_panic("unexpected message record type %d", rec_type);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    if (vstream_fclose(message_stream) != 0)
	msg_fatal("vstream_fclose: %m");
    vstring_free(rec_buf);

    /*
     * Clean up.
     */
    exit(0);
}

int     var_bounce_limit;
int     var_max_queue_time;
int     var_delay_warn_time;
char   *var_notify_classes;
char   *var_bounce_rcpt;
char   *var_2bounce_rcpt;
char   *var_delay_rcpt;
char   *var_bounce_tmpl;
bool    var_threaded_bounce;
char   *var_hfrom_format;		/* header_from_format */

int     bounce_hfrom_format;

int     main(int argc, char **argv)
{
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_BOUNCE_LIMIT, DEF_BOUNCE_LIMIT, &var_bounce_limit, 1, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_MAX_QUEUE_TIME, DEF_MAX_QUEUE_TIME, &var_max_queue_time, 0, 8640000,
	VAR_DELAY_WARN_TIME, DEF_DELAY_WARN_TIME, &var_delay_warn_time, 0, 0,
	0,
    };
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_BOUNCE_RCPT, DEF_BOUNCE_RCPT, &var_bounce_rcpt, 1, 0,
	VAR_2BOUNCE_RCPT, DEF_2BOUNCE_RCPT, &var_2bounce_rcpt, 1, 0,
	VAR_DELAY_RCPT, DEF_DELAY_RCPT, &var_delay_rcpt, 1, 0,
	VAR_BOUNCE_TMPL, DEF_BOUNCE_TMPL, &var_bounce_tmpl, 0, 0,
	VAR_HFROM_FORMAT, DEF_HFROM_FORMAT, &var_hfrom_format, 1, 0,
	0,
    };
    static const CONFIG_NBOOL_TABLE nbool_table[] = {
	VAR_THREADED_BOUNCE, DEF_THREADED_BOUNCE, &var_threaded_bounce,
	0,
    };

    test_main(argc, argv, test_driver,
	      CA_TEST_MAIN_INT_TABLE(int_table),
	      CA_TEST_MAIN_STR_TABLE(str_table),
	      CA_TEST_MAIN_TIME_TABLE(time_table),
	      CA_TEST_MAIN_NBOOL_TABLE(nbool_table),
	      0);

    exit(0);
}
