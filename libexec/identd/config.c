/*	$NetBSD: config.c,v 1.6 2000/07/03 03:34:26 matt Exp $	*/

/*
** config.c                         This file handles the config file
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 6 Dec 1992
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "error.h"
#include "identd.h"
#include "paths.h"

#define MAXLINE         1024
#define MAXNETS         1024
 
u_long localnet[MAXNETS], localmask[MAXNETS];
int netcnt;

int parse_config(path, silent_flag)
  char *path;
  int silent_flag;
{
  FILE *fp;
  char *start, buf[MAXLINE];

  if (!path)
    path = PATH_CONFIG;
  
  fp = fopen(path, "r");
  if (!fp)
  {
    if (silent_flag)
      return 0;

    ERROR1("error opening %s", path);
  }

  netcnt = 0;
  while ((start = fgets(buf, sizeof buf, fp)) != NULL) {
    char *net, *mask;
    char *cmd = strtok(buf, " \t");
    if (cmd) {
      if (!strcmp("trusted-net:", cmd)) {
        if (netcnt >= MAXNETS) {
          if (!silent_flag)
            ERROR1("too many networks defined in %s", path);
          return 0;
        }
        net = strtok(NULL, " \t");
        if (net) {
          localnet[netcnt] = inet_network(net);
          mask = strtok(NULL, " \t");
          if (mask) {
            localmask[netcnt] = inet_network(mask);
          } else {
            localmask[netcnt] = 0xFFFFFFFFL;
          }
          if ((localnet[netcnt] & localmask[netcnt]) == localnet[netcnt]) {
            netcnt++;
          } else if (!silent_flag) {
            ERROR1("netmask does not match network %s", net);
          }
        } else {
          if (!silent_flag)
            ERROR2("format error in %s cmd %s", path, cmd);
        }
      } else if (*cmd != '#') {
        ERROR2("unknown config statement %s in file %s", cmd, path);
      }
    }
  }

  fclose(fp);
  return 0;
}
