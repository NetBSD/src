/* $NetBSD: srtconfig.c,v 1.2.20.1 2009/05/13 19:20:41 jym Exp $ */
/* This file is in the public domain. */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if_srt.h>

extern const char *__progname;

#define ACT_ERROR     1 /* none of the below */
#define ACT_QUERYALL  2 /* srtX */
#define ACT_QUERYONE  3 /* srtX N */
#define ACT_DEL       4 /* srtX del N */
#define ACT_ADD       5 /* srtX add srcaddr mask dstif dstaddr */
#define ACT_SET       6 /* srtX set N srcaddr mask dstif dstaddr */
#define ACT_FLAGS     7 /* srtX flags */
#define ACT_SFLAG     8 /* srtX flags [+|-]flag */
#define ACT_DEBUG     9 /* srtX debug */
static int action = ACT_ERROR;

static char *txt_dev;
static char *txt_n;
static char *txt_flg;
static char *txt_addr;
static char *txt_mask;
static char *txt_dstif;
static char *txt_dstaddr;

static int devfd;

static struct {
	 const char *name;
	 unsigned int bit;
	 } flagbits[] = { { "mtulock", SSF_MTULOCK },
			  { 0, 0 } };

static void handleargs(int ac, char **av)
{
 txt_dev = av[1];
 if (ac == 2)
  { action = ACT_QUERYALL;
  }
 else if ((ac == 3) && !strcmp(av[2],"debug"))
  { action = ACT_DEBUG;
  }
 else if ((ac == 3) && !strcmp(av[2],"flags"))
  { action = ACT_FLAGS;
  }
 else if (ac == 3)
  { action = ACT_QUERYONE;
    txt_n = av[2];
  }
 else if ((ac == 4) && !strcmp(av[2],"del"))
  { action = ACT_DEL;
    txt_n = av[3];
  }
 else if ((ac == 4) && !strcmp(av[2],"flags"))
  { action = ACT_SFLAG;
    txt_flg = av[3];
  }
 else if ((ac == 7) && !strcmp(av[2],"add"))
  { action = ACT_ADD;
    txt_addr = av[3];
    txt_mask = av[4];
    txt_dstif = av[5];
    txt_dstaddr = av[6];
  }
 else if ((ac == 8) && !strcmp(av[2],"set"))
  { action = ACT_SET;
    txt_n = av[3];
    txt_addr = av[4];
    txt_mask = av[5];
    txt_dstif = av[6];
    txt_dstaddr = av[7];
  }
 if (action == ACT_ERROR)
  { fprintf(stderr,"Usage: %s srtX\n",__progname);
    fprintf(stderr,"       %s srtX N\n",__progname);
    fprintf(stderr,"       %s srtX del N\n",__progname);
    fprintf(stderr,"       %s srtX add addr mask dstif dstaddr\n",__progname);
    fprintf(stderr,"       %s srtX set N addr mask dstif dstaddr\n",__progname);
    fprintf(stderr,"       %s srtX flags {[+|-]flag}\n",__progname);
    fprintf(stderr,"       %s srtX debug\n",__progname);
    exit(1);
  }
}

static void open_dev(int how)
{
 if (! index(txt_dev,'/'))
  { char *tmp;
    asprintf(&tmp,"/dev/%s",txt_dev);
    txt_dev = tmp;
  }
 devfd = open(txt_dev,how,0);
 if (devfd < 0)
  { fprintf(stderr,"%s; can't open %s: %s\n",__progname,txt_dev,strerror(errno));
    exit(1);
  }
}

static void query_n(int n)
{
 struct srt_rt r;
 char obuf[64];

 r.inx = n;
 if (ioctl(devfd,SRT_GETRT,&r) < 0)
  { fprintf(stderr,"%s: can't get rt #%d: %s\n",__progname,n,strerror(errno));
    return;
  }
 printf("%d:",n);
 printf(" %s",inet_ntop(r.af,&r.srcmatch,&obuf[0],sizeof(obuf)));
 printf(" /%d",r.srcmask);
 printf(" %.*s",(int)sizeof(r.u.dstifn),&r.u.dstifn[0]);
 switch (r.af)
  { case AF_INET:
       printf(" %s",inet_ntoa(r.dst.sin.sin_addr));
       break;
    case AF_INET6:
       printf(" %s",inet_ntop(AF_INET6,&r.dst.sin6.sin6_addr,&obuf[0],sizeof(obuf)));
       break;
    default:
       printf(" ?af%d",r.af);
       break;
  }
 printf("\n");
}

static void do_query(int narg)
{
 int i;
 int n;

 open_dev(O_RDONLY);
 if (narg >= 0)
  { query_n(narg);
  }
 else
  { if (ioctl(devfd,SRT_GETNRT,&n) < 0)
     { fprintf(stderr,"%s: can't get count: %s\n",__progname,strerror(errno));
       exit(1);
     }
    for (i=0;i<n;i++) query_n(i);
  }
}

