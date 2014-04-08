/*	$NetBSD: commands.c,v 1.5 2014/04/08 21:51:06 martin Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/common/commands.c,v 1.19 2003/08/25 23:30:41 obrien Exp $"); */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include "bootstrap.h"

static char command_errbuf[256];

int
command_seterr(const char *fmt, ...)
{
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(command_errbuf, sizeof(command_errbuf), fmt, ap);
	va_end(ap);
	return len;
}

const char *
command_geterr(void)
{
	return command_errbuf;
}

static int page_file(char *filename);

/*
 * Help is read from a formatted text file.
 *
 * Entries in the file are formatted as 

# Ttopic [Ssubtopic] Ddescription
help
text
here
#

 *
 * Note that for code simplicity's sake, the above format must be followed
 * exactly.
 *
 * Subtopic entries must immediately follow the topic (this is used to
 * produce the listing of subtopics).
 *
 * If no argument(s) are supplied by the user, the help for 'help' is displayed.
 */

static int
help_getnext(int fd, char **topic, char **subtopic, char **desc) 
{
    char	line[81], *cp, *ep;
    
    for (;;) {
	if (fgetstr(line, 80, fd) < 0)
	    return(0);
	
	if ((strlen(line) < 3) || (line[0] != '#') || (line[1] != ' '))
	    continue;

	*topic = *subtopic = *desc = NULL;
	cp = line + 2;
	while((cp != NULL) && (*cp != 0)) {
	    ep = strchr(cp, ' ');
	    if ((*cp == 'T') && (*topic == NULL)) {
		if (ep != NULL)
		    *ep++ = 0;
		*topic = strdup(cp + 1);
	    } else if ((*cp == 'S') && (*subtopic == NULL)) {
		if (ep != NULL)
		    *ep++ = 0;
		*subtopic = strdup(cp + 1);
	    } else if (*cp == 'D') {
		*desc = strdup(cp + 1);
		ep = NULL;
	    }
	    cp = ep;
	}
	if (*topic == NULL) {
	    if (*subtopic != NULL)
		free(*subtopic);
	    if (*desc != NULL)
		free(*desc);
	    continue;
	}
	return(1);
    }
}

static int
help_emitsummary(char *topic, char *subtopic, char *desc)
{
    int		i;
    
    pager_output("    ");
    pager_output(topic);
    i = strlen(topic);
    if (subtopic != NULL) {
	pager_output(" ");
	pager_output(subtopic);
	i += strlen(subtopic) + 1;
    }
    if (desc != NULL) {
	do {
	    pager_output(" ");
	} while (i++ < 30);
	pager_output(desc);
    }
    return (pager_output("\n"));
}

	    
int
command_help(int argc, char *argv[]) 
{
    char	buf[81];	/* XXX buffer size? */
    int		hfd, matched, doindex;
    char	*topic, *subtopic, *t, *s, *d;

    /* page the help text from our load path */
    snprintf(buf, sizeof(buf), "%s/boot/loader.help", getenv("loaddev"));
    if ((hfd = open(buf, O_RDONLY)) < 0) {
	printf("Verbose help not available, use '?' to list commands\n");
	return(CMD_OK);
    }

    /* pick up request from arguments */
    topic = subtopic = NULL;
    switch(argc) {
    case 3:
	subtopic = strdup(argv[2]);
    case 2:
	topic = strdup(argv[1]);
	break;
    case 1:
	topic = strdup("help");
	break;
    default:
	command_seterr("usage is 'help <topic> [<subtopic>]");
	return(CMD_ERROR);
    }

    /* magic "index" keyword */
    doindex = !strcmp(topic, "index");
    matched = doindex;
    
    /* Scan the helpfile looking for help matching the request */
    pager_open();
    while(help_getnext(hfd, &t, &s, &d)) {

	if (doindex) {		/* dink around formatting */
	    if (help_emitsummary(t, s, d))
		break;

	} else if (strcmp(topic, t)) {
	    /* topic mismatch */
	    if(matched)		/* nothing more on this topic, stop scanning */
		break;

	} else {
	    /* topic matched */
	    matched = 1;
	    if (((subtopic == NULL) && (s == NULL)) ||
		((subtopic != NULL) && (s != NULL) && !strcmp(subtopic, s))) {
		/* exact match, print text */
		while((fgetstr(buf, 80, hfd) >= 0) && (buf[0] != '#')) {
		    if (pager_output(buf))
			break;
		    if (pager_output("\n"))
			break;
		}
	    } else if ((subtopic == NULL) && (s != NULL)) {
		/* topic match, list subtopics */
		if (help_emitsummary(t, s, d))
		    break;
	    }
	}
	free(t);
	free(s);
	free(d);
    }
    pager_close();
    close(hfd);
    if (!matched) {
	command_seterr("no help available for '%s'", topic);
	free(topic);
	if (subtopic)
	    free(subtopic);
	return(CMD_ERROR);
    }
    free(topic);
    if (subtopic)
	free(subtopic);
    return(CMD_OK);
}


int
command_commandlist(int argc, char *argv[])
{
    struct bootblk_command	*cmdp;
    int		res;
    char	name[20];
    int i;

    res = 0;
    pager_open();
    res = pager_output("Available commands:\n");

    for (i = 0, cmdp = commands; (cmdp->c_name != NULL) && (cmdp->c_desc != NULL ); i++, cmdp = commands + i) {
	    if (res)
	    break;
	if ((cmdp->c_name != NULL) && (cmdp->c_desc != NULL)) {
	    snprintf(name, sizeof(name), "  %s  ", cmdp->c_name);
	    pager_output(name);
	    pager_output(cmdp->c_desc);
	    res = pager_output("\n");
	}
    }
    pager_close();
    return(CMD_OK);
}

