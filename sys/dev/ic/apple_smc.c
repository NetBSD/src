/*	$NetBSD: apple_smc.c,v 1.1 2014/04/01 17:47:36 riastradh Exp $	*/

/*
 * Apple System Management Controller
 */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apple_smc.c,v 1.1 2014/04/01 17:47:36 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#if 0                           /* XXX sysctl */
#include <sys/sysctl.h>
#endif
#include <sys/systm.h>

#include <dev/ic/apple_smc.h>
#include <dev/ic/apple_smcreg.h>
#include <dev/ic/apple_smcvar.h>

static void	apple_smc_rescan1(struct apple_smc_tag *,
		    struct apple_smc_attach_args *,
		    const char *, const char *, device_t *);
static uint8_t	apple_smc_bus_read_1(struct apple_smc_tag *, bus_size_t);
static void	apple_smc_bus_write_1(struct apple_smc_tag *, bus_size_t,
		    uint8_t);
static int	apple_smc_read_data(struct apple_smc_tag *, uint8_t *);
static int	apple_smc_write(struct apple_smc_tag *, bus_size_t, uint8_t);
static int	apple_smc_write_cmd(struct apple_smc_tag *, uint8_t);
static int	apple_smc_write_data(struct apple_smc_tag *, uint8_t);
static int	apple_smc_begin(struct apple_smc_tag *, uint8_t,
		    const char *, uint8_t);
static int	apple_smc_input(struct apple_smc_tag *, uint8_t,
		    const char *, void *, uint8_t);
static int	apple_smc_output(struct apple_smc_tag *, uint8_t,
		    const char *, const void *, uint8_t);

void
apple_smc_attach(struct apple_smc_tag *smc)
{

	mutex_init(&smc->smc_lock, MUTEX_DEFAULT, IPL_NONE);

#if 0				/* XXX sysctl */
	apple_smc_sysctl_setup(smc);
#endif

        (void)apple_smc_rescan(smc, NULL, NULL);
}

int
apple_smc_rescan(struct apple_smc_tag *smc, const char *ifattr,
    const int *locators)
{
	struct apple_smc_attach_args asa;

	(void)locators;		/* ignore */

	(void)memset(&asa, 0, sizeof asa);
	asa.asa_smc = smc;
#if 0				/* XXX sysctl */
	asa.asa_sysctlnode = smc->smc_sysctlnode;
#endif

	apple_smc_rescan1(smc, &asa, ifattr, "applesmcfan", &smc->smc_fan);
	apple_smc_rescan1(smc, &asa, ifattr, "applesmctemp", &smc->smc_temp);
#if 0				/* XXX accelerometer */
	apple_smc_rescan1(smc, &asa, ifattr, "applesmcaccel",
	    &smc->smc_accelfan);
#endif

        return 0;
}

static void
apple_smc_rescan1(struct apple_smc_tag *smc, struct apple_smc_attach_args *asa,
    const char *ifattr, const char *ifa_conf, device_t *devp)
{

	if (ifattr_match(ifattr, ifa_conf) && *devp == NULL)
		*devp = config_found_ia(smc->smc_dev, ifa_conf, asa, NULL);
}

int
apple_smc_detach(struct apple_smc_tag *smc, int flags)
{
	int error;

	error = config_detach_children(smc->smc_dev, flags);
	if (error)
		return error;

#if 0				/* XXX sysctl */
	sysctl_teardown(&smc->smc_log);
#endif

	return 0;
}

static uint8_t
apple_smc_bus_read_1(struct apple_smc_tag *smc, bus_size_t reg)
{

	return bus_space_read_1(smc->smc_bst, smc->smc_bsh, reg);
}

static void
apple_smc_bus_write_1(struct apple_smc_tag *smc, bus_size_t reg, uint8_t v)
{

	bus_space_write_1(smc->smc_bst, smc->smc_bsh, reg, v);
}

/*
 * XXX These delays are pretty randomly chosen.  Wait in 100 us
 * increments, up to a total of 1 ms.
 */

static int
apple_smc_read_data(struct apple_smc_tag *smc, uint8_t *byte)
{
	uint8_t status;
	unsigned int i;

	KASSERT(mutex_owned(&smc->smc_lock));

	for (i = 0; i < 100; i++) {
		status = apple_smc_bus_read_1(smc, APPLE_SMC_CSR);
		if (status & APPLE_SMC_STATUS_READ_READY) {
			*byte = apple_smc_bus_read_1(smc, APPLE_SMC_DATA);
			return 0;
		}
		DELAY(100);
	}

	return ETIMEDOUT;
}