static void do_del(unsigned int n)
{
 open_dev(O_RDWR);
 if (ioctl(devfd,SRT_DELRT,&n) < 0)
  { fprintf(stderr,"%s: can't delete #%u: %s\n",__progname,n,strerror(errno));
    exit(1);
  }
}

static void do_set(int n)
{
 struct srt_rt r;
 int w;
 int maxw;
 void *dp;

 open_dev(O_RDWR);
 if (n < 0)
  { unsigned int v;
    if (ioctl(devfd,SRT_GETNRT,&v) < 0)
     { fprintf(stderr,"%s: can't get count: %s\n",__progname,strerror(errno));
       exit(1);
     }
    n = v;
  }
 bzero(&r.dst,sizeof(r.dst));
 r.inx = n;
 if (inet_pton(AF_INET,txt_addr,&r.srcmatch.v4) == 1)
  { r.af = AF_INET;
    r.dst.sin.sin_family = AF_INET;
    r.dst.sin.sin_len = sizeof(r.dst.sin);
    dp = &r.dst.sin.sin_addr;
    maxw = 32;
  }
 else if (inet_pton(AF_INET6,txt_addr,&r.srcmatch.v6) == 1)
  { r.af = AF_INET6;
    r.dst.sin6.sin6_family = AF_INET6;
    r.dst.sin6.sin6_len = sizeof(r.dst.sin6);
    dp = &r.dst.sin6.sin6_addr;
    maxw = 128;
  }
 else
  { fprintf(stderr,"%s: %s: invalid match address\n",__progname,txt_addr);
    exit(1);
  }
 if (txt_mask[0] == '/') txt_mask ++;
 w = atoi(txt_mask);
 if ((w < 0) || (w > maxw))
  { fprintf(stderr,"%s: %s: out-of-range CIDR width\n",__progname,txt_mask);
    exit(1);
  }
 r.srcmask = w;
 if (strlen(txt_dstif) > sizeof(r.u.dstifn)-1)
  { fprintf(stderr,"%s: %s: too long\n",__progname,txt_dstif);
    exit(1);
  }
 strncpy(&r.u.dstifn[0],txt_dstif,sizeof(r.u.dstifn));
 if (inet_pton(r.af,txt_dstaddr,dp) != 1)
  { fprintf(stderr,"%s: %s: invalid destination address\n",__progname,txt_dstaddr);
    exit(1);
  }
 if (ioctl(devfd,SRT_SETRT,&r) < 0)
  { fprintf(stderr,"%s: can't set route: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void do_flags(void)
{
 unsigned int f;
 int i;

 open_dev(O_RDONLY);
 if (ioctl(devfd,SRT_GFLAGS,&f) < 0)
  { fprintf(stderr,"%s: can't get flags: %s\n",__progname,strerror(errno));
    exit(1);
  }
 for (i=0;flagbits[i].name;i++)
  { printf(" %c%s",(f&flagbits[i].bit)?'+':'-',flagbits[i].name);
    f &= ~flagbits[i].bit;
  }
 if (f) printf(" +0x%x",f);
 printf("\n");
}

static void do_sflag(void)
{
 unsigned int f;
 unsigned int b;
 int i;

 switch (txt_flg[0])
  { case '+': case '-': break;
    default:
       fprintf(stderr,"%s: last argument must be +flag or -flag\n",__progname);
       exit(1);
       break;
  }
 for (i=0;flagbits[i].name;i++) if (!strcmp(flagbits[i].name,txt_flg+1)) break;
 if (! flagbits[i].name)
  { fprintf(stderr,"%s: unrecognized flag bit `%s'\n",__progname,txt_flg+1);
    exit(1);
  }
 b = flagbits[i].bit;
 open_dev(O_RDWR);
 if (ioctl(devfd,SRT_GFLAGS,&f) < 0)
  { fprintf(stderr,"%s: can't get flags: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (txt_flg[0] == '+') f |= b; else f &= ~b;
 if (ioctl(devfd,SRT_SFLAGS,&f) < 0)
  { fprintf(stderr,"%s: can't set flags: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void do_debug(void)
{
 void *vp;

 open_dev(O_RDWR);
 vp = 0;
 if (ioctl(devfd,SRT_DEBUG,&vp) < 0)
  { fprintf(stderr,"%s: can't SRT_DEBUG: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 switch (action)
  { case ACT_QUERYALL:
       do_query(-1);
       break;
    case ACT_QUERYONE:
       do_query(atoi(txt_n));
       break;
    case ACT_DEL:
       do_del(atoi(txt_n));
       break;
    case ACT_ADD:
       do_set(-1);
       break;
    case ACT_SET:
       do_set(atoi(txt_n));
       break;
    case ACT_FLAGS:
       do_flags();
       break;
    case ACT_SFLAG:
       do_sflag();
       break;
    case ACT_DEBUG:
       do_debug();
       break;
    default:
       abort();
       break;
  }
 exit(0);
}
