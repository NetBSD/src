/*	$NetBSD: ftpio.c,v 1.7 2000/01/19 23:28:33 hubertf Exp $	*/
/*	 Id: foo2.c,v 1.12 1999/12/17 02:31:57 feyrer Exp feyrer 	*/

/*
 * Work to implement wildcard dependency processing via FTP for the
 * NetBSD pkg_* package tools. 
 *
 * Copyright (c) 1999 Hubert Feyrer <hubertf@netbsd.org>
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vis.h>

#include "../lib/lib.h"

/*
 * Names of environment variables used to pass things to
 * subprocesses, for connection caching.
 */
#define PKG_FTPIO_COMMAND		"PKG_FTPIO_COMMAND"
#define PKG_FTPIO_ANSWER		"PKG_FTPIO_ANSWER"
#define PKG_FTPIO_CNT			"PKG_FTPIO_CNT"
#define PKG_FTPIO_CURRENTHOST		"PKG_FTPIO_CURRENTHOST"
#define PKG_FTPIO_CURRENTDIR		"PKG_FTPIO_CURRENTDIR"

#undef STANDALONE   /* define for standalone debugging */

/* File descriptors */
typedef struct {
    int command;
    int answer;
} fds;


#if EXPECT_DEBUG
static int	 expect_debug = 1;
#endif /* EXPECT_DEBUG */
#ifdef STANDALONE
int		Verbose=1;
#endif
static int	 needclose=0;
static int	 ftp_started=0;
static fds	 ftpio;


/*
 * expect "str" (a regular expression) on file descriptor "fd", storing
 * the FTP return code of the command in the integer "ftprc". The "str"
 * string is expected to match some FTP return codes after a '\n', e.g.
 * "\n(550|226).*\n" 
 */
static int
expect(int fd, const char *str, int *ftprc)
{
    int rc;
    char buf[90];  
#if EXPECT_DEBUG
    char *vstr;
#endif /* EXPECT_DEBUG */
    regex_t rstr;
    int done;
    struct timeval timeout;
    int retval;
    regmatch_t pmatch;
    int verbose_expect=0;

#if EXPECT_DEBUG
    vstr=malloc(2*sizeof(buf));
    if (vstr == NULL)
	err(1, "expect: malloc() failed");
    strvis(vstr, str, VIS_NL|VIS_SAFE|VIS_CSTYLE);
#endif /* EXPECT_DEBUG */
	    
    if (regcomp(&rstr, str, REG_EXTENDED) != 0)
	err(1, "expect: regcomp() failed");

#if EXPECT_DEBUG
    if (expect_debug)
	printf("expecting \"%s\" on fd %d ...\n", vstr, fd);
#endif /* EXPECT_DEBUG */

    if(0) setbuf(stdout, NULL);

    memset(buf, '#', sizeof(buf));

    timeout.tv_sec=60;
    timeout.tv_usec=0;
    done=0;
    retval=0;
    while(!done) {
	fd_set fdset;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	rc = select(FD_SETSIZE, &fdset, NULL, NULL, &timeout);
	switch (rc) {
	case -1:
	    warn("expect: select() failed (probably ftp died because of bad args)");
	    done = 1;
	    retval = -1;
	    break;
	case 0:
	    warnx("expect: select() timeout!");
	    done = 1; /* hope that's ok */
	    retval = -1;
	    break;
	default:
	    rc=read(fd,&buf[sizeof(buf)-1],1);

	    if (verbose_expect)
		putchar(buf[sizeof(buf)-1]);

#if EXPECT_DEBUG
	    {
		    char *v=malloc(2*sizeof(buf));
		    strvis(v, buf, VIS_NL|VIS_SAFE|VIS_CSTYLE);
		    if (expect_debug)
			    printf("expect=<%s>, buf=<%*s>\n", vstr, strlen(v), v);
		    free(v);
	    }
#endif /* EXPECT_DEBUG */

	    if (regexec(&rstr, buf, 1, &pmatch, 0) == 0) {
#if EXPECT_DEBUG
		if (expect_debug)
		    printf("Gotcha -> %s!\n", buf+pmatch.rm_so+1);
		fflush(stdout);
#endif /* EXPECT_DEBUG */

		if (ftprc && isdigit(buf[pmatch.rm_so+1])) 
		    *ftprc = atoi(buf+pmatch.rm_so+1);

		done=1;
		retval=0;
	    }
	    
	    memmove(buf, buf+1, sizeof(buf)-1); /* yes, this is non-performant */
	    break;
	}
    }

#if EXPECT_DEBUG
    printf("done.\n");

    if (str)
	free(vstr);
#endif /* EXPECT_DEBUG */

    return retval;
}

