/*
 * cvt_pt.c
 *
 *	Covert partition type.	$Revision: 1.4 $
 *
 *	Copyright (c)  1999, Eryk Vershen
 * 
 * History:
 * Log: cvt_pt.c,v
 * Revision 1.2  2000/05/16 13:56:11  eryk
 * Minor fixes
 *
 * Revision 1.1  2000/02/13 22:04:08  eryk
 * Initial revision
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#include "partition_map.h"
#include "errors.h"


/*
 * Defines / Constants
 */
#define CFLAG_DEFAULT	0
#define DFLAG_DEFAULT	0
#define HFLAG_DEFAULT	0
#define INTERACT_DEFAULT	0
#define RFLAG_DEFAULT	0


/*
 * Structs / Types
 */


/*
 * Global variables
 */
int hflag = HFLAG_DEFAULT;	/* show help */
int dflag = DFLAG_DEFAULT;	/* turn on debugging commands and printout */
int rflag = RFLAG_DEFAULT;	/* open device read Only */
int interactive = INTERACT_DEFAULT;
int cflag = CFLAG_DEFAULT;	/* compute device size */


/*
 * Forward declarations
 */
void process(char *filename);


/*
 * Start here ...
 */
int
main(int argc, char **argv)
{
    register int i;
#ifdef notdef
    register int c;
    extern char *optarg;	/* pointer to argument */
    extern int optind;		/* next argv index */
    extern int opterr;		/* who does error messages */
    extern int optopt;		/* char that caused the error */
    int getopt_error;		/* getopt choked */
    char option_error[100];	/* buffer for error message */
    char *arg_option = 0;
    int bool_option = 0;
#else
    int optind = 1;
#endif

    init_program_name(argv);

#ifdef notdef
    opterr = 0;	/* tell getopt to be quiet */

    /*
     * Changes to getopt's last argument should
     * be reflected in the string printed by the
     * usage() function.
     */
    while ((c = getopt(argc, argv, "a:b")) != EOF) {
	if (c == '?') {
	    getopt_error = 1;
	    c = optopt;
	} else {
	    getopt_error = 0;
	}

	switch (c) {
	case 'a':
	    if (getopt_error) {
		usage("missing argument");
	    } else {
		arg_option = optarg;
	    }
	    break;
	case 'b':
	    bool_option = 1;
	    break;
	default:
	    snprintf(option_error, sizeof(option_error),
	        "no such option as -%c", c);
	    usage(option_error);
	}
    }
#endif

    if (optind >= argc) {
	usage("no file argument");
    }
    for (i = optind ; i < argc; i++) {
	process(argv[i]);
    }
    return 0;
}


int
trim_num(char *s)
{
    char *t;
    int n;

    for (t = s; *t; t++) {
    }

    for (t--; t >= s; t--) {
        if (!isdigit(*t)) {
            t++;
            if (*t) {
                n = atoi(t);
                *t = 0;
            } else {
                n = -1;
            }
            return n;
        }
    }

    return -1;
}

/*
 * The operation to apply to each file ...
 */
void
process(char *filename)
{
    char *s;
    int index;
    partition_map_header *map;
    int valid_file;
    partition_map * entry;

    //printf("Processing %s\n", filename);

    // 1)       strip off number from end of filename
    s = strdup(filename);
    index = trim_num(s);

    if (index < 0) {
        fatal(-1, "%s does not end in a number", filename);
    }

    // 2)       open prefix of filename as partition map
    map = open_partition_map(s, &valid_file, 0);
    if (!valid_file) {
        fatal(-1, "%s does not have a partition map", s);
        return;
    }

    // 3)       verify the type for the partition;

    if (map->writable == 0) {
	fatal(-1, "The map is not writable");
        return;
    }

    // 4) find partition and change it
    entry = find_entry_by_disk_address(index, map);
    if (entry == NULL) {
	fatal(-1, "No such partition");
    } else if (strcmp(entry->data->dpme_type, kHFSType) != 0) {
	fatal(-1, "Can't convert a partition with type %s",
		entry->data->dpme_type);
    } else {
	// 4a)       modify the type
	strncpy(entry->data->dpme_type, kUnixType, DPISTRLEN);
	
	// 5)       and write back.
	write_partition_map(map);
    }
}
