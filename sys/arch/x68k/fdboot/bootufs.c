/*	$NetBSD: bootufs.c,v 1.3 1996/06/17 07:28:58 oki Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Takumi Nakamura.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Takumi Nakamura.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/***************************************************************
 *
 *	file: bootufs.c
 *
 *	author: chapuni(GBA02750@niftyserve.or.jp)
 *
 *	参考にしたもの:	UNIX C PROGRAMMING (NUTSHELL)
 *			(俗に言うライオン本)
 *
 *	これからの野望;
 *	・意味もなくグラフィカルにしてみよう。
 *	・ufs アクセスモジュールは、/sys/lib/libsa のものが
 *	  使えるハズなので、ぜひともそれを使うようにしよう。
 *	・将来きっと FD DRIVER が出来上がるハズなので、それに
 *	  いち早く対応しよう。
 *
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ufs/ufs/quota.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <a.out.h>

#include "fdboot.h"
#include "../x68k/iodevice.h"

#define alloca __builtin_alloca
#define memcpy __builtin_memcpy

static inline void
bcopy (s, d, len)
	const char *s;
	char *d;
	long len;
{
	while (len--)
		*d++ = *s++;
}

static inline int
strcmp (a, b)
	const char *a;
	const char *b;
{
	while (*a && *b && *a == *b)
		a++, b++;
	return (*a != *b);
}

static inline int
memcmp (a, b, n)
	const char *a;
	const char *b;
	int n;
{
	while (*a == *b && --n)
		a++, b++;
	return (*a != *b);
}

static inline void
memset(p, v, len)
	void *p;
	long v;
	long len;
{
	while (len--)
		*(char *)p++ = v;
}

/* for debug */
#ifdef DEBUG
extern int printf __P((const char *, ...));
#endif

#define IODEVbase ((volatile struct IODEVICE *)PHYS_IODEV)

union {
	struct fs superblk;		/* ホンモノ */
	unsigned char buf[SBSIZE];	/* サイズ合わせ */
} superblk_buf;

#define sblock superblk_buf.superblk

/***************************************************************
 *
 *	ディレクトリエントリをため込むバッファ
 *
 */

#define N_VMUNIX 24

struct {
	ino_t ino;	/* inode */
	char name[60];
} vmunix_dirent[N_VMUNIX];

char *lowram;

/***************************************************************
 *
 *	ブロックの連続読みを試みる。
 *	length = 0 のときは、単にバッファをフラッシュする
 *	だけとなる。
 *
 */

void *s_buf;
int s_pos;
size_t s_len;

int
raw_read_queue(buf, pos, len)
	void *buf;
	int pos;
	size_t len;
{
	int r = 0;
	/* さいしょは何もしない */
	if (s_len == 0) {
		s_buf = buf;
		s_pos = pos;
		s_len = len;
		return r;
	}
	/* 前のセクタと連続しているとき */
	if (len > 0 && s_pos + btodb(s_len) == pos) {
		s_len += len;
		return r;
	}
	/* これまで溜っていたものを読む */
	r = RAW_READ(s_buf, s_pos, s_len);
	s_buf = buf;
	s_pos = pos;
	s_len = len;
	return r;
}

/***************************************************************
 *
 *	スーパーブロックを読みこむ
 *
 */

void
get_superblk()
{
	RAW_READ(&superblk_buf.buf, dbtob(SBLOCK), SBSIZE);
#ifdef DEBUG
	printf("fs_magic=%08lx\n", sblock.fs_magic);
	printf("fs_ipg=%08lx\n", sblock.fs_ipg);
	printf("fs_ncg=%08lx\n", sblock.fs_ncg);
	printf("fs_bsize=%08lx\n", sblock.fs_bsize);
	printf("fs_fsize=%08lx\n", sblock.fs_fsize);
	printf("INOPB=%08lx\n", INOPB(&sblock));
#endif
}

/***************************************************************
 *
 *	inode を取ってくる
 *
 */

int
get_inode(ino, pino)
	ino_t ino;
	struct dinode *pino;
{
	struct dinode *buf = alloca(sblock.fs_bsize);
	RAW_READ(buf,
		 dbtob(fsbtodb(&sblock, ino_to_fsba(&sblock, ino))),
		 sblock.fs_bsize);
	*pino = buf[ino_to_fsbo(&sblock, ino)];
#ifdef DEBUG
	printf("%d)permission=%06ho\n", ino, pino->di_mode);
	printf("%d)nlink=%4d\n", ino, pino->di_nlink);
	printf("%d)uid=%4d\n", ino, pino->di_uid);
	printf("%d)gid=%4d\n", ino, pino->di_gid);
	printf("%d)size=%ld\n", ino, pino->di_size);
#endif
	return 0;
}