/*
 * send a certain ftp-command "cmd" to our FTP coprocess, and wait for
 * "expectstr" to be returned. Return numeric FTP return code. 
 */
static int
ftp_cmd(const char *cmd, const char *expectstr)
{
    int rc=0, verbose_ftp=0;

    if (Verbose)
	    verbose_ftp=1;
    
    if (verbose_ftp)
      fprintf(stderr, "\n[1mftp> %s[0m", cmd);
    
    fflush(stdout);
    rc=write(ftpio.command, cmd, strlen(cmd));
    if (rc == strlen(cmd)) {
	    if (expectstr) {
		    expect(ftpio.answer, expectstr, &rc);
	    }
    } else {
	    if (Verbose)
		    warn("short write");
    }

    return rc;
}


/*
 * Really fire up FTP coprocess
 */
static int
setupCoproc(const char *base)
{
    int command_pipe[2];
    int answer_pipe[2];
    int rc1, rc2;
    char buf[20];

    rc1 = pipe(command_pipe);
    rc2 = pipe(answer_pipe);

    if(rc1==-1 || rc2==-1) {
	warn("setupCoproc: pipe() failed");
	return -1;
    }

    rc1=fork();
    switch (rc1) {
    case -1:
	    /* Error */
	    
	    warn("setupCoproc: fork() failed");
	    return -1;
	    break;

    case 0:
	    /* Child */
	    
	    close(command_pipe[1]);
	    dup2(command_pipe[0], 0);
	    close(command_pipe[0]);
	    
	    close(answer_pipe[0]);
	    dup2(answer_pipe[1], 1);
	    close(answer_pipe[1]);
	    
	    setbuf(stdout, NULL);
	    
	    if (Verbose)
		    fprintf(stderr, "[1mftp -detv %s[0m\n", base);
	    rc1 = execl("/usr/bin/ftp", "ftp", "-detv", base, NULL);
	    warn("setupCoproc: execl() failed");
	    exit(-1);
	    break;
    default: 
	    /* Parent */
	    close(command_pipe[0]);
	    close(answer_pipe[1]);
	    
	    snprintf(buf, sizeof(buf), "%d", command_pipe[1]);
	    setenv(PKG_FTPIO_COMMAND, buf, 1);
	    snprintf(buf, sizeof(buf), "%d", answer_pipe[0]);
	    setenv(PKG_FTPIO_ANSWER, buf, 1);
	    
	    ftpio.command = command_pipe[1];
	    ftpio.answer  = answer_pipe[0];
	    
	    fcntl(ftpio.command, F_SETFL, O_NONBLOCK);
	    fcntl(ftpio.answer , F_SETFL, O_NONBLOCK);
	    
	    break;
    }

    return 0;
}


/*
 * SIGPIPE only happens when there's something wrong with the FTP
 * coprocess. In that case, set mark to not try to close shut down
 * the coprocess.
 */
static void
sigpipe_handler(int n)
{
    /* aparently our ftp companion died */
    if (Verbose)
	    fprintf(stderr, "SIGPIPE!\n");
    needclose = 0;
}


/*
 * Close the FTP coprocess' current connection, but
 * keep the process itself alive. 
 */
void 
ftp_stop(void)
{
	char *tmp1, *tmp2;
	
	if (!ftp_started)
		return;
	
	tmp1=getenv(PKG_FTPIO_COMMAND);
	tmp2=getenv(PKG_FTPIO_ANSWER);
	
	/* (Only) the last one closes the link */
	if (tmp1 != NULL && tmp2 != NULL) {
		if (needclose)
			ftp_cmd("close\n", "\n(221 Goodbye.|Not connected.)\n");
		
		close(ftpio.command);
		close(ftpio.answer);
	}

	unsetenv(PKG_FTPIO_COMMAND); 
	unsetenv(PKG_FTPIO_ANSWER);
}


/*
 * (Start and re-)Connect the FTP coprocess to some host/dir.
 * If the requested host/dir is different than the one that the
 * coprocess is currently at, close first. 
 */
