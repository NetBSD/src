/*	$NetBSD: bootufs.c,v 1.2 2001/06/12 16:57:28 minoura Exp $	*/

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
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/dir.h>
#include <a.out.h>
#include <machine/bootinfo.h>
#ifdef SCSI_ADHOC_BOOTPART
#include <machine/disklabel.h>
#endif

#include "xxboot.h"
#include "../../x68k/iodevice.h"
#include "../common/execkern.h"
#ifdef GZIP
#include "gunzip/gzip.h"
#endif

/* assertion of sector size == 512 */
#if !(defined(DEV_BSHIFT) && defined(DEV_BSIZE) && defined(dbtob)	\
	&& DEV_BSHIFT == 9 && DEV_BSIZE == 512 && dbtob(1) == 512)
 #error You must make changes to pass sector size to RAW_READ or 64bit-ise it.
#endif

#define alloca __builtin_alloca
#define memcpy __builtin_memcpy

static inline void memset __P((void *p, int v, size_t len));
#ifdef SCSI_ADHOC_BOOTPART
static int get_scsi_part __P((void));
#endif
static int get_scsi_host_adapter __P((void));

static inline int
strcmp(a, b)
	const char *a;
	const char *b;
{

	while (*a && *a == *b)
		a++, b++;
	return (*a != *b);
}

static inline int
memcmp(a, b, n)
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
	int v;
	size_t len;
{

	while (len--)
		*(char *)p++ = v;
}

/* for debug */
#ifdef DEBUG_WITH_STDIO
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
u_int32_t s_blkpos;
size_t s_len;

