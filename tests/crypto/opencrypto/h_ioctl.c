/*	$NetBSD: h_ioctl.c,v 1.4 2022/05/21 13:31:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2017 Internet Initiative Japan Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <crypto/cryptodev.h>

/* copy from h_aescbc.c */
#define AES_KEY_LEN 16
unsigned char aes_key[AES_KEY_LEN] =
{ 0x06, 0xa9, 0x21, 0x40, 0x36, 0xb8, 0xa1, 0x5b,
  0x51, 0x2e, 0x03, 0xd5, 0x34, 0x12, 0x00, 0x06, };

#define AES_IV_LEN 16
unsigned char aes_iv[AES_IV_LEN] =
{ 0x3d, 0xaf, 0xba, 0x42, 0x9d, 0x9e, 0xb4, 0x30,
  0xb4, 0x22, 0xda, 0x80, 0x2c, 0x9f, 0xac, 0x41, };

#define AES_PLAINTX_LEN 64
unsigned char aes_plaintx[AES_PLAINTX_LEN] = "Single block msg";

#define AES_CIPHER_LEN 64
unsigned char aes_cipher[AES_CIPHER_LEN] =
{ 0xe3, 0x53, 0x77, 0x9c, 0x10, 0x79, 0xae, 0xb8,
  0x27, 0x08, 0x94, 0x2d, 0xbe, 0x77, 0x18, 0x1a, };

#define COUNT 2

/*
 * CRIOGET is deprecated.
 */

/*
 * CIOCNGSESSION
 * Hmm, who uses? (1)
 */
static int
test_ngsession(int fd)
{
	int ret;
	struct crypt_sgop sg;
	struct session_n_op css[COUNT];

	for (size_t i = 0; i < COUNT; i++) {
		struct session_n_op *cs = &css[i];

		memset(cs, 0, sizeof(*cs));
		cs->cipher = CRYPTO_AES_CBC;
		cs->keylen = AES_KEY_LEN;
		cs->key = __UNCONST(&aes_key);
	}
	memset(&sg, 0, sizeof(sg));
	sg.count = COUNT;
	sg.sessions = css;

	ret = ioctl(fd, CIOCNGSESSION, &sg);
	if (ret < 0)
		warn("failed: CIOCNGSESSION");

	return ret;
}

/*
 * CIOCNFSESSION
 * Hmm, who uses? (2)
 */
static int
test_nfsession(int fd)
{
	int ret;
	struct crypt_sfop sf;
	u_int32_t sids[COUNT];

	memset(sids, 0, sizeof(sids));
	memset(&sf, 0, sizeof(sf));
	sf.count = COUNT;
	sf.sesid = sids;

	ret = ioctl(fd, CIOCNFSESSION, &sf);
	if (ret < 0)
		warn("failed: CIOCNFSESSION");

	return ret;
}

/*
 * CIOCNCRYPTM
 * Hmm, who uses? (3)
 */
static int
test_ncryptm(int fd)
{
	int ret;
	struct crypt_mop mop;
	struct crypt_n_op css[COUNT];

	for (size_t i = 0; i < COUNT; i++) {
		struct crypt_n_op *cs;
		cs = &css[i];

		memset(cs, 0, sizeof(*cs));
		cs->ses = 0; /* session id */
		cs->op = COP_ENCRYPT;
		/* XXX */
	}

	memset(&mop, 0, sizeof(mop));
	mop.count = COUNT;
	mop.reqs = css;

	ret = ioctl(fd, CIOCNCRYPTM, &mop);
	if (ret < 0)
		warn("failed: CIOCNCRYPTM");

	return ret;
}

/*
 * CIOCNCRYPTRETM
 * Hmm, who uses? (4)
 */
static int
test_ncryptretm(int fd)
{
	int ret;
	struct session_op cs;

	struct crypt_mop mop;
	struct crypt_n_op cnos[COUNT];
	unsigned char cno_dst[COUNT][AES_CIPHER_LEN];
	struct cryptret cret;
	struct crypt_result crs[COUNT];

	memset(&cs, 0, sizeof(cs));
	cs.cipher = CRYPTO_AES_CBC;
	cs.keylen = AES_KEY_LEN;
	cs.key = __UNCONST(&aes_key);
	ret = ioctl(fd, CIOCGSESSION, &cs);
	if (ret < 0) {
		warn("failed: CIOCGSESSION");
		return ret;
	}

	for (size_t i = 0; i < COUNT; i++) {
		struct crypt_n_op *cno = &cnos[i];

		memset(cno, 0, sizeof(*cno));
		cno->ses = cs.ses;
		cno->op = COP_ENCRYPT;
		cno->len = AES_PLAINTX_LEN;
		cno->src = aes_plaintx;
		cno->dst_len = AES_CIPHER_LEN;
		cno->dst = cno_dst[i];
	}

	memset(&mop, 0, sizeof(mop));
	mop.count = COUNT;
	mop.reqs = cnos;
	ret = ioctl(fd, CIOCNCRYPTM, &mop);
	if (ret < 0)
		warn("failed: CIOCNCRYPTM");

	for (size_t i = 0; i < COUNT; i++) {
		struct crypt_result *cr = &crs[i];

		memset(cr, 0, sizeof(*cr));
		cr->reqid = cnos[i].reqid;
	}

	memset(&cret, 0, sizeof(cret));
	cret.count = COUNT;
	cret.results = crs;
	ret = ioctl(fd, CIOCNCRYPTRETM, &cret);
	if (ret < 0)
		warn("failed: CIOCNCRYPTRETM");

	return ret;
}

