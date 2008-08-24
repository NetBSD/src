/*	$Id: util.c,v 1.1.1.1 2008/08/24 05:34:48 gmcgarry Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fio.h"

static void dcat(char *a,char *b);

#define DIRSIZE	14

void
g_char(char *a, ftnlen alen, char *b)
{
	char *x=a+alen-1,*y=b+alen-1;
	*(y+1)=0;
	for(;x>=a && *x==' ';x--) *y--=0;
	for(;x>=a;*y--= *x--);
}

void
b_char(char *a, char *b, ftnlen blen)
{
	int i;
	for(i=0;i<blen && *a!=0;i++) *b++= *a++;
	for(;i<blen;i++) *b++=' ';
}

int
inode(char *a)
{
	struct stat x;
	if(stat(a,&x)<0) return(-1);
	return(x.st_ino);
}
#define DONE {*bufpos++=0; close(file); return;}
#define INTBOUND (sizeof(int)-1)
#define register 

static void
mvgbt(int n,int len,char *a,char *b)
{	register int num=n*len;
	if( ((int)a&INTBOUND)==0 && ((int)b&INTBOUND)==0 && (num&INTBOUND)==0 )
	{	register int *x=(int *)a,*y=(int *)b;
		num /= sizeof(int);
		if(x>y) for(;num>0;num--) *y++= *x++;
		else for(num--;num>=0;num--) *(y+num)= *(x+num);
	}
	else
	{	register char *x=a,*y=b;
		if(x>y) for(;num>0;num--) *y++= *x++;
		else for(num--;num>=0;num--) *(y+num)= *(x+num);
	}
}
static char *
curdir(void)
{	char name[256],*bufpos = name;
	struct stat x;
	struct dirent y;
	int file,i;
	*bufpos++ = 0;
loop:	stat(".",&x);
	if((file=open("..",0))<0) goto done;
	do
	{	if(read(file,&y,sizeof(y))<sizeof(y)) goto done;
	} while(y.d_ino!=x.st_ino);
	close(file);
	if(y.d_ino!=2)
	{	dcat(name,y.d_name);
		chdir("..");
		goto loop;
	}
	if(stat(y.d_name,&x)<0 || chdir("/")<0
		|| (file=open("/",0))<0) goto done;
	i=x.st_dev;
	do
	{	if(read(file,&y,sizeof(y))<sizeof(y)) goto done;
		if(y.d_ino==0) continue;
		if(stat(y.d_name,&x)<0) goto done;
	} while(x.st_dev!=i || (x.st_mode&S_IFMT)!=S_IFDIR);
	if(strcmp(".",y.d_name) || strcmp("..",y.d_name))
		dcat(name,y.d_name);
	dcat(name,"/");
done:
	bufpos=calloc(strlen(name)+1,1);
	strcpy(bufpos,name);
	chdir(name);
	close(file);
	return(bufpos);
}

void
dcat(char *a,char *b)
{
	int i,j;
	i=strlen(b);
	j=strlen(a);
	mvgbt(1,j+1,a,a+i+1);
	mvgbt(1,i,b,a);
	a[i]='/';
}

int
fullpath(char *a,char *b, int errflag)
{
	char *a1,*a2,*npart,*dpart,*p;
	a1=curdir();
	npart=NULL;
	for(p=a;*p!=0;p++)
		if(*p=='/') npart=p;
	if(npart==NULL)
	{	dpart=NULL;
		npart=a;
	}
	else
	{	dpart=a;
		*npart++ = 0;
	}
	if(dpart!=NULL)
	{	chdir(dpart);
		a2=curdir();
		strcpy(b,a2);
	}
	else
	{	a2=NULL;
		strcpy(b, a1);
	}
	strcat(b,npart);
	chdir(a1);
	if(a1!=NULL)
	{	free(a1);
		a1=NULL;
	}
	if(a2!=NULL)
	{	free(a2);
	}
	return(0);
}
