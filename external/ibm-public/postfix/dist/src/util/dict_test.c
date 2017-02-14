/*	$NetBSD: dict_test.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

 /*
  * Proof-of-concept test program. Create, update or read a database. Type
  * '?' for a list of commands.
  */

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <stringops.h>
#include <vstring.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <dict.h>
#include <dict_lmdb.h>
#include <dict_db.h>

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s type:file read|write|create [flags...]", myname);
}

void    dict_test(int argc, char **argv)
{
    VSTRING *keybuf = vstring_alloc(1);
    VSTRING *inbuf = vstring_alloc(1);
    DICT   *dict;
    char   *dict_name;
    int     open_flags;
    char   *bufp;
    char   *cmd;
    const char *key;
    const char *value;
    int     ch;
    int     dict_flags = 0;
    int     n;
    int     rc;

#define USAGE	"verbose|del key|get key|put key=value|first|next|masks|flags"

    signal(SIGPIPE, SIG_IGN);

    msg_vstream_init(argv[0], VSTREAM_ERR);
    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	default:
	    usage(argv[0]);
	case 'v':
	    msg_verbose++;
	    break;
	}
    }
    optind = OPTIND;
    if (argc - optind < 2)
	usage(argv[0]);
    if (strcasecmp(argv[optind + 1], "create") == 0)
	open_flags = O_CREAT | O_RDWR | O_TRUNC;
    else if (strcasecmp(argv[optind + 1], "write") == 0)
	open_flags = O_RDWR;
    else if (strcasecmp(argv[optind + 1], "read") == 0)
	open_flags = O_RDONLY;
    else
	msg_fatal("unknown access mode: %s", argv[2]);
    for (n = 2; argv[optind + n]; n++)
	dict_flags |= dict_flags_mask(argv[optind + 2]);
    if ((dict_flags & DICT_FLAG_OPEN_LOCK) == 0)
	dict_flags |= DICT_FLAG_LOCK;
    if ((dict_flags & (DICT_FLAG_DUP_WARN | DICT_FLAG_DUP_IGNORE)) == 0)
	dict_flags |= DICT_FLAG_DUP_REPLACE;
    dict_flags |= DICT_FLAG_UTF8_REQUEST;
    vstream_fflush(VSTREAM_OUT);
    dict_name = argv[optind];
    dict_allow_surrogate = 1;
    util_utf8_enable = 1;
    dict = dict_open(dict_name, open_flags, dict_flags);
    dict_register(dict_name, dict);
    vstream_printf("owner=%s (uid=%ld)\n",
		   dict->owner.status == DICT_OWNER_TRUSTED ? "trusted" :
		   dict->owner.status == DICT_OWNER_UNTRUSTED ? "untrusted" :
		   dict->owner.status == DICT_OWNER_UNKNOWN ? "unspecified" :
		   "error", (long) dict->owner.uid);
    vstream_fflush(VSTREAM_OUT);

    while (vstring_fgets_nonl(inbuf, VSTREAM_IN)) {
	bufp = vstring_str(inbuf);
	if (!isatty(0)) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#')
	    continue;
	if ((cmd = mystrtok(&bufp, " ")) == 0) {
	    vstream_printf("usage: %s\n", USAGE);
	    vstream_fflush(VSTREAM_OUT);
	    continue;
	}
	if (dict_changed_name())
	    msg_warn("dictionary has changed");
	key = *bufp ? vstring_str(unescape(keybuf, mystrtok(&bufp, " ="))) : 0;
	value = mystrtok(&bufp, " =");
	if (strcmp(cmd, "verbose") == 0 && !key) {
	    msg_verbose++;
	} else if (strcmp(cmd, "del") == 0 && key && !value) {
	    if ((rc = dict_del(dict, key)) > 0)
		vstream_printf("%s: not found\n", key);
	    else if (rc < 0)
		vstream_printf("%s: error\n", key);
	    else
		vstream_printf("%s: deleted\n", key);
	} else if (strcmp(cmd, "get") == 0 && key && !value) {
	    if ((value = dict_get(dict, key)) == 0) {
		vstream_printf("%s: %s\n", key, dict->error ?
			       "error" : "not found");
	    } else {
		vstream_printf("%s=%s\n", key, value);
	    }
	} else if (strcmp(cmd, "put") == 0 && key && value) {
	    if (dict_put(dict, key, value) != 0)
		vstream_printf("%s: %s\n", key, dict->error ?
			       "error" : "not updated");
	} else if (strcmp(cmd, "first") == 0 && !key && !value) {
	    if (dict_seq(dict, DICT_SEQ_FUN_FIRST, &key, &value) == 0)
		vstream_printf("%s=%s\n", key, value);
	    else
		vstream_printf("%s\n", dict->error ?
			       "error" : "not found");
	} else if (strcmp(cmd, "next") == 0 && !key && !value) {
	    if (dict_seq(dict, DICT_SEQ_FUN_NEXT, &key, &value) == 0)
		vstream_printf("%s=%s\n", key, value);
	    else
		vstream_printf("%s\n", dict->error ?
			       "error" : "not found");
	} else if (strcmp(cmd, "flags") == 0 && !key && !value) {
	    vstream_printf("dict flags %s\n",
			   dict_flags_str(dict->flags));
	} else if (strcmp(cmd, "masks") == 0 && !key && !value) {
	    vstream_printf("DICT_FLAG_IMPL_MASK %s\n",
			   dict_flags_str(DICT_FLAG_IMPL_MASK));
	    vstream_printf("DICT_FLAG_PARANOID %s\n",
			   dict_flags_str(DICT_FLAG_PARANOID));
	    vstream_printf("DICT_FLAG_RQST_MASK %s\n",
			   dict_flags_str(DICT_FLAG_RQST_MASK));
	    vstream_printf("DICT_FLAG_INST_MASK %s\n",
			   dict_flags_str(DICT_FLAG_INST_MASK));
	} else {
	    vstream_printf("usage: %s\n", USAGE);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(keybuf);
    vstring_free(inbuf);
    dict_close(dict);
}
