/*	$NetBSD: pcnfsd_print.c,v 1.2 1995/07/25 22:20:48 gwr Exp $	*/

/* RE_SID: @(%)/usr/dosnfs/shades_SCCS/unix/pcnfsd/v2/src/SCCS/s.pcnfsd_print.c 1.7 92/01/24 19:58:58 SMI */
/*
**=====================================================================
** Copyright (c) 1986,1987,1988,1989,1990,1991 by Sun Microsystems, Inc.
**	@(#)pcnfsd_print.c	1.7	1/24/92
**=====================================================================
*/
#include "common.h"
/*
**=====================================================================
**             I N C L U D E   F I L E   S E C T I O N                *
**                                                                    *
** If your port requires different include files, add a suitable      *
** #define in the customization section, and make the inclusion or    *
** exclusion of the files conditional on this.                        *
**=====================================================================
*/
#include "pcnfsd.h"
#include <malloc.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#ifndef SYSV
#include <sys/wait.h>
#endif

#ifdef ISC_2_0
#include <sys/fcntl.h>
#endif

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif

/*
**---------------------------------------------------------------------
** Other #define's 
**---------------------------------------------------------------------
*/
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef SPOOLDIR
#define SPOOLDIR        "/export/pcnfs"
#endif

#ifndef LPRDIR
#define LPRDIR		"/usr/bin"
#endif

#ifndef LPCDIR
#define LPCDIR		"/usr/sbin"
#endif

/*
** The following defintions give the maximum time allowed for
** an external command to run (in seconds)
*/
#define MAXTIME_FOR_PRINT	10
#define MAXTIME_FOR_QUEUE	10
#define MAXTIME_FOR_CANCEL	10
#define MAXTIME_FOR_STATUS	10

#define QMAX 50

/*
** The following is derived from ucb/lpd/displayq.c
*/
#define SIZECOL 62
#define FILECOL 24

extern void     scramble();
extern void     run_ps630();
extern char    *crypt();
extern FILE    *su_popen();
extern int      su_pclose();
int             build_pr_list();
char 	       *map_printer_name();
char	       *expand_alias();
void           *grab();
void            free_pr_list_item();
void            free_pr_queue_item();
pr_list		list_virtual_printers();

/*
**---------------------------------------------------------------------
**                       Misc. variable definitions
**---------------------------------------------------------------------
*/

extern int      errno;
extern int	interrupted;	/* in pcnfsd_misc.c */
struct stat     statbuf;
char            pathname[MAXPATHLEN];
char            new_pathname[MAXPATHLEN];
char            sp_name[MAXPATHLEN] = SPOOLDIR;
char            tempstr[256];
char            delims[] = " \t\r\n:()";

pr_list         printers = NULL;
pr_queue        queue = NULL;

/*
**=====================================================================
**                      C O D E   S E C T I O N                       *
**=====================================================================
*/

/*
 * This is the latest word on the security check. The following
 * routine "suspicious()" returns non-zero if the character string
 * passed to it contains any shell metacharacters.
 * Callers will typically code
 *
 *	if(suspicious(some_parameter)) reject();
 */

int suspicious (s)
char *s;
{
	if(strpbrk(pathname, ";|&<>`'#!?*()[]^") != NULL)
		return 1;
	return 0;
}


int
valid_pr(pr)
char *pr;
{
char *p;
pr_list curr;
	if(printers == NULL)
		build_pr_list();

	if(printers == NULL)
		return(1); /* can't tell - assume it's good */

	p = map_printer_name(pr);
	if (p == NULL)
		return(1);	/* must be ok is maps to NULL! */
	curr = printers;
	while(curr) {
		if(!strcmp(p, curr->pn))
			return(1);
		curr = curr->pr_next;
	}
		
	return(0);
}

pirstat pr_init(sys, pr, sp)
char *sys;
char *pr;
char**sp;
{
int    dir_mode = 0777;
int rc;

	*sp = &pathname[0];
	pathname[0] = '\0';

	if(suspicious(sys) || suspicious(pr))
		return(PI_RES_FAIL);

	/* get pathname of current directory and return to client */

	(void)sprintf(pathname,"%s/%s",sp_name, sys);
	(void)mkdir(sp_name, dir_mode);	/* ignore the return code */
	(void)chmod(sp_name, dir_mode);
	rc = mkdir(pathname, dir_mode);	/* DON'T ignore this return code */
	if((rc < 0 && errno != EEXIST) ||
	   (chmod(pathname, dir_mode) != 0) ||
	   (stat(pathname, &statbuf) != 0) ||
	   !(statbuf.st_mode & S_IFDIR)) {
	   (void)sprintf(tempstr,
		         "rpc.pcnfsd: unable to set up spool directory %s\n",
		 	  pathname);
            msg_out(tempstr);
	    pathname[0] = '\0';	/* null to tell client bad vibes */
	    return(PI_RES_FAIL);
	    }
 	if (!valid_pr(pr)) 
           {
	    pathname[0] = '\0';	/* null to tell client bad vibes */
	    return(PI_RES_NO_SUCH_PRINTER);
	    } 
	return(PI_RES_OK);

}