int
raw_read_queue(buf, blkpos, len)
	void *buf;
	u_int32_t blkpos;
	size_t len;
{
	int r = 0;

	/* さいしょは何もしない */
	if (s_len == 0) {
		s_buf = buf;
		s_blkpos = blkpos;
		s_len = len;
		return r;
	}
	/* 前のセクタと連続しているとき */
	if (len > 0 && s_blkpos + btodb(s_len) == blkpos) {
		s_len += len;
		return r;
	}
	/* これまで溜っていたものを読む */
	r = RAW_READ(s_buf, s_blkpos, s_len);
	s_buf = buf;
	s_blkpos = blkpos;
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

	RAW_READ(&superblk_buf.buf, SBLOCK, SBSIZE);
#ifdef DEBUG_WITH_STDIO
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
	struct dinode *buf = alloca((size_t) sblock.fs_bsize);

	RAW_READ(buf,
		 fsbtodb(&sblock, (u_int32_t) ino_to_fsba(&sblock, ino)),
		 (size_t) sblock.fs_bsize);
	*pino = buf[ino_to_fsbo(&sblock, ino)];
#ifdef DEBUG_WITH_STDIO
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
	ufs_daddr_t blkno;
	int level;
	void **buf;
	int count;
{
	ufs_daddr_t idblk[MAXBSIZE / sizeof(ufs_daddr_t)];
	int i;

	RAW_READ(idblk,
		 fsbtodb(&sblock, (u_int32_t) blkno),
		 (size_t) sblock.fs_bsize);

	for (i = 0; i < NINDIR(&sblock) && count > 0; i++) {
		if (level) {
			/* さらに間接ブロックを読ませる */
			count = read_indirect(idblk[i], level - 1, buf, count);
		} else {
			/* データを読む */
#ifdef DEBUG_WITH_STDIO
			printf("%d%4d\n", level, idblk[i]);
#endif
			raw_read_queue(*buf,
				       fsbtodb(&sblock, (u_int32_t) idblk[i]),
				       (size_t) sblock.fs_bsize);
			*buf += sblock.fs_bsize;
			count -= sblock.fs_bsize;
		}
	}
	return count;
}

void
read_blocks(dp, buf, count)
	struct dinode *dp;
	void *buf;
	int count;
{
	int i;

	if (dp->di_size < (unsigned) count)
		count = dp->di_size;

	s_len = 0;

	/* direct block を読む */
	for (i = 0; i < NDADDR && count > 0; i++) {
#ifdef DEBUG_WITH_STDIO
		printf(" %4d\n", dp->di_db[i]);
#endif
		raw_read_queue(buf,
			       fsbtodb(&sblock, (u_int32_t) dp->di_db[i]),
			       (size_t) sblock.fs_bsize);
		buf += sblock.fs_bsize;
		count -= sblock.fs_bsize;
	}
  
	/* indirect block を読む */
	for (i = 0; i < NIADDR && count > 0; i++) {
		count = read_indirect(dp->di_ib[i], i, &buf, count);
	}
	/* バッファのフラッシュ */
	raw_read_queue(NULL, (u_int32_t) 0, (size_t) 0);
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
	dirp = alloca((size_t) (dinode.di_size + MAXBSIZE - 1) & ~((unsigned) MAXBSIZE - 1));
	read_blocks(&dinode, dirp, (int) dinode.di_size);

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
load_ino(buf, ino, filename)
	void *buf;
	ino_t ino;
	const char *filename;
{
	struct dinode dinode;

	B_PRINT("loading ");
	B_PRINT(filename);
	B_PRINT("...");
	get_inode(ino, &dinode);
	read_blocks(&dinode, buf, (int) dinode.di_size);
	return dinode.di_size;
}

unsigned
load_name(buf, dirino, filename)
	void *buf;		/* 読み込み先 */
	ino_t dirino;		/* 該当ディレクトリの inode 番号 */
	const char *filename;	/* ファイル名 */
{
	ino_t ino;

	ino = search_file(dirino, filename);
	return ino ? load_ino(buf, ino, filename) : 0;
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
print_hex(x, l)
	unsigned x;	/* 表示する数字 */
	int l;		/* 表示する桁数 */
{

	if (l > 0) {
		print_hex(x >> 4, l - 1);
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
	dirp = alloca((size_t) (dinode.di_size + MAXBSIZE - 1) & ~((unsigned) MAXBSIZE - 1));
	read_blocks(&dinode, dirp, (int) dinode.di_size);

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
print_list(n, active, boothowto)
	int n;
	int active;
	unsigned boothowto;
{

	if (!vmunix_dirent[n].ino)
		return;
	B_LOCATE(0, 7 + n);
	B_PRINT(active && (boothowto & RB_SINGLE)
		? "SINGLE "
		: "       ");
	print_hex(vmunix_dirent[n].ino, 8);
	B_PRINT("  ");
	if (active)
		B_COLOR(0x0F);
	B_PRINT(vmunix_dirent[n].name);
	B_COLOR(0x03);
}

#ifdef SCSI_ADHOC_BOOTPART
/*
 * get partition # from partition start position
 */

#define NPART		15
#define PARTTBL_TOP	((unsigned)4)	/* pos of part inf in 512byte-blocks */
#define MAXPART		6
const unsigned char partition_conv[MAXPART + 1] = { 0, 1, 3, 4, 5, 6, 7 };

static int
get_scsi_part()
{
	struct {
		u_int32_t	magic;		/* 0x5836384B ("X68K") */
		u_int32_t	parttotal;
		u_int32_t	diskblocks;
		u_int32_t	diskblocks2;	/* backup? */
		struct dos_partition parttbl[NPART];
		unsigned char	formatstr[256];
		unsigned char	rest[512];
	} partbuf;
	int i;
	u_int32_t part_top;

#ifdef BOOT_DEBUG
	B_PRINT("seclen: ");
	print_hex(SCSI_BLKLEN, 8);	/* 0: 256, 1: 512, 2: 1024 */
	B_PRINT(", topsec: ");
	print_hex(SCSI_PARTTOP, 8);	/* partition top in sector */
#endif
	/*
	 * read partition table
	 */
	RAW_READ0(&partbuf, PARTTBL_TOP, sizeof partbuf);

	part_top = SCSI_PARTTOP >> (2 - SCSI_BLKLEN);
	for (i = 0; i < MAXPART; i++)
		if ((u_int32_t) partbuf.parttbl[i].dp_start == part_top)
			goto found;

	BOOT_ERROR("Can't boot from this partition");
	/* NOTREACHED */
found:
#ifdef BOOT_DEBUG
	B_PRINT("; sd");
	B_PUTC((unsigned)ID + '0');	/* SCSI ID (not NetBSD unit #) */
	B_PUTC((unsigned)partition_conv[i] + 'a');
#endif
	return partition_conv[i];
}
#endif	/* SCSI_ADHOC_BOOTPART */

/*
 * Check the type of SCSI interface
 */
static int
get_scsi_host_adapter()
{
	char *bootrom;
	int ha;

#ifdef BOOT_DEBUG
	B_PRINT(" at ");
#endif
	bootrom = (char *) (BOOT_INFO & 0x00ffffe0);
	if (!memcmp(bootrom + 0x24, "SCSIIN", 6)) {
#ifdef BOOT_DEBUG
		B_PRINT("spc0");
#endif
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
	} else if (badbaddr(&IODEVbase->io_exspc.bdid)) {
#ifdef BOOT_DEBUG
		B_PRINT("mha0");
#endif
		ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
	} else {
#ifdef BOOT_DEBUG
		B_PRINT("spc1");
#endif
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
	}

	return ha;
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
	unsigned long boothowto;
	unsigned long esym;
	int i;

	/* これは 0x100000 から存在しているもの */
	extern struct exec header;
	extern char image[];
	/* これはコピー品 */
	struct execkern_arg execinfo;

	int bootdev;
	unsigned short SFT;
#ifdef BOOT_DEBUG
	/* for debug; 起動時のレジスタが入っている */
	extern unsigned startregs[16];
#endif

	/* MPU チェック */
#if 0
	int sys_stat;

	sys_stat = SYS_STAT(0);
	if ((sys_stat & 255) == 0 ||
	    (getcpu() != 4 && !(sys_stat & (1 << 14)))) {
		BOOT_ERROR("MMU がないため、NetBSD を起動できません。");
		/* NOTREACHED */
	}
#endif

#ifdef BOOT_DEBUG
	/* for debug; レジスタの状態をプリントする */
	for (i = 0; i < 16; i++) {
		print_hex(startregs[i], 8);
		B_PRINT((i & 7) == 7 ? "\r\n" : " ");
	}
#endif

	/*
	 * get boot device
	 */
	if (BINF_ISFD(&BOOT_INFO)) {
		/* floppy */
		bootdev = X68K_MAKEBOOTDEV(X68K_MAJOR_FD, BOOT_INFO & 3,
				(FDSECMINMAX.minsec.N == 3) ? 0 : 2);
	} else {
		/* SCSI */
		int part, ha;

#ifdef SCSI_ADHOC_BOOTPART
		part = get_scsi_part();
#else
		part = 0;			/* sd?a only */
#endif
		ha = get_scsi_host_adapter();
		bootdev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_SD, ha >> 4, ha & 15,
						ID & 7, 0, part);
	}
#ifdef BOOT_DEBUG
	B_PRINT("\r\nbootdev: ");
	print_hex((unsigned) bootdev, 8);
	B_PRINT("\r\nhit key\r\n");
	(void) B_KEYINP();	/* wait key input */
#endif

	/* boothowto をセット */
	boothowto = RB_AUTOBOOT;

	/* シフトキーが押されていたら */
	SFT = B_SFTSNS();
	if (SFT & 0x01)
		boothowto |= RB_SINGLE;

	/* vmunix を読み込んでくる */
	get_superblk();
	if (sblock.fs_magic != FS_MAGIC) {
		BOOT_ERROR("bogus super block: "
			   "ルートファイルシステムが壊れています！");
		/* NOTREACHED */
	}

	if (boothowto == RB_AUTOBOOT) {
		esym = load_name(&header, ROOTINO, "netbsd");
#ifdef GZIP
		if (!esym)
			esym = load_name(&header, ROOTINO, "netbsd.gz");
#endif

		SFT = B_SFTSNS();
		if (SFT & 0x0001)
			boothowto |= RB_SINGLE;
	}

	if (boothowto != RB_AUTOBOOT || !esym) {
		int x = 0;

		printtitle();
		B_PRINT("....どのカーネルを読み込みますか？\r\n"
			    "SHIFT で RB_SINGLE がトグルします。\r\n");

		/* ルートディレクトリを解析する */
		pickup_list(ROOTINO);

		/* 表示する */
		for (i = 0; i < N_VMUNIX; i++) {
			if (!memcmp(vmunix_dirent[i].name, "netbsd", 7)) {
				print_list(x, 0, boothowto);
				x = i;
			}
			print_list(i, i == x, boothowto);
		}

	wait_shift_release:
		while (B_SFTSNS() & 0x0001)
			;

		/* 入力待ちループする */
		for (;;) {
			switch ((B_KEYINP() >> 8) & 0xFF) {
			case 0x3C:	/* UP */
				if (x > 0) {
					print_list(--x, 1, boothowto);
					print_list(x + 1, 0, boothowto);
				}
				break;
			case 0x3E:	/* DOWN */
				if (x < N_VMUNIX - 1
				    && vmunix_dirent[x + 1].ino) {
					print_list(++x, 1, boothowto);
					print_list(x - 1, 0, boothowto);
				}
				break;
			case 0x70:	/* SHIFT */
				boothowto ^= RB_SINGLE;
				print_list(x, 1, boothowto);
				goto wait_shift_release;
			case 0x1D:	/* CR */
			case 0x4E:	/* ENTER */
				goto exit_loop;
			}
		}
	exit_loop:
		printtitle();
		esym = load_ino(&header,
				vmunix_dirent[x].ino, vmunix_dirent[x].name);
	}
#ifdef GZIP
    load_done:
#endif
	B_PRINT("done.\r\n");

#ifdef GZIP
	/*
	 * Check if the file is gzip'ed
	 */
	if (check_gzip_magic((char *) &header)) {
		/*
		 * uncompress
		 */
		char *ctop;	/* gzip'ed top */

		/*
		 * transfer loaded file to safe? area
		 *
		 *    0x3F0000  -  SIZE_ALLOC_BUF - ?
		 * (stack bottom)  (see inflate.c)
		 */
		ctop = (char *) 0x3e8000 - esym;
		memcpy(ctop, &header, esym);

		B_PRINT("decompressing...");

		/* unzip() doesn't return on failure */
		esym = unzip(ctop, (char *) &header);

		goto load_done;
	}
#endif

	/* 「ひらがな」で、ddb を起こす */
	SFT = B_SFTSNS();
	if (SFT & 0x2000)
		boothowto |= RB_KDB;

	/* ヘッダをコピーする */
	execinfo.image_top = image;
	execinfo.load_addr = 0;
	execinfo.text_size = header.a_text;
	execinfo.data_size = header.a_data;
	execinfo.bss_size = header.a_bss;
	execinfo.symbol_size = header.a_syms;
	execinfo.d5 = BOOT_INFO;		/* unused for now */
	execinfo.rootdev = bootdev;
	execinfo.boothowto = boothowto;
	execinfo.entry_addr = header.a_entry;

	/* 本体をコアへ load する */
	if (N_GETMID(header) == MID_M68K
	    && N_GETMAGIC(header) == NMAGIC) {

		/*
		 * ここから先は割り込み禁止である。
		 * 気をつけてくれ。
		 */
		asm("oriw #0x0700,%sr");

		/* 実行 */
		exec_kernel(&execinfo);
		/* NOTREACHED */
	} else {
		BOOT_ERROR("improper file format: 実行不可能です。");
		/* NOTREACHED */
	}
}

/* eof */