static int
apple_smc_write(struct apple_smc_tag *smc, bus_size_t reg, uint8_t byte)
{
	uint8_t status;
	unsigned int i;

	KASSERT(mutex_owned(&smc->smc_lock));

	apple_smc_bus_write_1(smc, reg, byte);
	for (i = 0; i < 100; i++) {
		status = apple_smc_bus_read_1(smc, APPLE_SMC_CSR);
		if (status & APPLE_SMC_STATUS_WRITE_ACCEPTED)
			return 0;
		DELAY(100);
		if (!(status & APPLE_SMC_STATUS_WRITE_PENDING))
			apple_smc_bus_write_1(smc, reg, byte);
	}

	return ETIMEDOUT;
}

static int
apple_smc_write_cmd(struct apple_smc_tag *smc, uint8_t cmd)
{

	return apple_smc_write(smc, APPLE_SMC_CSR, cmd);
}

static int
apple_smc_write_data(struct apple_smc_tag *smc, uint8_t data)
{

	return apple_smc_write(smc, APPLE_SMC_DATA, data);
}

struct apple_smc_key {
	char ask_name[4 + 1];
	struct apple_smc_desc ask_desc;
#ifdef DIAGNOSTIC
	struct apple_smc_tag *ask_smc;
#endif
};

static int
apple_smc_begin(struct apple_smc_tag *smc, uint8_t cmd, const char *key,
    uint8_t size)
{
	unsigned int i;
	int error;

	KASSERT(mutex_owned(&smc->smc_lock));

	error = apple_smc_write_cmd(smc, cmd);
	if (error)
		return error;

	for (i = 0; i < 4; i++) {
		error = apple_smc_write_data(smc, key[i]);
		if (error)
			return error;
	}

	error = apple_smc_write_data(smc, size);
	if (error)
		return error;

	return 0;
}

static int
apple_smc_input(struct apple_smc_tag *smc, uint8_t cmd, const char *key,
    void *buffer, uint8_t size)
{
	uint8_t *bytes = buffer;
	uint8_t i;
	int error;

	mutex_enter(&smc->smc_lock);
	error = apple_smc_begin(smc, cmd, key, size);
	if (error)
		goto out;

	for (i = 0; i < size; i++) {
		error = apple_smc_read_data(smc, &bytes[i]);
		if (error)
			goto out;
	}

	/* Success!  */
	error = 0;

out:
	mutex_exit(&smc->smc_lock);
	return error;
}

static int
apple_smc_output(struct apple_smc_tag *smc, uint8_t cmd, const char *key,
    const void *buffer, uint8_t size)
{
	const uint8_t *bytes = buffer;
	uint8_t i;
	int error;

	mutex_enter(&smc->smc_lock);
	error = apple_smc_begin(smc, cmd, key, size);
	if (error)
		goto out;

	for (i = 0; i < size; i++) {
		error = apple_smc_write_data(smc, bytes[i]);
		if (error)
			goto out;
	}

out:
	mutex_exit(&smc->smc_lock);
	return error;
}

const char *
apple_smc_key_name(const struct apple_smc_key *key)
{

	return key->ask_name;
}

const struct apple_smc_desc *
apple_smc_key_desc(const struct apple_smc_key *key)
{

	return &key->ask_desc;
}

uint32_t
apple_smc_nkeys(struct apple_smc_tag *smc)
{

	return smc->smc_nkeys;
}

int
apple_smc_nth_key(struct apple_smc_tag *smc, uint32_t index,
    const char type[4 + 1], struct apple_smc_key **keyp)
{
	union { uint32_t u32; char name[4]; } index_be;
	struct apple_smc_key *key;
	int error;

	if ((type != NULL) && (strlen(type) != 4))
		return EINVAL;

	key = kmem_alloc(sizeof(*key), KM_SLEEP);
#ifdef DIAGNOSTIC
	key->ask_smc = smc;
#endif

	index_be.u32 = htobe32(index);
	error = apple_smc_input(smc, APPLE_SMC_CMD_NTH_KEY, index_be.name,
	    key->ask_name, 4);
	if (error)
		goto fail;
	key->ask_name[4] = '\0';

	CTASSERT(sizeof(key->ask_desc) == 6);
	error = apple_smc_input(smc, APPLE_SMC_CMD_KEY_DESC, key->ask_name,
	    &key->ask_desc, 6);
	if (error)
		goto fail;

	if ((type != NULL) && (0 != memcmp(key->ask_desc.asd_type, type, 4))) {
		error = EINVAL;
		goto fail;
	}

	/* Success!  */
	*keyp = key;
	return 0;

fail:
	kmem_free(key, sizeof(*key));
	return error;
}