static int
ftp_start(char *base)
{
	char *tmp1, *tmp2;
	int rc;
	char newHost[256];
	char newDir[1024];
	char *currentHost=getenv(PKG_FTPIO_CURRENTHOST);
	char *currentDir=getenv(PKG_FTPIO_CURRENTDIR);
	
	fileURLHost(base, newHost, sizeof(newHost));
	strcpy(newDir, strchr(base+URLlength(base), '/') + 1);
	if (currentHost
	    && currentDir
	    && ( strcmp(newHost, currentHost) != 0
		 || strcmp(newDir, currentDir) != 0)) { /* could handle new dir case better here, w/o reconnect */
		if (Verbose) {
			printf("ftp_stgart: new host or dir, stopping previous connect...\n");
			printf("currentHost='%s', newHost='%s'\n", currentHost, newHost);
			printf("currentDir='%s', newDir='%s'\n", currentDir, newDir);
		}
		
		ftp_stop();
		
		if (Verbose)
			printf("ftp stopped\n");
	}
	setenv(PKG_FTPIO_CURRENTHOST, newHost, 1); /* need to update this in the environment */
	setenv(PKG_FTPIO_CURRENTDIR, newDir, 1);   /* for subprocesses to have this available */
	
	tmp1=getenv(PKG_FTPIO_COMMAND);
	tmp2=getenv(PKG_FTPIO_ANSWER);
	if(tmp1==NULL || tmp2==NULL || *tmp1=='\0' || *tmp2=='\0') {
		/* no FTP coprocess running yet */
		
		if (Verbose)
			printf("Spawning FTP coprocess\n");
		
		rc = setupCoproc(base);
		if (rc == -1) {
			warnx("setupCoproc() failed");
			return -1;
		}
		
		needclose=1;
		signal(SIGPIPE, sigpipe_handler);
		
		if ((expect(ftpio.answer, "\n(221|250|221|550).*\n", &rc) != 0)
		    || rc != 250) {
			warnx("expect1 failed, rc=%d", rc);
			return -1;
		}
		
		rc = ftp_cmd("prompt off\n", "\n(Interactive mode off|221).*\n");
		if (rc == 221) {
			/* something is wrong */
			ftp_started=1; /* not really, but for ftp_stop() */
			ftp_stop();
		}
		
		ftp_started=1;
	} else {
		/* get FDs of our coprocess */
		
		ftpio.command = dup(atoi(tmp1));
		ftpio.answer  = dup(atoi(tmp2));
		
		if (Verbose)
			printf("Reusing FDs %s/%s for communication to FTP coprocess\n", tmp1, tmp2);
		
		fcntl(ftpio.command, F_SETFL, O_NONBLOCK);
		fcntl(ftpio.answer , F_SETFL, O_NONBLOCK);
	}
	
	return 0;
}


/*
 * Expand the given wildcard URL "wildcardurl" if possible, and store the
 * expanded value into "expandedurl". return 0 if successful, -1 else.
 */
