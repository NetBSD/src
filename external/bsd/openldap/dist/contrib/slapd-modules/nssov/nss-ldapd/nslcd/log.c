/*
   log.c - logging funtions

   Copyright (C) 2002, 2003 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "log.h"


/* set the logname */
#undef PACKAGE
#define PACKAGE "nslcd"


/* storage for logging modes */
static struct cvsd_log {
  FILE *fp; /* NULL==syslog */
  int loglevel;
  struct cvsd_log *next;
} *cvsd_loglist=NULL;


/* default loglevel when no logging is configured */
static int prelogging_loglevel=LOG_INFO;


/* set loglevel when no logging is configured */
void log_setdefaultloglevel(int loglevel)
{
  prelogging_loglevel=loglevel;
}


/* add logging method to configuration list */
static void log_addlogging_fp(FILE *fp,int loglevel)
{
  struct cvsd_log *tmp,*lst;
  /* create new logstruct */
  tmp=(struct cvsd_log *)malloc(sizeof(struct cvsd_log));
  if (tmp==NULL)
  {
    fprintf(stderr,"malloc() failed: %s",strerror(errno));
    /* since this is done during initialisation it's best to bail out */
    exit(EXIT_FAILURE);
  }
  tmp->fp=fp;
  tmp->loglevel=loglevel;
  tmp->next=NULL;
  /* save the struct in the list */
  if (cvsd_loglist==NULL)
    cvsd_loglist=tmp;
  else
  {
    for (lst=cvsd_loglist;lst->next!=NULL;lst=lst->next);
    lst->next=tmp;
  }
}


/* configure logging to a file */
void log_addlogging_file(const char *filename,int loglevel)
{
  FILE *fp;
  fp=fopen(filename,"a");
  if (fp==NULL)
  {
    log_log(LOG_ERR,"cannot open logfile (%s) for appending: %s",filename,strerror(errno));
    exit(EXIT_FAILURE);
  }
  log_addlogging_fp(fp,loglevel);
}


/* configure logging to syslog */
void log_addlogging_syslog(int loglevel)
{
  openlog(PACKAGE,LOG_PID,LOG_DAEMON);
  log_addlogging_fp(NULL,loglevel);
}


/* configure a null logging mode (no logging) */
void log_addlogging_none()
{
  /* this is a hack, but it's so easy */
  log_addlogging_fp(NULL,LOG_EMERG);
}


/* start the logging with the configured logging methods
   if no method is configured yet, logging is done to syslog */
void log_startlogging(void)
{
  if (cvsd_loglist==NULL)
    log_addlogging_syslog(LOG_INFO);
  prelogging_loglevel=-1;
}


/* log the given message using the configured logging method */
void log_log(int pri,const char *format, ...)
{
  int res;
  struct cvsd_log *lst;
  /* TODO: make this something better */
  #define maxbufferlen 200
  char buffer[maxbufferlen];
  va_list ap;
  /* make the message */
  va_start(ap,format);
  res=vsnprintf(buffer,maxbufferlen,format,ap);
  if ((res<0)||(res>=maxbufferlen))
  {
    /* truncate with "..." */
    buffer[maxbufferlen-2]='.';
    buffer[maxbufferlen-3]='.';
    buffer[maxbufferlen-4]='.';
  }
  buffer[maxbufferlen-1]='\0';
  va_end(ap);
  /* do the logging */
  if (prelogging_loglevel>=0)
  {
    /* if logging is not yet defined, log to stderr */
    if (pri<=prelogging_loglevel)
      fprintf(stderr,"%s: %s%s\n",PACKAGE,pri==LOG_DEBUG?"DEBUG: ":"",buffer);
  }
  else
  {
    for (lst=cvsd_loglist;lst!=NULL;lst=lst->next)
    {
      if (pri<=lst->loglevel)
      {
        if (lst->fp==NULL) /* syslog */
          syslog(pri,"%s",buffer);
        else /* file */
        {
          fprintf(lst->fp,"%s: %s\n",PACKAGE,buffer);
          fflush(lst->fp);
        }
      }
    }
  }
}


/* return the syslog loglevel represented by the string
   return -1 on unknown */
int log_getloglevel(const char *lvl)
{
  if ( strcmp(lvl,"crit")==0 )
    return LOG_CRIT;
  else if ( (strcmp(lvl,"error")==0) ||
            (strcmp(lvl,"err")==0) )
    return LOG_ERR;
  else if ( strcmp(lvl,"warning")==0 )
    return LOG_WARNING;
  else if ( strcmp(lvl,"notice")==0 )
    return LOG_NOTICE;
  else if ( strcmp(lvl,"info")==0 )
    return LOG_INFO;
  else if ( strcmp(lvl,"debug")==0 )
    return LOG_DEBUG;
  else
    return -1; /* unknown */
}