psrstat pr_start2(system, pr, user, fname, opts, id)
char *system;
char *pr;
char *user;
char *fname;
char *opts;
char **id;
{
char            snum[20];
static char     req_id[256];
char            cmdbuf[256];
char            resbuf[256];
FILE *fd;
int i;
char *xcmd;
char *cp;
int failed = 0;

#ifdef HACK_FOR_ROTATED_TRANSCRIPT
char            scratch[512];
#endif


	if(suspicious(system) || 
		suspicious(pr) ||
		suspicious(user) ||
		suspicious(fname))
		return(PS_RES_FAIL);

	(void)sprintf(pathname,"%s/%s/%s",sp_name,
	                         system,
	                         fname);	

	*id = &req_id[0];
	req_id[0] = '\0';

	if (stat(pathname, &statbuf)) 
           {
	   /*
           **-----------------------------------------------------------------
	   ** We can't stat the file. Let's try appending '.spl' and
	   ** see if it's already in progress.
           **-----------------------------------------------------------------
	   */

	   (void)strcat(pathname, ".spl");
	   if (stat(pathname, &statbuf)) 
	      {
	      /*
              **----------------------------------------------------------------
	      ** It really doesn't exist.
              **----------------------------------------------------------------
	      */


	      return(PS_RES_NO_FILE);
	      }
	      /*
              **-------------------------------------------------------------
	      ** It is already on the way.
              **-------------------------------------------------------------
	      */


		return(PS_RES_ALREADY);
	     }

	if (statbuf.st_size == 0) 
	   {
	   /*
           **-------------------------------------------------------------
	   ** Null file - don't print it, just kill it.
           **-------------------------------------------------------------
	   */
	   (void)unlink(pathname);

	    return(PS_RES_NULL);
	    }
	 /*
         **-------------------------------------------------------------
	 ** The file is real, has some data, and is not already going out.
	 ** We rename it by appending '.spl' and exec "lpr" to do the
	 ** actual work.
         **-------------------------------------------------------------
	 */
	(void)strcpy(new_pathname, pathname);
	(void)strcat(new_pathname, ".spl");

	/*
        **-------------------------------------------------------------
	** See if the new filename exists so as not to overwrite it.
        **-------------------------------------------------------------
	*/


	if (!stat(new_pathname, &statbuf)) 
	   {
	   (void)strcpy(new_pathname, pathname);  /* rebuild a new name */
	   (void)sprintf(snum, "%d", rand());	  /* get some number */
	   (void)strncat(new_pathname, snum, 3);
	   (void)strcat(new_pathname, ".spl");	  /* new spool file */
	    }
	if (rename(pathname, new_pathname)) 
	   {
	   /*
           **---------------------------------------------------------------
	   ** Should never happen.
           **---------------------------------------------------------------
           */
	   (void)sprintf(tempstr, "rpc.pcnfsd: spool file rename (%s->%s) failed.\n",
			pathname, new_pathname);
                msg_out(tempstr);
		return(PS_RES_FAIL);
	    }

		if (*opts == 'd') 
	           {
		   /*
                   **------------------------------------------------------
		   ** This is a Diablo print stream. Apply the ps630
		   ** filter with the appropriate arguments.
                   **------------------------------------------------------
		   */
		   (void)run_ps630(new_pathname, opts);
		   }
		/*
		** Try to match to an aliased printer
		*/
		xcmd = expand_alias(pr, new_pathname, user, system);
	        /*
                **----------------------------------------------------------
	        **  Use the copy option so we can remove the orignal spooled
	        **  nfs file from the spool directory.
                **----------------------------------------------------------
	        */
		if(!xcmd) {
			sprintf(cmdbuf, "%s/lpr -P%s %s",
				LPRDIR, pr, new_pathname);
			xcmd = cmdbuf;
		}
		if ((fd = su_popen(user, xcmd, MAXTIME_FOR_PRINT)) == NULL) {
			msg_out("rpc.pcnfsd: su_popen failed");
			return(PS_RES_FAIL);
		}
		req_id[0] = '\0';	/* asume failure */
		while(fgets(resbuf, 255, fd) != NULL) {
			i = strlen(resbuf);
			if(i)
				resbuf[i-1] = '\0'; /* trim NL */
			if(!strncmp(resbuf, "request id is ", 14))
				/* New - just the first word is needed */
				strcpy(req_id, strtok(&resbuf[14], delims));
			else if (strembedded("disabled", resbuf))
				failed = 1;
		}
		if(su_pclose(fd) == 255)
			msg_out("rpc.pcnfsd: su_pclose alert");
		(void)unlink(new_pathname);
		return((failed | interrupted)? PS_RES_FAIL : PS_RES_OK);
}