int
expandURL(char *expandedurl, const char *wildcardurl)
{
    char *pkg;
    int rc;
    char base[FILENAME_MAX];

    pkg=strrchr(wildcardurl, '/');
    if (pkg == NULL){
	warnx("expandURL: no '/' in url %s?!", wildcardurl);
	return -1;
    }
    snprintf(base, FILENAME_MAX, "%*.*s/", pkg-wildcardurl, pkg-wildcardurl, wildcardurl);
    pkg++;

    rc = ftp_start(base);
    if (rc == -1) {
	    warnx("ftp_start() failed");
	    return -1; /* error */
    }

    /* for a given wildcard URL, find the best matching pkg */
    {
	char *s, buf[FILENAME_MAX];
	char tmpname[FILENAME_MAX];
	char best[FILENAME_MAX];

	strcpy(tmpname, "/tmp/pkg.XXX");
	mktemp(tmpname);
	assert(tmpname != NULL);

	s=strpbrk(pkg, "<>[]?*{"); /* Could leave out "[]?*" here;
				    * ftp(1) is not that stupid */
	if (!s) {
		/* This should only happen when getting here with (only) a package
		 * name specified to pkg_add, and PKG_PATH containing some URL.
		 */
		snprintf(buf,FILENAME_MAX, "ls %s %s\n", pkg, tmpname);
	} else {
		/* replace possible version(wildcard) given with "-*".
		 * we can't use the pkg wildcards here as dewey compare
		 * and alternates won't be handled by ftp(1); sort
		 * out later, using pmatch() */
		snprintf(buf, FILENAME_MAX, "ls %*.*s*.tgz %s\n", s-pkg, s-pkg, pkg, tmpname);
	}
	
	rc = ftp_cmd(buf, "\n(550|226).*\n"); /* catch errors */
	if (rc != 226) {
	    if (Verbose)
		    warnx("ls failed!");
	    return -1;
	}

	/* Sync - don't remove */
	rc = ftp_cmd("cd .\n", "\n(550|250).*\n");
	if (rc != 250) {
	    warnx("chdir failed!");
	    return -1;
	}
	
	best[0]='\0';
	if (access(tmpname, R_OK)==0) {
		int matches;
		FILE *f;
		char filename[FILENAME_MAX];
		
		f=fopen(tmpname, "r");
		if (f == NULL) {
		    warn("fopen");
		    return -1;
		}
		matches=0;
		/* The following loop is basically the same as the readdir() loop
		 * in findmatchingname() */
		while (fgets(filename, FILENAME_MAX, f)) {
			filename[strlen(filename)-1] = '\0';
			if (pmatch(pkg, filename)) {
				matches++;

				/* compare findbestmatchingname() */
				findbestmatchingname_fn(filename, best);
			}
		}
		fclose(f);
		
		if (matches == 0)
			warnx("nothing appropriate found\n");
	}

	unlink(tmpname);

	if (best[0] != '\0') {
		if (Verbose)
			printf("best match: '%s%s'\n", base, best);
		sprintf(expandedurl, "%s%s", base, best);
	}
    }

    return 0;
}


/*
 * extract the given (expanded) URL "url" to the given directory "dir"
 * return -1 on error, 0 else;
 */
int
unpackURL(const char *url, const char *dir)
{
	char *pkg;
	int rc;
	char base[FILENAME_MAX];
	char pkg_path[FILENAME_MAX];

	{
		/* Verify if the url is really ok */
		int rc;
		char exp[FILENAME_MAX];

		rc=expandURL(exp, url);
		if (rc == -1) {
			warnx("unpackURL: verification expandURL failed");
			return -1;
		}
		if (strcmp(exp, url) != 0) {
			warnx("unpackURL: verificatinon expandURL failed, '%s'!='%s'",
			      exp, url);
			return -1;
		}
	}
	
	pkg=strrchr(url, '/');
	if (pkg == NULL){
		warnx("unpackURL: no '/' in url %s?!", url);
		return -1;
	}
	snprintf(base, FILENAME_MAX, "%*.*s/", pkg-url, pkg-url, url);
	snprintf(pkg_path, FILENAME_MAX, "%*.*s", pkg-url, pkg-url, url); /* no trailing '/' */
	pkg++;

	/* Leave a hint for any depending pkgs that may need it */
	if (getenv("PKG_PATH") == NULL) {
		setenv("PKG_PATH", pkg_path, 1);
		printf("setenv PKG_PATH='%s'\n",pkg_path);
	}
	
	rc = ftp_start(base);
	if (rc == -1) {
		warnx("ftp_start() failed");
		return -1; /* error */
	}

	{
		char cmd[1024];

		if (Verbose)
			printf("unpackURL '%s' to '%s'\n", url, dir);

		/* yes, this is gross, but needed for borken ftp(1) */
		snprintf(cmd, sizeof(cmd), "get %s \"| ( cd %s ; gunzip 2>/dev/null | tar -%sx -f - )\"\n", pkg, dir, Verbose?"vv":"");
		rc = ftp_cmd(cmd, "\n(226|550).*\n");
		if (rc != 226) {
			warnx("Cannot fetch file (%d!=226)!", rc);
			return -1;
		}
	}

	return 0;
}


#if 0
/*
 * Some misc stuff not needed yet, but maybe later
 */
