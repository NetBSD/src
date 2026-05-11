/*	$NetBSD: nbdb_sniffer.c,v 1.2.2.2 2026/05/11 17:13:52 martin Exp $	*/

/*++
/* NAME
/*	nbdb_sniffer 3
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_sniffer.h>
/*
/*	bool	nbdb_parse_as_postmap(VSTRING *line_buffer)
/*
/*	bool	nbdb_parse_as_postalias(VSTRING *line_buffer)
/*
/*	const char *nbdb_get_index_cmd_as(
/*	const char *source_path,
/*	uid_t	uid,
/*	gid_t	gid,
/*	VSTRING *why)
/* DESCRIPTION
/*	nbdb_parse_as_postmap() determines if a text line buffer contains
/*	valid input for the "postmap" command. nbdb_parse_as_postalias()
/*	does a similar thing for "postalias". Both functions modify
/*	their input.
/*
/*	nbdb_get_index_cmd_as() examines the first handful lines of an
/*	Berkeley DB source file, and returns "postmap", "postalias",
/*	or null in case of a file open error.
/* .PP
/*	Arguments:
/* .IP line_buffer
/*	The input sample. This will be parsed destructively.
/*  .IP source_path
/*	Berkeley DB source file pathname.
/* .IP uid, gid
/*	The privileges used for opening the Berkeley DB source file.
/* .IP why
/*	Storage that is updated with an applicable error description.
/* SEE ALSO
/*	nbdb_sniffer_test(1t), unit test
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <open_as.h>
#include <readlline.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <nbdb_util.h>
#include <tok822.h>

 /*
  * Application-specific.
  */
#include <nbdb_reindexd.h>
#include <nbdb_sniffer.h>

#define STR(x)	vstring_str(x)

/* nbdb_parse_as_postmap - sniff a source line and parse as key space value */

bool    nbdb_parse_as_postmap(VSTRING *line_buffer)
{
    int     in_quotes = 0;
    char   *key;
    char   *value;

    /*
     * Split on the first whitespace character, then trim leading and
     * trailing whitespace from key and value. Note that postalias does not
     * require a space after the ':'.
     */
    for (value = STR(line_buffer); *value; value++) {
	if (*value == '\\') {
	    if (*++value == 0)
		break;
	} else if (ISSPACE(*value)) {
	    if (!in_quotes)
		break;
	} else if (*value == '"') {
	    in_quotes = !in_quotes;
	}
    }

    /*
     * Keys should not contain an unbalanced double quote (input that is
     * skipped by postmap and dict_thash).
     */
    if (in_quotes)
	return (false);
    if (*value)
	*value++ = 0;
    while (ISSPACE(*value))
	value++;
    trimblanks(value, 0)[0] = 0;
    key = STR(line_buffer);

    /*
     * Input should be in "key whitespace value" form (other input is skipped
     * by postmap and dict_thash). Empty keys are not allowed because some
     * lookup tables did not support that. Additionally, the Postfix lookup
     * table API does not allow empty values. Instead, programs are supposed
     * to return a "not found" result.
     */
    if (*key == 0 || *value == 0)
	return (false);

    /*
     * Postmap and dict_thash accept keys ending in ':' but also log
     * warnings. Flag keys as 'postmap' format only if we have positive
     * evidence of well-formed input.
     */
    if (key[strlen(key) - 1] == ':')
	return (false);

    return (true);
}

/* nbdb_parse_as_postalias - sniff a source line and parse as key ':' value */

bool    nbdb_parse_as_postalias(VSTRING *line_buffer)
{
    TOK822 *tok_list;
    TOK822 *colon;
    bool    retval;

    /*
     * The postalias format requires a ':' between key and value. Do a quick
     * check to eliminate obvious cases without actually parsing the input.
     */
    if (strchr(STR(line_buffer), ':') == 0)
	return (false);

    /*
     * Tokenize the input, so that we do the right thing when a quoted
     * localpart contains special characters such as ":" and so on.
     */
    if ((tok_list = tok822_scan(STR(line_buffer), (TOK822 **) 0)) == 0)
	return (false);

    /*
     * Enforce the key:value format. Disallow missing keys, multi-address
     * keys, or missing values. In order to specify an empty string or value,
     * enclose it in double quotes. Note: postalias format does not require
     * space after ':'.
     */
    retval = !((colon = tok822_find_type(tok_list, ':')) == 0
	       || colon->prev == 0 || colon->next == 0
	       || tok822_rfind_type(colon, ','));
    tok822_free_tree(tok_list);
    return (retval);
}

/* get_index_cmd_as - sniff the table format: postmap, or postalias, or error */

const char *nbdb_get_index_cmd_as(const char *source_path, uid_t uid, gid_t gid,
				          VSTRING *why)
{
    VSTREAM *fp;

    /*
     * Some lookup tables may contain sensitive information (passwords, etc.)
     * and should not be world-readable. If a process with index owner
     * privilege cannot open this source file, then it also cannot index this
     * table with the postmap or postalias command.
     */
    if ((fp = vstream_fopen_as(source_path, O_RDONLY, 0, uid, gid)) == 0) {
	vstring_sprintf(why, "open lookup table source file %s as uid=%d "
			"gid=%d: %m", source_path, (int) uid, (int) gid);
	return (0);
    } else {
	VSTRING *line_buffer = vstring_alloc(100);
	VSTRING *copy = vstring_alloc(100);
	int     postmap_score = 0;
	int     postalias_score = 0;

	/*
	 * TODO(wietse) Eliminating 'format sniffing' code would require
	 * matching the source_path against the values of $alias_maps and
	 * $alias_database for every master.cf service. Such information is
	 * available from postconf(1).
	 */
#define NO_LINENO	((int *) 0)
#define NO_LAST_LINE	((int *) 0)

	while (readllines(line_buffer, fp, NO_LINENO, NO_LAST_LINE)) {
	    vstring_strcpy(copy, STR(line_buffer));
	    if (nbdb_parse_as_postmap(copy))
		postmap_score += 1;
	    if (nbdb_parse_as_postalias(line_buffer))
		postalias_score += 1;
	    if (postmap_score + postalias_score == 10)
		break;
	}
	vstream_fclose(fp);
	vstring_free(line_buffer);
	vstring_free(copy);
	if (postalias_score > postmap_score)
	    return "postalias";
	else
	    return ("postmap");
    }
}
