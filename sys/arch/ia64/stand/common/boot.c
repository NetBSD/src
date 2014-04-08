/*	$NetBSD: boot.c,v 1.7 2014/04/08 21:51:06 martin Exp $	*/

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
/* __FBSDID("$FreeBSD: src/sys/boot/common/boot.c,v 1.29 2003/08/25 23:30:41 obrien Exp $"); */


/*
 * Loading modules, booting the system
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include "bootstrap.h"

static char	*getbootfile(int try);
static int	loadakernel(int try, int argc, char* argv[]);

/* List of kernel names to try */
static const char *default_bootfiles = "netbsd";

static int autoboot_tried;

/*
 * The user wants us to boot.
 */

int
command_boot(int argc, char *argv[])
{
    struct preloaded_file	*fp;
    
    /*
     * See if the user has specified an explicit kernel to boot.
     */
    if ((argc > 1) && (argv[1][0] != '-')) {
	
	/* XXX maybe we should discard everything and start again? */
	if (file_findfile(NULL, NULL) != NULL) {
	    command_seterr("can't boot '%s', kernel module already loaded",
		argv[1]);
	    return(CMD_ERROR);
	}

	/* find/load the kernel module */
	if (file_loadkernel(argv[1], argc - 2, argv + 2) != 0)
	    return(CMD_ERROR);

	/* we have consumed all arguments */
	argc = 1;
    }

    /*
     * See if there is a kernel module already loaded
     */
    if (file_findfile(NULL, NULL) == NULL)
	if (loadakernel(0, argc - 1, argv + 1))
	    /* we have consumed all arguments */
	    argc = 1;

    /*
     * Loaded anything yet?
     */
    if ((fp = file_findfile(NULL, NULL)) == NULL) {
	command_seterr("no bootable kernel");
	return(CMD_ERROR);
    }

    /*
     * If we were given arguments, discard any previous.
     * XXX should we merge arguments?  Hard to DWIM.
     */
    if (argc > 1) {
	if (fp->f_args != NULL)	
	    free(fp->f_args);
	fp->f_args = unargv(argc - 1, argv + 1);
    }

    /* Hook for platform-specific autoloading of modules */
    if (archsw.arch_autoload() != 0)
	return(CMD_ERROR);

    /* Call the exec handler from the loader matching the kernel */
    command_seterr("%s", strerror(file_formats[fp->f_loader]->l_exec(fp)));
    return(CMD_ERROR);
}


/*
 * Autoboot after a delay
 */

int
command_autoboot(int argc, char *argv[])
{
    int		howlong;
    char	*cp, *prompt;

    prompt = NULL;
    howlong = -1;
    switch(argc) {
    case 3:
	prompt = argv[2];
	/* FALLTHROUGH */
    case 2:
	howlong = strtol(argv[1], &cp, 0);
	if (*cp != 0) {
	    command_seterr("bad delay '%s'", argv[1]);
	    return(CMD_ERROR);
	}
	/* FALLTHROUGH */
    case 1:
	return(autoboot(howlong, prompt));
    }
	
    command_seterr("too many arguments");
    return(CMD_ERROR);
}

/*
 * Called before we go interactive.  If we think we can autoboot, and
 * we haven't tried already, try now.
 */
void
autoboot_maybe(void)
{
    char	*cp;
    
    cp = getenv("autoboot_delay");
    if ((autoboot_tried == 0) && ((cp == NULL) || strcasecmp(cp, "NO")))
	autoboot(-1, NULL);		/* try to boot automatically */
}

int
autoboot(int timeout, char *prompt)
{
    time_t	when, otime, ntime;
    int		c, yes;
    char	*argv[2], *cp, *ep;
    char	*kernelname;

    autoboot_tried = 1;

    if (timeout == -1) {
	/* try to get a delay from the environment */
	if ((cp = getenv("autoboot_delay"))) {
	    timeout = strtol(cp, &ep, 0);
	    if (cp == ep)
		timeout = -1;
	}
    }
    if (timeout == -1)		/* all else fails */
	timeout = 10;

    kernelname = getenv("kernelname");
    if (kernelname == NULL) {
	argv[0] = NULL;
	loadakernel(0, 0, argv);
	kernelname = getenv("kernelname");
	if (kernelname == NULL) {
	    command_seterr("no valid kernel found");
	    return(CMD_ERROR);
	}
    }

    otime = time(NULL);
    when = otime + timeout;	/* when to boot */
    yes = 0;

    printf("%s\n", (prompt == NULL) ? "Hit [Enter] to boot immediately, or any other key for command prompt." : prompt);

    for (;;) {
	if (ischar()) {
	    c = getchar();
	    if ((c == '\r') || (c == '\n'))
		yes = 1;
	    break;
	}
	ntime = time(NULL);
	if (ntime >= when) {
	    yes = 1;
	    break;
	}
	
	if (ntime != otime) {
	    printf("\rBooting [%s] in %d second%s... ",
	    		kernelname, (int)(when - ntime),
			(when-ntime)==1?"":"s");
	    otime = ntime;
	}
    }
    if (yes)
	printf("\rBooting [%s]...               ", kernelname);
    putchar('\n');
    if (yes) {
	argv[0] = "boot";
	argv[1] = NULL;
	return(command_boot(1, argv));
    }
    return(CMD_OK);
}