/*
 * determine which printers are valid...
 * on BSD use "lpc status"
 * on SVR4 use "lpstat -v"
 */
int
build_pr_list()
{
	pr_list last = NULL;
	pr_list curr = NULL;
	char buff[256];
	FILE *p;
	char *cp;
	int saw_system;

	sprintf(buff, "%s/lpc status", LPCDIR);
	p = popen(buff, "r");
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen lpc stat");
		return(0);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		if (isspace(buff[0]))
			continue;

		if ((cp = strtok(buff, delims)) == NULL)
			continue;

		curr = (struct pr_list_item *)
			grab(sizeof (struct pr_list_item));

		/* XXX - Should distinguish remote printers. */
		curr->pn = strdup(cp);
		curr->device = strdup(cp);
		curr->remhost = strdup("");
		curr->cm = strdup("-");
		curr->pr_next = NULL;

		if(last == NULL)
			printers = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) fclose(p);

	/*
	 ** Now add on the virtual printers, if any
	 */
	if(last == NULL)
		printers = list_virtual_printers();
	else
		last->pr_next = list_virtual_printers();

	return(1);
}

void *grab(n)
int n;
{
void *p;

	p = (void *)malloc(n);
	if(p == NULL) {
		msg_out("rpc.pcnfsd: malloc failure");
		exit(1);
	}
	return(p);
}

void
free_pr_list_item(curr)
pr_list curr;
{
	if(curr->pn)
		free(curr->pn);
	if(curr->device)
		free(curr->device);
	if(curr->remhost)
		free(curr->remhost);
	if(curr->cm)
		free(curr->cm);
	if(curr->pr_next)
		free_pr_list_item(curr->pr_next); /* recurse */
	free(curr);
}
/*
** Print queue handling.
**
** Note that the first thing we do is to discard any
** existing queue.
*/

pirstat
build_pr_queue(pn, user, just_mine, p_qlen, p_qshown)
printername     pn;
username        user;
int            just_mine;
int            *p_qlen;
int            *p_qshown;
{
pr_queue last = NULL;
pr_queue curr = NULL;
char buff[256];
FILE *p;
char *cp;
int i;
char *rank;
char *owner;
char *job;
char *files;
char *totsize;

	if(queue) {
		free_pr_queue_item(queue);
		queue = NULL;
	}
	*p_qlen = 0;
	*p_qshown = 0;
	pn = map_printer_name(pn);
	if(pn == NULL || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);

	sprintf(buff, "%s/lpq -P%s", LPRDIR, pn);

	p = su_popen(user, buff, MAXTIME_FOR_QUEUE);
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen() lpq");
		return(PI_RES_FAIL);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		i = strlen(buff) - 1;
		buff[i] = '\0';		/* zap trailing NL */
		if(i < SIZECOL)
			continue;
		if(!mystrncasecmp(buff, "rank", 4))
			continue;

		totsize = &buff[SIZECOL-1];
		files = &buff[FILECOL-1];
		cp = totsize;
		cp--;
		while(cp > files && isspace(*cp))
			*cp-- = '\0';

		buff[FILECOL-2] = '\0';

		cp = strtok(buff, delims);
		if(!cp)
			continue;
		rank = cp;

		cp = strtok(NULL, delims);
		if(!cp)
			continue;
		owner = cp;

		cp = strtok(NULL, delims);
		if(!cp)
			continue;
		job = cp;

		*p_qlen += 1;

		if(*p_qshown > QMAX)
			continue;

		if(just_mine && mystrcasecmp(owner, user))
			continue;

		*p_qshown += 1;

		curr = (struct pr_queue_item *)
			grab(sizeof (struct pr_queue_item));

		curr->position = atoi(rank); /* active -> 0 */
		curr->id = strdup(job);
		curr->size = strdup(totsize);
		curr->status = strdup(rank);
		curr->system = strdup("");
		curr->user = strdup(owner);
		curr->file = strdup(files);
		curr->cm = strdup("-");
		curr->pr_next = NULL;

		if(last == NULL)
			queue = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) su_pclose(p);
	return(PI_RES_OK);
}