/*
 * CIOCNCRYPTRET
 * Hmm, who uses? (5)
 */
/* test when it does not request yet. */
static int
test_ncryptret_noent(int fd)
{
	int ret;
	struct crypt_result cr;

	memset(&cr, 0, sizeof(cr));

	ret = ioctl(fd, CIOCNCRYPTRET, &cr);
	if (ret == 0) {
		warn("failed: CIOCNCRYPTRET unexpected success when no entry");
		ret = -1;
	} else if (errno == EINPROGRESS) {
		/* expected fail */
		ret = 0;
	}

	return ret;
}

static int
test_ncryptret_ent(int fd)
{
	int ret;
	struct session_op cs;

	struct crypt_mop mop;
	struct crypt_n_op cno;
	unsigned char cno_dst[AES_CIPHER_LEN];

	struct crypt_result cr;

	memset(&cs, 0, sizeof(cs));
	cs.cipher = CRYPTO_AES_CBC;
	cs.keylen = AES_KEY_LEN;
	cs.key = __UNCONST(&aes_key);
	ret = ioctl(fd, CIOCGSESSION, &cs);
	if (ret < 0) {
		warn("failed: CIOCGSESSION");
		return ret;
	}

	memset(&cno, 0, sizeof(cno));
	cno.ses = cs.ses;
	cno.op = COP_ENCRYPT;
	cno.len = AES_PLAINTX_LEN;
	cno.src = aes_plaintx;
	cno.dst_len = AES_CIPHER_LEN;
	cno.dst = cno_dst;

	memset(&mop, 0, sizeof(mop));
	mop.count = 1;
	mop.reqs = &cno;
	ret = ioctl(fd, CIOCNCRYPTM, &mop);
	if (ret < 0)
		warn("failed: CIOCNCRYPTM");

	memset(&cr, 0, sizeof(cr));
	cr.reqid = cno.reqid;

	ret = ioctl(fd, CIOCNCRYPTRET, &cr);
	if (ret < 0)
		warn("failed: CIOCNCRYPTRET");

	return ret;
}

static int
test_ncryptret(int fd)
{
	int ret;

	ret = test_ncryptret_noent(fd);
	if (ret < 0)
		return ret;

	ret = test_ncryptret_ent(fd);
	if (ret < 0)
		return ret;

	return ret;
}

/*
 * CIOCASYMFEAT
 */
static int
set_userasymcrypto(int new, int *old)
{
	int ret;

	ret = sysctlbyname("kern.userasymcrypto", NULL, NULL, &new, sizeof(new));
	if (ret < 0) {
		warn("failed: kern.userasymcrypto=%d", new);
		return ret;
	}

	if (old != NULL)
		*old = new;

	return ret;
}

static int
test_asymfeat_each(int fd, u_int32_t *asymfeat, int userasym)
{
	int ret;

	ret = ioctl(fd, CIOCASYMFEAT, asymfeat);
	if (ret < 0)
		warn("failed: CIOCASYMFEAT when userasym=%d", userasym);

	return ret;
}

static int
test_asymfeat(int fd)
{
	int ret, new, orig;
	u_int32_t asymfeat = 0;

	/* test for kern.userasymcrypto=1 */
	new = 1;
	ret = set_userasymcrypto(new, &orig);
	if (ret < 0)
		return ret;
	ret = test_asymfeat_each(fd, &asymfeat, new);
	if (ret < 0)
		return ret;

	/* test for kern.userasymcrypto=0 */
	new = 0;
	ret = set_userasymcrypto(new, NULL);
	if (ret < 0)
		return ret;
	ret = test_asymfeat_each(fd, &asymfeat, new);
	if (ret < 0)
		return ret;

	/* cleanup */
	ret = set_userasymcrypto(orig, NULL);
	if (ret < 0)
		warnx("failed: cleanup kern.userasymcrypto");

	return ret;
}

int
main(void)
{
	int fd, ret;

	fd = open("/dev/crypto", O_RDWR, 0);
	if (fd < 0)
		err(1, "open");

	ret = test_ngsession(fd);
	if (ret < 0)
		err(1, "test_ngsession");

	ret = test_nfsession(fd);
	if (ret < 0)
		err(1, "test_ngsession");

	ret = test_ncryptm(fd);
	if (ret < 0)
		err(1, "test_ncryptm");

	test_ncryptretm(fd);
	if (ret < 0)
		err(1, "test_ncryptretm");

	ret = test_ncryptret(fd);
	if (ret < 0)
		err(1, "test_ncryptret");

	ret = test_asymfeat(fd);
	if (ret < 0)
		err(1, "test_asymfeat");

	return 0;
}