/*
 * Scrounge for the name of the (try)'th file we will try to boot.
 */
static char *
getbootfile(int try) 
{
    static char *name = NULL;
    const char	*spec, *ep;
    size_t	len;
    
    /* we use dynamic storage */
    if (name != NULL) {
	free(name);
	name = NULL;
    }
    
    /* 
     * Try $bootfile, then try our builtin default
     */
    if ((spec = getenv("bootfile")) == NULL)
	spec = default_bootfiles;

    while ((try > 0) && (spec != NULL)) {
	spec = strchr(spec, ';');
	if (spec)
	    spec++;	/* skip over the leading ';' */
	try--;
    }
    if (spec != NULL) {
	if ((ep = strchr(spec, ';')) != NULL) {
	    len = ep - spec;
	} else {
	    len = strlen(spec);
	}
	name = alloc(len + 1);
	strncpy(name, spec, len);
	name[len] = 0;
    }
    if (name && name[0] == 0) {
	free(name);
	name = NULL;
    }
    else {
      setenv("kernelname", name, 1);
    }

    return(name);
}

/*
 * Try to find the /etc/fstab file on the filesystem (rootdev),
 * which should be be the root filesystem, and parse it to find 
 * out what the kernel ought to think the root filesystem is.
 *
 * If we're successful, set vfs.root.mountfrom to <vfstype>:<path>
 * so that the kernel can tell both which VFS and which node to use
 * to mount the device.  If this variable's already set, don't
 * overwrite it.
 */
int
getrootmount(char *rootdev)
{
    char	lbuf[128], *cp, *ep, *dev, *fstyp;
    int		fd, error;

    if (getenv("vfs.root.mountfrom") != NULL)
	return(0);

    snprintf(lbuf, sizeof(lbuf), "%s/etc/fstab", rootdev);
    if ((fd = open(lbuf, O_RDONLY)) < 0)
	return(1);

    /* loop reading lines from /etc/fstab    What was that about sscanf again? */
    error = 1;
    while (fgetstr(lbuf, sizeof(lbuf), fd) >= 0) {
	if ((lbuf[0] == 0) || (lbuf[0] == '#'))
	    continue;
	
	/* skip device name */
	for (cp = lbuf; (*cp != 0) && !isspace(*cp); cp++)
	    ;
	if (*cp == 0)		/* misformatted */
	    continue;
	/* delimit and save */
	*cp++ = 0;
	dev = strdup(lbuf);
    
	/* skip whitespace up to mountpoint */
	while ((*cp != 0) && isspace(*cp))
	    cp++;
	/* must have /<space> to be root */
	if ((*cp == 0) || (*cp != '/') || !isspace(*(cp + 1)))
	    continue;
	/* skip whitespace up to fstype */
	cp += 2;
	while ((*cp != 0) && isspace(*cp))
	    cp++;
	if (*cp == 0)		/* misformatted */
	    continue;
	/* skip text to end of fstype and delimit */
	ep = cp;
	while ((*cp != 0) && !isspace(*cp))
	    cp++;
	*cp = 0;
	fstyp = strdup(ep);

	/* build the final result and save it */
	snprintf(lbuf, sizeof(lbuf), "%s:%s", fstyp, dev);
	free(dev);
	free(fstyp);
	setenv("vfs.root.mountfrom", lbuf, 0);
	error = 0;
	break;
    }
    close(fd);
    return(error);
}

static int
loadakernel(int try, int argc, char* argv[])
{
	char *cp;
	
	for (try = 0; (cp = getbootfile(try)) != NULL; try++)
		if (file_loadkernel(cp, argc - 1, argv + 1) != 0)
			printf("can't load '%s'\n", cp);
		else
			return 1;
	return 0;
}