/***************************************************************
 *
 *	inode の指しているブロックを取ってくる
 *
 *	NutShell ライオン本よりのパクり
 *
 *	buf にはデータを読んでくるアドレスをセットして
 *	おいてほしいなあ。できればブロックを切りあげて
 *
 */

int
read_indirect(blkno, level, buf, count)
	ino_t blkno;
	int level;
	void **buf;
	int count;
{
	daddr_t idblk[MAXBSIZE / sizeof(daddr_t)];
	int i;

	RAW_READ(idblk,
		 dbtob(fsbtodb(&sblock, blkno)),
		 sblock.fs_bsize);

	for (i = 0; i < NINDIR(&sblock) && count > 0; i++) {
		if (level) {
			/* さらに間接ブロックを読ませる */
			count = read_indirect(idblk[i], level - 1, buf, count);
		} else {
			/* データを読む */
#ifdef DEBUG
			printf("%d%4d\n", level, idblk[i]);
#endif
			raw_read_queue(*buf,
				       dbtob(fsbtodb(&sblock, idblk[i])),
				       sblock.fs_bsize);
			*buf += sblock.fs_bsize;
			count -= sblock.fs_bsize;
		}
	}
	return count;
}

/*
 * なぜか、ANSI C 形式でないと動かない(?)
 */
void
read_blocks(struct dinode *dp, void *buf, int count)
{
	int i;

	if (dp->di_size < count)
		count = dp->di_size;

	s_len = 0;

	/* direct block を読む */
	for (i = 0; i < NDADDR && count > 0; i++) {
#ifdef DEBUG
		printf(" %4d\n", dp->di_db[i]);
#endif
		raw_read_queue(buf,
			       dbtob(fsbtodb(&sblock, dp->di_db[i])),
			       sblock.fs_bsize);
		buf += sblock.fs_bsize;
		count -= sblock.fs_bsize;
	}
  
	/* indirect block を読む */
	for (i = 0; i < NIADDR && count > 0; i++) {
		count = read_indirect(dp->di_ib[i], i, &buf, count);
	}
	/* バッファのフラッシュ */
	raw_read_queue(NULL, 0, 0);
}

/***************************************************************
 *
 *	指定されたファイルの inode を求める
 *
 */

ino_t
search_file(dirino, filename)
	ino_t dirino;		/* ディレクトリの位置 */
	const char *filename;	/* ファイル名 */
{
	void *dirp;
	struct dinode dinode;
	struct direct *dir;

	get_inode(dirino, &dinode);
	dirp = alloca((dinode.di_size + MAXBSIZE - 1) & -MAXBSIZE);
	read_blocks(&dinode, dirp, dinode.di_size);

	while (dir = dirp, dir->d_ino != 0) {
		if (!strcmp(dir->d_name, filename))
			return dir->d_ino;
		dirp += dir->d_reclen;
	}
	return 0;
}

/***************************************************************
 *
 *	指定されたファイルを読み込んでくる
 *
 */

unsigned
load(buf, dirino, filename)
	void *buf;		/* 読み込み先 */
	ino_t dirino;		/* 該当ディレクトリの inode 番号 */
	const char *filename;	/* ファイル名 */
{
	struct dinode dinode;

	get_inode(search_file(dirino, filename), &dinode);
	read_blocks(&dinode, buf, dinode.di_size);
	return dinode.di_size;
}

/***************************************************************
 *
 *	for test
 *
 *	今のところ、vmunix はシンボリックリンクは許されて
 *	いないが、将来的にはどうにかなるかもしれん
 *
 */

void
debug_print(s)
	unsigned char *s;
{
	unsigned c;
	while ((c = *s++)) {
		if (c >= 0x80)
			c = JISSFT((0x0100 * c + *s++) & 0x7F7F);
		B_PUTC(c);
	}
}

void
debug_print_hex(x, l)
	unsigned x;	/* 表示する数字 */
	int l;		/* 表示する桁数 */
{
	if (l >= 0) {
		debug_print_hex(x >> 4, l - 1);
		x &= 0x0F;
		if (x > 9)
			x += 7;
		B_PUTC('0' + x);
	}
}

/***************************************************************
 *
 *	vmunix.* netbsd.* のリストをつくり出す
 *
 */