int
apple_smc_named_key(struct apple_smc_tag *smc, const char name[4 + 1],
    const char type[4 + 1], struct apple_smc_key **keyp)
{
	struct apple_smc_key *key;
	int error;

	KASSERT(name != NULL);
	if (strlen(name) != 4)
		return EINVAL;

	if ((type != NULL) && (strlen(type) != 4))
		return EINVAL;

	key = kmem_alloc(sizeof(*key), KM_SLEEP);
#ifdef DIAGNOSTIC
	key->ask_smc = smc;
#endif
	(void)memcpy(key->ask_name, name, 4);
	key->ask_name[4] = '\0';

	CTASSERT(sizeof(key->ask_desc) == 6);
	error = apple_smc_input(smc, APPLE_SMC_CMD_KEY_DESC, key->ask_name,
	    &key->ask_desc, 6);
	if (error)
		goto fail;

	if ((type != NULL) && (0 != memcmp(key->ask_desc.asd_type, type, 4))) {
		error = EINVAL;
		goto fail;
	}

	*keyp = key;
	return 0;

fail:
	kmem_free(key, sizeof(*key));
	return error;
}

void
apple_smc_release_key(struct apple_smc_tag *smc, struct apple_smc_key *key)
{

#ifdef DIAGNOSTIC
	if (key->ask_smc != smc)
		aprint_error_dev(smc->smc_dev,
		    "releasing key with wrong tag: %p != %p",
		    smc, key->ask_smc);
#endif
	kmem_free(key, sizeof(*key));
}

int
apple_smc_key_search(struct apple_smc_tag *smc, const char *name,
    uint32_t *result)
{
	struct apple_smc_key *key;
	uint32_t start = 0, end = apple_smc_nkeys(smc), median;
	int error;

	while (start < end) {
		median = (start + ((end - start) / 2));
		error = apple_smc_nth_key(smc, median, NULL, &key);
		if (error)
			return error;

		if (memcmp(name, apple_smc_key_name(key), 4) < 0)
			end = median;
		else
			start = (median + 1);
		apple_smc_release_key(smc, key);
	}

	*result = start;
	return 0;
}

int
apple_smc_read_key(struct apple_smc_tag *smc, const struct apple_smc_key *key,
    void *buffer, uint8_t size)
{

	if (key->ask_desc.asd_size != size)
		return EINVAL;
	if (!(key->ask_desc.asd_flags & APPLE_SMC_FLAG_READ))
		return EACCES;

	return apple_smc_input(smc, APPLE_SMC_CMD_READ_KEY, key->ask_name,
	    buffer, size);
}

int
apple_smc_read_key_1(struct apple_smc_tag *smc,
    const struct apple_smc_key *key, uint8_t *p)
{

	return apple_smc_read_key(smc, key, p, 1);
}

int
apple_smc_read_key_2(struct apple_smc_tag *smc,
    const struct apple_smc_key *key, uint16_t *p)
{
	uint16_t be;
	int error;

	error = apple_smc_read_key(smc, key, &be, 2);
	if (error)
		return error;

	*p = be16toh(be);
	return 0;
}

int
apple_smc_read_key_4(struct apple_smc_tag *smc,
    const struct apple_smc_key *key, uint32_t *p)
{
	uint32_t be;
	int error;

	error = apple_smc_read_key(smc, key, &be, 4);
	if (error)
		return error;

	*p = be32toh(be);
	return 0;
}

int
apple_smc_write_key(struct apple_smc_tag *smc, const struct apple_smc_key *key,
    const void *buffer, uint8_t size)
{

	if (key->ask_desc.asd_size != size)
		return EINVAL;
	if (!(key->ask_desc.asd_flags & APPLE_SMC_FLAG_WRITE))
		return EACCES;

	return apple_smc_output(smc, APPLE_SMC_CMD_WRITE_KEY, key->ask_name,
	    buffer, size);
}

int
apple_smc_write_key_1(struct apple_smc_tag *smc,
    const struct apple_smc_key *key, uint8_t v)
{

	return apple_smc_write_key(smc, key, &v, 1);
}

int
apple_smc_write_key_2(struct apple_smc_tag *smc,
    const struct apple_smc_key *key, uint16_t v)
{
	const uint16_t v_be = htobe16(v);

	return apple_smc_write_key(smc, key, &v_be, 2);
}

int
apple_smc_write_key_4(struct apple_smc_tag *smc,
    const struct apple_smc_key *key, uint32_t v)
{
	const uint16_t v_be = htobe32(v);

	return apple_smc_write_key(smc, key, &v_be, 4);
}