int
miscstuff(const char *url)
{
    char *pkg;
    int rc;
    char base[FILENAME_MAX];

    pkg=strrchr(url, '/');
    if (pkg == NULL){
	warnx("miscstuff: no '/' in url %s?!", url);
	return -1;
    }
    snprintf(base, FILENAME_MAX, "%*.*s/", pkg-url, pkg-url, url);
    pkg++;

    rc = ftp_start(base);
    if (rc == -1) {
	    warnx("ftp_start() failed");
	    return -1; /* error */
    }

    /* basic operation */
    if (0) {
	    rc = ftp_cmd("cd ../All\n", "\n(550|250).*\n");
	    if (rc != 250) {
		    warnx("chdir failed!");
		    return -1;
	    }
    }

    /* get and extract a file to tmpdir */
    if (0) {
	char cmd[256];
	char tmpdir[256];

	snprintf(tmpdir, sizeof(tmpdir), "/tmp/dir%s",
		 (getenv(PKG_FTPIO_CNT))?getenv(PKG_FTPIO_CNT):"");

	mkdir(tmpdir, 0755);

	/* yes, this is gross, but needed for borken ftp(1) */
	snprintf(cmd, sizeof(cmd), "get xpmroot-1.01.tgz \"| ( cd %s ; gunzip 2>/dev/null | tar -vvx -f - )\"\n", tmpdir);
	rc = ftp_cmd(cmd, "\n(226|550).*\n");
	if (rc != 226) {
	    warnx("Cannot fetch file (%d != 226)!", rc);
	    return -1;
	}
    }
    
    /* check if one more file(s) exist */
    if (0) {
	char buf[FILENAME_MAX];
	snprintf(buf,FILENAME_MAX, "ls %s /tmp/xxx\n", pkg);
	rc = ftp_cmd(buf, "\n(226|550).*\n"); /* catch errors */
	if (rc != 226) {
	    if (Verbose)
		warnx("ls failed!");
	    return -1;
	}

	/* Sync - don't remove */
	rc = ftp_cmd("cd .\n", "\n(550|250).*\n");
	if (rc != 250) {
	    warnx("chdir failed!");
	    return -1;
	}
	
	if (access("/tmp/xxx", R_OK)==0) {
	    system("cat /tmp/xxx");
	    
	    {
		/* count lines - >0 -> fexists() == true */
		int len, count;
		FILE *f;
		
		f=fopen("/tmp/xxx", "r");
		if (f == NULL) {
		    warn("fopen");
		    return -1;
		}
		count=0;
		while (fgetln(f, &len))
		    count++;
		fclose(f);
		
		printf("#lines = %d\n", count);
	    }
	} else
	    printf("NO MATCH\n");
	
	unlink("/tmp/xxx");
    }

    /* for a given wildcard URL, find the best matching pkg */
    /* spawn child - like pkg_add'ing another package */
    /* Here's where the connection caching kicks in */
#if 0
    if (0) {
	char *s, buf[FILENAME_MAX];

	if ((s=getenv(PKG_FTPIO_CNT)) && atoi(s)>0){
	    snprintf(buf,FILENAME_MAX,"%d", atoi(s)-1);
	    setenv(PKG_FTPIO_CNT, buf, 1);

	    snprintf(buf, FILENAME_MAX, "%s \"%s/%s\"", argv0, url, pkg);
	    printf("%s>>> %s\n", s, buf);
	    system(buf);
	}
    }
#endif 

    return 0;

}
#endif


#ifdef STANDALONE
static void
usage(void)
{
  errx(1, "Usage: foo [-v] ftp://-pattern\n");
}


int
main(int argc, char *argv[])
{
    int rc, ch;
    char *argv0 = argv[0];

    while ((ch = getopt(argc, argv, "v")) != -1) {
      switch (ch) {
      case 'v':
	Verbose=1;
	break;
      default:
	usage();
      }
    }
    argc -= optind;
    argv += optind;

    if (argc<1)
	usage();

    while(argv[0] != NULL) {
	    char newurl[FILENAME_MAX];
	    
	    printf("Expand %s:\n", argv[0]);
	    rc = expandURL(newurl, argv[0]);
	    if (rc==-1)
		    warnx("Cannot expand %s", argv[0]);
	    else
		    printf("Expanded URL: %s\n", newurl);

	    /* test out connection caching */
	    if (1) {
		    char *s, buf[FILENAME_MAX];
		    
		    if ((s=getenv(PKG_FTPIO_CNT)) && atoi(s)>0){
			    snprintf(buf,FILENAME_MAX,"%d", atoi(s)-1);
			    setenv(PKG_FTPIO_CNT, buf, 1);
			    
			    snprintf(buf, FILENAME_MAX, "%s -v '%s'", argv0, argv[0]);
			    printf("%s>>> %s\n", s, buf);
			    system(buf);
		    }
	    }
	    
	    printf("\n\n\n");
	    argv++;
    }

    ftp_stop();

    return 0;
}
#endif /* STANDALONE */