/*
 * XXX set/show should become set/echo if we have variable
 * substitution happening.
 */

int
command_show(int argc, char *argv[])
{
    struct env_var	*ev;
    char		*cp;

    if (argc < 2) {
	/* 
	 * With no arguments, print everything.
	 */
	pager_open();
	for (ev = environ; ev != NULL; ev = ev->ev_next) {
	    pager_output(ev->ev_name);
	    cp = getenv(ev->ev_name);
	    if (cp != NULL) {
		pager_output("=");
		pager_output(cp);
	    }
	    if (pager_output("\n"))
		break;
	}
	pager_close();
    } else {
	if ((cp = getenv(argv[1])) != NULL) {
	    printf("%s\n", cp);
	} else {
	    command_seterr("variable '%s' not found", argv[1]);
	    return(CMD_ERROR);
	}
    }
    return(CMD_OK);
}


int
command_set(int argc, char *argv[])
{
    int		err;
    
    if (argc != 2) {
	command_seterr("wrong number of arguments");
	return(CMD_ERROR);
    } else {
	if ((err = putenv(argv[1])) != 0) {
	    command_seterr("%s", strerror(err));
	    return(CMD_ERROR);
	}
    }
    return(CMD_OK);
}


int
command_unset(int argc, char *argv[]) 
{
    int		err;
    
    if (argc != 2) {
	command_seterr("wrong number of arguments");
	return(CMD_ERROR);
    } else {
	if ((err = unsetenv(argv[1])) != 0) {
	    command_seterr("%s", strerror(err));
	    return(CMD_ERROR);
	}
    }
    return(CMD_OK);
}


int
command_echo(int argc, char *argv[])
{
    char	*s;
    int		nl, ch;
    
    nl = 0;
    optind = 1;
    optreset = 1;
    while ((ch = getopt(argc, argv, "n")) != -1) {
	switch(ch) {
	case 'n':
	    nl = 1;
	    break;
	case '?':
	default:
	    /* getopt has already reported an error */
	    return(CMD_OK);
	}
    }
    argv += (optind);
    argc -= (optind);

    s = unargv(argc, argv);
    if (s != NULL) {
	printf("%s", s);
	free(s);
    }
    if (!nl)
	printf("\n");
    return(CMD_OK);
}

/*
 * A passable emulation of the sh(1) command of the same name.
 */


int
command_read(int argc, char *argv[])
{
    char	*prompt;
    int		timeout;
    time_t	when;
    char	*cp;
    char	*name;
    char	buf[256];		/* XXX size? */
    int		c;
    
    timeout = -1;
    prompt = NULL;
    optind = 1;
    optreset = 1;
    while ((c = getopt(argc, argv, "p:t:")) != -1) {
	switch(c) {
	    
	case 'p':
	    prompt = optarg;
	    break;
	case 't':
	    timeout = strtol(optarg, &cp, 0);
	    if (cp == optarg) {
		command_seterr("bad timeout '%s'", optarg);
		return(CMD_ERROR);
	    }
	    break;
	default:
	    return(CMD_OK);
	}
    }

    argv += (optind);
    argc -= (optind);
    name = (argc > 0) ? argv[0]: NULL;
	
    if (prompt != NULL)
	printf("%s", prompt);
    if (timeout >= 0) {
	when = time(NULL) + timeout;
	while (!ischar())
	    if (time(NULL) >= when)
		return(CMD_OK);		/* is timeout an error? */
    }

    ngets(buf, sizeof(buf));

    if (name != NULL)
	setenv(name, buf, 1);
    return(CMD_OK);
}

/*
 * File pager
 */

int
command_more(int argc, char *argv[])
{
    int         i;
    int         res;
    char	line[80];

    res=0;
    pager_open();
    for (i = 1; (i < argc) && (res == 0); i++) {
	snprintf(line, sizeof(line), "*** FILE %s BEGIN ***\n", argv[i]);
	if (pager_output(line))
		break;
        res = page_file(argv[i]);
	if (!res) {
	    snprintf(line, sizeof(line), "*** FILE %s END ***\n", argv[i]);
	    res = pager_output(line);
	}
    }
    pager_close();

    if (res == 0)
	return CMD_OK;
    else
	return CMD_ERROR;
}

static int
page_file(char *filename)
{
    int result;

    result = pager_file(filename);

    if (result == -1)
	command_seterr("error showing %s", filename);

    return result;
}   

/*
 * List all disk-like devices
 */

int
command_lsdev(int argc, char *argv[])
{
    int		verbose, ch, i;
    char	line[80];
    
    verbose = 0;
    optind = 1;
    optreset = 1;
    while ((ch = getopt(argc, argv, "v")) != -1) {
	switch(ch) {
	case 'v':
	    verbose = 1;
	    break;
	case '?':
	default:
	    /* getopt has already reported an error */
	    return(CMD_OK);
	}
    }
    argv += (optind);
    argc -= (optind);

    pager_open();

    snprintf(line, sizeof(line), "Device Enumeration:\n");
    pager_output(line);

    for (i = 0; i < ndevs; i++) {
	 snprintf(line, sizeof(line), "%s\n", devsw[i].dv_name);
	    if (pager_output(line))
		    break;
    }

    pager_close();
    return(CMD_OK);
}