void
free_pr_queue_item(curr)
pr_queue curr;
{
	if(curr->id)
		free(curr->id);
	if(curr->size)
		free(curr->size);
	if(curr->status)
		free(curr->status);
	if(curr->system)
		free(curr->system);
	if(curr->user)
		free(curr->user);
	if(curr->file)
		free(curr->file);
	if(curr->cm)
		free(curr->cm);
	if(curr->pr_next)
		free_pr_queue_item(curr->pr_next); /* recurse */
	free(curr);
}



pirstat
get_pr_status(pn, avail, printing, qlen, needs_operator, status)
printername   pn;
bool_t       *avail;
bool_t       *printing;
int          *qlen;
bool_t       *needs_operator;
char         *status;
{
char cmd[128];
char buff[256];
char buff2[256];
char pname[64];
FILE *p;
char *cp;
char *cp1;
char *cp2;
int n;
pirstat stat = PI_RES_NO_SUCH_PRINTER;

	/* assume the worst */
	*avail = FALSE;
	*printing = FALSE;
	*needs_operator = FALSE;
	*qlen = 0;
	*status = '\0';

	pn = map_printer_name(pn);
	if(pn == NULL || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);

	sprintf(pname, "%s:", pn);
	n = strlen(pname);

#if 0
/*
 * We used to use lpstat -a, but it breaks because of
 * printer alias inconsistency
 */
	p = popen("/usr/bin/lpstat -a", "r");
#else
/*
 * So now we use:  lpc status
 */
	sprintf(cmd, "%s/lpc status %s", LPCDIR, pn);
	p = popen(cmd, "r");
#endif
	if(p == NULL) {
		msg_out("rpc.pcnfsd: unable to popen() lp status");
		return(PI_RES_FAIL);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		if(strncmp(buff, pname, n))
			continue;
/*
** We have a match. The only failure now is PI_RES_FAIL if
** lpstat output cannot be decoded
*/
		stat = PI_RES_FAIL;
/*
** The next four lines are usually if the form
**
**     queuing is [enabled|disabled]
**     printing is [enabled|disabled]
**     [no entries | N entr[y|ies] in spool area]
**     <status message, may include the word "attention">
*/
		while(fgets(buff, 255, p) != NULL && isspace(buff[0])) {
			cp = buff;
			while(isspace(*cp))
				cp++;
			if(*cp == '\0')
				break;
			cp1 = cp;
			cp2 = buff2;
			while(*cp1 && *cp1 != '\n') {
				*cp2++ = tolower(*cp1);
				cp1++;
			}
			*cp1 = '\0';
			*cp2 = '\0';
/*
** Now buff2 has a lower-cased copy and cp points at the original;
** both are null terminated without any newline
*/			
			if(!strncmp(buff2, "queuing", 7)) {
				*avail = (strstr(buff2, "enabled") != NULL);
				continue;
			}
			if(!strncmp(buff2, "printing", 8)) {
				*printing = (strstr(buff2, "enabled") != NULL);
				continue;
			}
			if(isdigit(buff2[0]) && (strstr(buff2, "entr") !=NULL)) {

				*qlen = atoi(buff2);
				continue;
			}
			if(strstr(buff2, "attention") != NULL ||
			   strstr(buff2, "error") != NULL)
				*needs_operator = TRUE;
			if(*needs_operator || strstr(buff2, "waiting") != NULL)
				strcpy(status, cp);
		}
		stat = PI_RES_OK;
		break;
	}
	(void) pclose(p);
	return(stat);
}


pcrstat pr_cancel(pr, user, id)
char *pr;
char *user;
char *id;
{
char            cmdbuf[256];
char            resbuf[256];
FILE *fd;
int i;
pcrstat stat = PC_RES_NO_SUCH_JOB;

	pr = map_printer_name(pr);
	if(pr == NULL || suspicious(pr))
		return(PC_RES_NO_SUCH_PRINTER);
	if(suspicious(id))
		return(PC_RES_NO_SUCH_JOB);

		sprintf(cmdbuf, "%s/lprm -P%s %s", LPRDIR, pr, id);
		if ((fd = su_popen(user, cmdbuf, MAXTIME_FOR_CANCEL)) == NULL) {
			msg_out("rpc.pcnfsd: su_popen failed");
			return(PC_RES_FAIL);
		}
		while(fgets(resbuf, 255, fd) != NULL) {
			i = strlen(resbuf);
			if(i)
				resbuf[i-1] = '\0'; /* trim NL */
			if(strstr(resbuf, "dequeued") != NULL)
				stat = PC_RES_OK;
			if(strstr(resbuf, "unknown printer") != NULL)
				stat = PC_RES_NO_SUCH_PRINTER;
			if(strstr(resbuf, "Permission denied") != NULL)
				stat = PC_RES_NOT_OWNER;
		}
		if(su_pclose(fd) == 255)
			msg_out("rpc.pcnfsd: su_pclose alert");
		return(stat);
}