void
pickup_list(dirino)
	ino_t dirino;
{
	void *dirp;
	struct dinode dinode;
	struct direct *dir;
	int n = 0;

	get_inode(dirino, &dinode);
	dirp = alloca((dinode.di_size + MAXBSIZE - 1) & -MAXBSIZE);
	read_blocks(&dinode, dirp, dinode.di_size);

	while (dir = dirp, dir->d_ino != 0) {
		if (!memcmp(dir->d_name, "vmunix", 6)
		    || !memcmp(dir->d_name, "netbsd", 6)) {
			vmunix_dirent[n].ino = dir->d_ino;
			memcpy(vmunix_dirent[n].name, dir->d_name, 60);
			if (++n >= N_VMUNIX)
				return;
		}
		dirp += dir->d_reclen;
	}
	while (n < N_VMUNIX)
		vmunix_dirent[n++].ino = 0;
}

/***************************************************************
 *
 *	vmunix_dirent[] のレコードを表示する
 *
 */

void
print_list(n, active, bootflags)
	int n;
	int active;
	unsigned bootflags;
{
	if (!vmunix_dirent[n].ino)
		return;
	B_LOCATE(0, 4 + n);
	B_PRINT(active && (bootflags & RB_SINGLE)
		? "SINGLE "
		: "       ");
	debug_print_hex(vmunix_dirent[n].ino, 8);
	B_PRINT("  ");
	if (active)
		B_COLOR(0x0F);
	B_PRINT(vmunix_dirent[n].name);
	B_COLOR(0x03);
}

/***************************************************************
 *
 *	まじかよ？
 *
 *	すべての IPL モジュールを読み込んできたあとは、
 *	ここに飛んでくる。「コアの転送」が終了するまでは、
 *	x68k IOCS は生きています。割り込みをマスクするまでは
 *	大丈夫でしょう。
 *
 */

