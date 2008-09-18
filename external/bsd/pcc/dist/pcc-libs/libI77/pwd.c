/*	$Id: pwd.c,v 1.1.1.1.2.2 2008/09/18 05:15:46 wrstuden Exp $	*/
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
/*
 * Print working (current) directory
 */
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>

char	dot[]	".";
char	dotdot[]	"..";
char	root[]	"/";
char	name[512];
int	file;
int	off	-1;
struct	stat	x;
struct	direct	y;

main()
{
	for (;;) {
		stat(dot, &x);
		if ((file = open(dotdot,0)) < 0) prname();
		do {
			if (read(file, &y, sizeof(y)) < sizeof(y))
				prname();
		} while (y.d_ino != x.st_ino);
		close(file);
		if (y.d_ino == 2)
			ckroot();
		cat();
		chdir(dotdot);
	}
}

ckroot()
{
	register i;

	if (stat(y.d_name,&x)<0 || chdir(root)<0 || (file=open(root,0))<0)
		prname();
	i = x.st_dev;
	do {
		if (read(file,&y,sizeof(y)) < sizeof(y))
			prname();
		if (y.d_ino == 0)
			continue;
		if (stat(y.d_name,&x) < 0)
			prname();
	} while (x.st_dev!=i || (x.st_mode&S_IFMT)!=S_IFDIR);
	if (strcmp(dot, y.d_name) || strcmp(dotdot, y.d_name))
		cat();
	write(1, root, 1);
	prname();
}

prname()
{
	if (off<0)
		off = 0;
	name[off] = '\n';
	write(1, name, off+1);
	exit(0);
}

cat()
{
	register i, j;

	i = -1;
	while (y.d_name[++i] != 0);
	if ((off+i+2) > 511)
		prname();
	for(j=off+1; j>=0; --j)
		name[j+i+1] = name[j];
	off=i+off+1;
	name[i] = root[0];
	for(--i; i>=0; --i)
		name[i] = y.d_name[i];
}