/*
** New subsystem here. We allow the administrator to define
** up to NPRINTERDEFS aliases for printer names. This is done
** using the "/etc/pcnfsd.conf" file, which is read at startup.
** There are three entry points to this subsystem
**
** void add_printer_alias(char *printer, char *alias_for, char *command)
**
** This is invoked from "config_from_file()" for each
** "printer" line. "printer" is the name of a printer; note that
** it is possible to redefine an existing printer. "alias_for"
** is the name of the underlying printer, used for queue listing
** and other control functions. If it is "-", there is no
** underlying printer, or the administrative functions are
** not applicable to this printer. "command"
** is the command which should be run (via "su_popen()") if a
** job is printed on this printer. The following tokens may be
** embedded in the command, and are substituted as follows:
**
** $FILE	-	path to the file containing the print data
** $USER	-	login of user
** $HOST	-	hostname from which job originated
**
** Tokens may occur multiple times. If The command includes no
** $FILE token, the string " $FILE" is silently appended.
**
** pr_list list_virtual_printers()
**
** This is invoked from build_pr_list to generate a list of aliased
** printers, so that the client that asks for a list of valid printers
** will see these ones.
**
** char *map_printer_name(char *printer)
**
** If "printer" identifies an aliased printer, this function returns
** the "alias_for" name, or NULL if the "alias_for" was given as "-".
** Otherwise it returns its argument.
**
** char *expand_alias(char *printer, char *file, char *user, char *host)
**
** If "printer" is an aliased printer, this function returns a
** pointer to a static string in which the corresponding command
** has been expanded. Otherwise ot returns NULL.
*/
#define NPRINTERDEFS	16
int num_aliases = 0;
struct {
	char *a_printer;
	char *a_alias_for;
	char *a_command;
} alias [NPRINTERDEFS];



void
add_printer_alias(printer, alias_for, command)
char *printer;
char *alias_for;
char *command;
{
	if(num_aliases < NPRINTERDEFS) {
		alias[num_aliases].a_printer = strdup(printer);
		alias[num_aliases].a_alias_for =
			(strcmp(alias_for,  "-") ? strdup(alias_for) : NULL);
		if(strstr(command, "$FILE"))
			alias[num_aliases].a_command = strdup(command);
		else {
			alias[num_aliases].a_command = (char *)grab(strlen(command) + 8);
			strcpy(alias[num_aliases].a_command, command);
			strcat(alias[num_aliases].a_command, " $FILE");
		}
		num_aliases++;
	}
}

pr_list list_virtual_printers()
{
pr_list first = NULL;
pr_list last = NULL;
pr_list curr = NULL;
int i;


	if(num_aliases == 0)
		return(NULL);

	for (i = 0; i < num_aliases; i++) {
		curr = (struct pr_list_item *)
			grab(sizeof (struct pr_list_item));

		curr->pn = strdup(alias[i].a_printer);
		if(alias[i].a_alias_for == NULL)
			curr->device = strdup("");
		else
			curr->device = strdup(alias[i].a_alias_for);
		curr->remhost = strdup("");
		curr->cm = strdup("(alias)");
		curr->pr_next = NULL;
		if(last == NULL)
			first = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	return(first);
}


char *
map_printer_name(printer)
char *printer;
{
int i;
	for (i = 0; i < num_aliases; i++){
		if(!strcmp(printer, alias[i].a_printer))
			return(alias[i].a_alias_for);
	}
	return(printer);
}

static void
substitute(string, token, data)
char *string;
char *token;
char *data;
{
char temp[512];
char *c;

	while(c = strstr(string, token)) {
		*c = '\0';
		strcpy(temp, string);
		strcat(temp, data);
		c += strlen(token);
		strcat(temp, c);
		strcpy(string, temp);
	}
}

char *
expand_alias(printer, file, user, host)
char *printer;
char *file;
char *user;
char *host;
{
static char expansion[512];
int i;
	for (i = 0; i < num_aliases; i++){
		if(!strcmp(printer, alias[i].a_printer)) {
			strcpy(expansion, alias[i].a_command);
			substitute(expansion, "$FILE", file);
			substitute(expansion, "$USER", user);
			substitute(expansion, "$HOST", host);
			return(expansion);
		}
	}
	return(NULL);
}