volatile void
bootufs()
{
	unsigned long bootflags;
	unsigned long esym;
	int i;
	char *addr;
	/* これは 0x100000 から存在しているもの */
	extern struct exec header;
	extern char image[];
#if 0
	/* for debug; 起動時のレジスタが入っている */
	extern long tmpbuf[16];
#endif

#if 0	/* sdboot */
	extern long ID;
#else
	extern long FDMODE;
	extern unsigned char FDSECMINMAX[8];	/* or int [2] */
#endif

	unsigned short SFT;

	/* これはコピー品 */
	struct exec localheader;

	/* MPU チェック */
#if 0
	sys_stat = SYS_STAT(0);
	if ((sys_stat & 255) == 0 ||
	    (getcpu() != 4 && !(sys_stat & (1 << 14)))) {
		debug_print("MMU がないため、NetBSD を起動できません。\r\n");
		debug_print("リセットしてください。\r\n");
		for(;;);
	}
#endif

	debug_print("\r\n");

	/* bootflags をセット */
	bootflags = RB_AUTOBOOT;

	/* シフトキーが押されていたら */
	SFT = B_SFTSNS();
	if (SFT & 0x01)
		bootflags |= RB_SINGLE;

#if 0
	/* for debug; レジスタの状態をプリントする */
	for (i = 0; i < 16; i++) {
		debug_print_hex(tmpbuf[i], 8);
		debug_print("\r\n");
	}
#endif

	/* vmunix を読み込んでくる */
	get_superblk();
	if (sblock.fs_magic != FS_MAGIC) {
		debug_print("fdboot: bogus super block;"
			    " ルートファイルシステムが壊れています！\r\n");
		debug_print("リセットしてください。\r\n");
		for (;;);
	}

	if (bootflags == RB_AUTOBOOT) {
		debug_print("loading netbsd...");
		esym = load(&header, ROOTINO, "netbsd");

		SFT = B_SFTSNS();
		if (SFT & 0x0001)
			bootflags |= RB_SINGLE;
	}

	if (bootflags != RB_AUTOBOOT) {
		int x = 0;
		struct dinode dinode;

		B_CLR_ST(2);
		debug_print("....どのカーネルを読み込みますか？\r\n"
			    "SHIFT で RB_SINGLE がトグルします。\r\n");

		/* ルートディレクトリを解析する */
		pickup_list(ROOTINO);

		/* 表示する */
		for (i = 0; i < N_VMUNIX; i++) {
			if (!memcmp(vmunix_dirent[i].name, "netbsd", 7)) {
				print_list(x, 0, bootflags);
				x = i;
			}
			print_list(i, i == x, bootflags);
		}

		while (B_SFTSNS() & 0x0001);

		/* 入力待ちループする */
		for (;;) {
			switch ((B_KEYINP() >> 8) & 0xFF) {
			case 0x3C:	/* UP */
				if (x > 0) {
					print_list(--x, 1, bootflags);
					print_list(x + 1, 0, bootflags);
				}
				break;
			case 0x3E:	/* DOWN */
				if (x < N_VMUNIX - 1
				    && vmunix_dirent[x + 1].ino) {
					print_list(++x, 1, bootflags);
					print_list(x - 1, 0, bootflags);
				}
				break;
			case 0x70:	/* SHIFT */
				bootflags ^= RB_SINGLE;
				print_list(x, 1, bootflags);
				break;
			case 0x1D:	/* CR */
			case 0x4E:	/* ENTER */
				goto exit_loop;
			}
		}
	exit_loop:
		B_CLR_ST(2);
		B_PRINT("loading ");
		B_PRINT(vmunix_dirent[x].name);
		B_PRINT("...");
		get_inode(vmunix_dirent[x].ino, &dinode);
		read_blocks(&dinode, &header, dinode.di_size);
		esym = dinode.di_size;
	}
	debug_print("done.\r\n");

	/* 「ひらがな」で、ddb を起こす */
	SFT = B_SFTSNS();
	if (SFT & 0x2000)
		bootflags |= RB_KDB;

	debug_print("\r\n\r\nInsert file system floppy\r\n");
	for (;;) {
		switch ((B_KEYINP() >> 8) & 0xFF) {
 		case 0x1D:
		case 0x4E:
			goto system_floppy_ready;
		}
	}
 system_floppy_ready:
	/* esym == 32 + tsize + dsize + ssize + strsize */

	/* ヘッダをコピーする */
	localheader = header;
#define x localheader

	esym -= sizeof(header) + localheader.a_text + localheader.a_data;
	/* esym == ssize + strsize */

	/* 本体をコアへ load する */
	if (N_GETMID(localheader) == MID_M68K
	    && N_GETMAGIC(localheader) == NMAGIC) {

		/*
		 * ここから先は割り込み禁止である。
		 * 気をつけてくれ。
		 */
		asm volatile ("oriw #0x0700,sr");

		addr = lowram;
		memcpy(addr, image, x.a_text);
		addr += (x.a_text + __LDPGSZ - 1) & ~(__LDPGSZ - 1);
		memcpy(addr, image + x.a_text, x.a_data);
		addr += x.a_data;
		memset(addr, 0x00, x.a_bss);
		addr += x.a_bss;
		/*ssym = addr;*/
		bcopy(&x.a_syms, addr, sizeof(x.a_syms));
		addr += sizeof(x.a_syms);
		bcopy(image + x.a_text + x.a_data, addr, x.a_syms);
		addr += x.a_syms;
		i = *(int *)(image + x.a_text + x.a_data + x.a_syms);
		bcopy(image + x.a_text + x.a_data + x.a_syms, addr, sizeof(int));
		if (i) {
			i -= sizeof(int);
			addr += sizeof(int);
			bcopy(image + x.a_text + x.a_data + x.a_syms + sizeof(int), addr, i);
			addr += i;
		}
#define	round_to_size(x) \
	(((int)(x) + sizeof(int) - 1) & ~(sizeof(int) - 1))
	esym = (unsigned long)round_to_size(addr - lowram);
#undef round_to_size
memset(esym, 0, 0x003e8000 - (int)esym /* XXX */);

		IODEVbase->io_mfp.iera = 0;
		IODEVbase->io_mfp.ierb = 0;
		IODEVbase->io_mfp.rsr  = 0;
/*		IODEVbase->io_sysport.keyctrl = 0;*/

		/* 実行 */
#if 0	/* sdboot */
		asm volatile ("movl %0,d6" : : "g" (MAKEBOOTDEV(4, 0, 0, ID & 7, 0)) : "d6");
#else
		asm volatile ("movl %0,d6" : : "g" (MAKEBOOTDEV(2, 0, 0, (FDMODE >> 8) & 3, (FDSECMINMAX[0] == 3) ? 0 : 2)) : "d6");
#endif
		asm volatile ("movl %0,d7" : : "g" (bootflags) : "d7");

		/* すまぬ */
		(*(void volatile (*)(long, long, long))localheader.a_entry)
			(0, *(long *)0xED0008, esym);
	} else {
		debug_print("fdboot: improper file format: 実行不可能です。\r\n");
		debug_print("リセットしてください。\r\n");
		for (;;);
	}
}

/* eof */
