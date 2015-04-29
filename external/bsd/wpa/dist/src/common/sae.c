/*
 * Simultaneous authentication of equals
 * Copyright (c) 2012-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "crypto/crypto.h"
#include "crypto/sha256.h"
#include "crypto/random.h"
#include "crypto/dh_groups.h"
#include "ieee802_11_defs.h"
#include "sae.h"


int sae_set_group(struct sae_data *sae, int group)
{
	struct sae_temporary_data *tmp;

	sae_clear_data(sae);
	tmp = sae->tmp = os_zalloc(sizeof(*tmp));
	if (tmp == NULL)
		return -1;

	/* First, check if this is an ECC group */
	tmp->ec = crypto_ec_init(group);
	if (tmp->ec) {
		sae->group = group;
		tmp->prime_len = crypto_ec_prime_len(tmp->ec);
		tmp->prime = crypto_ec_get_prime(tmp->ec);
		tmp->order = crypto_ec_get_order(tmp->ec);
		return 0;
	}

	/* Not an ECC group, check FFC */
	tmp->dh = dh_groups_get(group);
	if (tmp->dh) {
		sae->group = group;
		tmp->prime_len = tmp->dh->prime_len;
		if (tmp->prime_len > SAE_MAX_PRIME_LEN) {
			sae_clear_data(sae);
			return -1;
		}

		tmp->prime_buf = crypto_bignum_init_set(tmp->dh->prime,
							tmp->prime_len);
		if (tmp->prime_buf == NULL) {
			sae_clear_data(sae);
			return -1;
		}
		tmp->prime = tmp->prime_buf;

		tmp->order_buf = crypto_bignum_init_set(tmp->dh->order,
							tmp->dh->order_len);
		if (tmp->order_buf == NULL) {
			sae_clear_data(sae);
			return -1;
		}
		tmp->order = tmp->order_buf;

		return 0;
	}

	/* Unsupported group */
	return -1;
}


void sae_clear_temp_data(struct sae_data *sae)
{
	struct sae_temporary_data *tmp;
	if (sae == NULL || sae->tmp == NULL)
		return;
	tmp = sae->tmp;
	crypto_ec_deinit(tmp->ec);
	crypto_bignum_deinit(tmp->prime_buf, 0);
	crypto_bignum_deinit(tmp->order_buf, 0);
	crypto_bignum_deinit(tmp->sae_rand, 1);
	crypto_bignum_deinit(tmp->pwe_ffc, 1);
	crypto_bignum_deinit(tmp->own_commit_scalar, 0);
	crypto_bignum_deinit(tmp->own_commit_element_ffc, 0);
	crypto_bignum_deinit(tmp->peer_commit_element_ffc, 0);
	crypto_ec_point_deinit(tmp->pwe_ecc, 1);
	crypto_ec_point_deinit(tmp->own_commit_element_ecc, 0);
	crypto_ec_point_deinit(tmp->peer_commit_element_ecc, 0);
	wpabuf_free(tmp->anti_clogging_token);
	bin_clear_free(tmp, sizeof(*tmp));
	sae->tmp = NULL;
}


void sae_clear_data(struct sae_data *sae)
{
	if (sae == NULL)
		return;
	sae_clear_temp_data(sae);
	crypto_bignum_deinit(sae->peer_commit_scalar, 0);
	os_memset(sae, 0, sizeof(*sae));
}


static void buf_shift_right(u8 *buf, size_t len, size_t bits)
{
	size_t i;
	for (i = len - 1; i > 0; i--)
		buf[i] = (buf[i - 1] << (8 - bits)) | (buf[i] >> bits);
	buf[0] >>= bits;
}


static struct crypto_bignum * sae_get_rand(struct sae_data *sae)
{
	u8 val[SAE_MAX_PRIME_LEN];
	int iter = 0;
	struct crypto_bignum *bn = NULL;
	int order_len_bits = crypto_bignum_bits(sae->tmp->order);
	size_t order_len = (order_len_bits + 7) / 8;

	if (order_len > sizeof(val))
		return NULL;

	for (;;) {
		if (iter++ > 100)
			return NULL;
		if (random_get_bytes(val, order_len) < 0)
			return NULL;
		if (order_len_bits % 8)
			buf_shift_right(val, order_len, 8 - order_len_bits % 8);
		bn = crypto_bignum_init_set(val, order_len);
		if (bn == NULL)
			return NULL;
		if (crypto_bignum_is_zero(bn) ||
		    crypto_bignum_is_one(bn) ||
		    crypto_bignum_cmp(bn, sae->tmp->order) >= 0) {
			crypto_bignum_deinit(bn, 0);
			continue;
		}
		break;
	}

	os_memset(val, 0, order_len);
	return bn;
}


static struct crypto_bignum * sae_get_rand_and_mask(struct sae_data *sae)
{
	crypto_bignum_deinit(sae->tmp->sae_rand, 1);
	sae->tmp->sae_rand = sae_get_rand(sae);
	if (sae->tmp->sae_rand == NULL)
		return NULL;
	return sae_get_rand(sae);
}


static void sae_pwd_seed_key(const u8 *addr1, const u8 *addr2, u8 *key)
{
	wpa_printf(MSG_DEBUG, "SAE: PWE derivation - addr1=" MACSTR
		   " addr2=" MACSTR, MAC2STR(addr1), MAC2STR(addr2));
	if (os_memcmp(addr1, addr2, ETH_ALEN) > 0) {
		os_memcpy(key, addr1, ETH_ALEN);
		os_memcpy(key + ETH_ALEN, addr2, ETH_ALEN);
	} else {
		os_memcpy(key, addr2, ETH_ALEN);
		os_memcpy(key + ETH_ALEN, addr1, ETH_ALEN);
	}
}


static int sae_test_pwd_seed_ecc(struct sae_data *sae, const u8 *pwd_seed,
				 struct crypto_ec_point *pwe)
{
	u8 pwd_value[SAE_MAX_ECC_PRIME_LEN], prime[SAE_MAX_ECC_PRIME_LEN];
	struct crypto_bignum *x;
	int y_bit;
	size_t bits;

	if (crypto_bignum_to_bin(sae->tmp->prime, prime, sizeof(prime),
				 sae->tmp->prime_len) < 0)
		return -1;

	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-seed", pwd_seed, SHA256_MAC_LEN);

	/* pwd-value = KDF-z(pwd-seed, "SAE Hunting and Pecking", p) */
	bits = crypto_ec_prime_len_bits(sae->tmp->ec);
	sha256_prf_bits(pwd_seed, SHA256_MAC_LEN, "SAE Hunting and Pecking",
			prime, sae->tmp->prime_len, pwd_value, bits);
	if (bits % 8)
		buf_shift_right(pwd_value, sizeof(pwd_value), 8 - bits % 8);
	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-value",
			pwd_value, sae->tmp->prime_len);

	if (os_memcmp(pwd_value, prime, sae->tmp->prime_len) >= 0)
		return 0;

	y_bit = pwd_seed[SHA256_MAC_LEN - 1] & 0x01;

	x = crypto_bignum_init_set(pwd_value, sae->tmp->prime_len);
	if (x == NULL)
		return -1;
	if (crypto_ec_point_solve_y_coord(sae->tmp->ec, pwe, x, y_bit) < 0) {
		crypto_bignum_deinit(x, 0);
		wpa_printf(MSG_DEBUG, "SAE: No solution found");
		return 0;
	}
	crypto_bignum_deinit(x, 0);

	wpa_printf(MSG_DEBUG, "SAE: PWE found");

	return 1;
}


static int sae_test_pwd_seed_ffc(struct sae_data *sae, const u8 *pwd_seed,
				 struct crypto_bignum *pwe)
{
	u8 pwd_value[SAE_MAX_PRIME_LEN];
	size_t bits = sae->tmp->prime_len * 8;
	u8 exp[1];
	struct crypto_bignum *a, *b;
	int res;

	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-seed", pwd_seed, SHA256_MAC_LEN);

	/* pwd-value = KDF-z(pwd-seed, "SAE Hunting and Pecking", p) */
	sha256_prf_bits(pwd_seed, SHA256_MAC_LEN, "SAE Hunting and Pecking",
			sae->tmp->dh->prime, sae->tmp->prime_len, pwd_value,
			bits);
	if (bits % 8)
		buf_shift_right(pwd_value, sizeof(pwd_value), 8 - bits % 8);
	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-value", pwd_value,
			sae->tmp->prime_len);

	if (os_memcmp(pwd_value, sae->tmp->dh->prime, sae->tmp->prime_len) >= 0)
	{
		wpa_printf(MSG_DEBUG, "SAE: pwd-value >= p");
		return 0;
	}

	/* PWE = pwd-value^((p-1)/r) modulo p */

	a = crypto_bignum_init_set(pwd_value, sae->tmp->prime_len);

	if (sae->tmp->dh->safe_prime) {
		/*
		 * r = (p-1)/2 for the group used here, so this becomes:
		 * PWE = pwd-value^2 modulo p
		 */
		exp[0] = 2;
		b = crypto_bignum_init_set(exp, sizeof(exp));
	} else {
		/* Calculate exponent: (p-1)/r */
		exp[0] = 1;
		b = crypto_bignum_init_set(exp, sizeof(exp));
		if (b == NULL ||
		    crypto_bignum_sub(sae->tmp->prime, b, b) < 0 ||
		    crypto_bignum_div(b, sae->tmp->order, b) < 0) {
			crypto_bignum_deinit(b, 0);
			b = NULL;
		}
	}

	if (a == NULL || b == NULL)
		res = -1;
	else
		res = crypto_bignum_exptmod(a, b, sae->tmp->prime, pwe);

	crypto_bignum_deinit(a, 0);
	crypto_bignum_deinit(b, 0);

	if (res < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to calculate PWE");
		return -1;
	}

	/* if (PWE > 1) --> found */
	if (crypto_bignum_is_zero(pwe) || crypto_bignum_is_one(pwe)) {
		wpa_printf(MSG_DEBUG, "SAE: PWE <= 1");
		return 0;
	}

	wpa_printf(MSG_DEBUG, "SAE: PWE found");
	return 1;
}


static int sae_derive_pwe_ecc(struct sae_data *sae, const u8 *addr1,
			      const u8 *addr2, const u8 *password,
			      size_t password_len)
{
	u8 counter, k = 4;
	u8 addrs[2 * ETH_ALEN];
	const u8 *addr[2];
	size_t len[2];
	int found = 0;
	struct crypto_ec_point *pwe_tmp;

	if (sae->tmp->pwe_ecc == NULL) {
		sae->tmp->pwe_ecc = crypto_ec_point_init(sae->tmp->ec);
		if (sae->tmp->pwe_ecc == NULL)
			return -1;
	}
	pwe_tmp = crypto_ec_point_init(sae->tmp->ec);
	if (pwe_tmp == NULL)
		return -1;

	wpa_hexdump_ascii_key(MSG_DEBUG, "SAE: password",
			      password, password_len);

	/*
	 * H(salt, ikm) = HMAC-SHA256(salt, ikm)
	 * pwd-seed = H(MAX(STA-A-MAC, STA-B-MAC) || MIN(STA-A-MAC, STA-B-MAC),
	 *              password || counter)
	 */
	sae_pwd_seed_key(addr1, addr2, addrs);

	addr[0] = password;
	len[0] = password_len;
	addr[1] = &counter;
	len[1] = sizeof(counter);

	/*
	 * Continue for at least k iterations to protect against side-channel
	 * attacks that attempt to determine the number of iterations required
	 * in the loop.
	 */
	for (counter = 1; counter < k || !found; counter++) {
		u8 pwd_seed[SHA256_MAC_LEN];
		int res;

		if (counter > 200) {
			/* This should not happen in practice */
			wpa_printf(MSG_DEBUG, "SAE: Failed to derive PWE");
			break;
		}

		wpa_printf(MSG_DEBUG, "SAE: counter = %u", counter);
		if (hmac_sha256_vector(addrs, sizeof(addrs), 2, addr, len,
				       pwd_seed) < 0)
			break;
		res = sae_test_pwd_seed_ecc(sae, pwd_seed,
					    found ? pwe_tmp :
					    sae->tmp->pwe_ecc);
		if (res < 0)
			break;
		if (res == 0)
			continue;
		if (found) {
			wpa_printf(MSG_DEBUG, "SAE: Ignore this PWE (one was "
				   "already selected)");
		} else {
			wpa_printf(MSG_DEBUG, "SAE: Use this PWE");
			found = 1;
		}
	}

	crypto_ec_point_deinit(pwe_tmp, 1);

	return found ? 0 : -1;
}


static int sae_derive_pwe_ffc(struct sae_data *sae, const u8 *addr1,
			      const u8 *addr2, const u8 *password,
			      size_t password_len)
{
	u8 counter;
	u8 addrs[2 * ETH_ALEN];
	const u8 *addr[2];
	size_t len[2];
	int found = 0;

	if (sae->tmp->pwe_ffc == NULL) {
		sae->tmp->pwe_ffc = crypto_bignum_init();
		if (sae->tmp->pwe_ffc == NULL)
			return -1;
	}

	wpa_hexdump_ascii_key(MSG_DEBUG, "SAE: password",
			      password, password_len);

	/*
	 * H(salt, ikm) = HMAC-SHA256(salt, ikm)
	 * pwd-seed = H(MAX(STA-A-MAC, STA-B-MAC) || MIN(STA-A-MAC, STA-B-MAC),
	 *              password || counter)
	 */
	sae_pwd_seed_key(addr1, addr2, addrs);

	addr[0] = password;
	len[0] = password_len;
	addr[1] = &counter;
	len[1] = sizeof(counter);

	for (counter = 1; !found; counter++) {
		u8 pwd_seed[SHA256_MAC_LEN];
		int res;

		if (counter > 200) {
			/* This should not happen in practice */
			wpa_printf(MSG_DEBUG, "SAE: Failed to derive PWE");
			break;
		}

		wpa_printf(MSG_DEBUG, "SAE: counter = %u", counter);
		if (hmac_sha256_vector(addrs, sizeof(addrs), 2, addr, len,
				       pwd_seed) < 0)
			break;
		res = sae_test_pwd_seed_ffc(sae, pwd_seed, sae->tmp->pwe_ffc);
		if (res < 0)
			break;
		if (res > 0) {
			wpa_printf(MSG_DEBUG, "SAE: Use this PWE");
			found = 1;
		}
	}

	return found ? 0 : -1;
}


static int sae_derive_commit_element_ecc(struct sae_data *sae,
					 struct crypto_bignum *mask)
{
	/* COMMIT-ELEMENT = inverse(scalar-op(mask, PWE)) */
	if (!sae->tmp->own_commit_element_ecc) {
		sae->tmp->own_commit_element_ecc =
			crypto_ec_point_init(sae->tmp->ec);
		if (!sae->tmp->own_commit_element_ecc)
			return -1;
	}

	if (crypto_ec_point_mul(sae->tmp->ec, sae->tmp->pwe_ecc, mask,
				sae->tmp->own_commit_element_ecc) < 0 ||
	    crypto_ec_point_invert(sae->tmp->ec,
				   sae->tmp->own_commit_element_ecc) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not compute commit-element");
		return -1;
	}

	return 0;
}


static int sae_derive_commit_element_ffc(struct sae_data *sae,
					 struct crypto_bignum *mask)
{
	/* COMMIT-ELEMENT = inverse(scalar-op(mask, PWE)) */
	if (!sae->tmp->own_commit_element_ffc) {
		sae->tmp->own_commit_element_ffc = crypto_bignum_init();
		if (!sae->tmp->own_commit_element_ffc)
			return -1;
	}

	if (crypto_bignum_exptmod(sae->tmp->pwe_ffc, mask, sae->tmp->prime,
				  sae->tmp->own_commit_element_ffc) < 0 ||
	    crypto_bignum_inverse(sae->tmp->own_commit_element_ffc,
				  sae->tmp->prime,
				  sae->tmp->own_commit_element_ffc) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not compute commit-element");
		return -1;
	}

	return 0;
}


static int sae_derive_commit(struct sae_data *sae)
{
	struct crypto_bignum *mask;
	int ret = -1;

	mask = sae_get_rand_and_mask(sae);
	if (mask == NULL) {
		wpa_printf(MSG_DEBUG, "SAE: Could not get rand/mask");
		return -1;
	}

	/* commit-scalar = (rand + mask) modulo r */
	if (!sae->tmp->own_commit_scalar) {
		sae->tmp->own_commit_scalar = crypto_bignum_init();
		if (!sae->tmp->own_commit_scalar)
			goto fail;
	}
	crypto_bignum_add(sae->tmp->sae_rand, mask,
			  sae->tmp->own_commit_scalar);
	crypto_bignum_mod(sae->tmp->own_commit_scalar, sae->tmp->order,
			  sae->tmp->own_commit_scalar);

	if (sae->tmp->ec && sae_derive_commit_element_ecc(sae, mask) < 0)
		goto fail;
	if (sae->tmp->dh && sae_derive_commit_element_ffc(sae, mask) < 0)
		goto fail;

	ret = 0;
fail:
	crypto_bignum_deinit(mask, 1);
	return ret;
}


int sae_prepare_commit(const u8 *addr1, const u8 *addr2,
		       const u8 *password, size_t password_len,
		       struct sae_data *sae)
{
	if (sae->tmp == NULL)
		return -1;
	if (sae->tmp->ec && sae_derive_pwe_ecc(sae, addr1, addr2, password,
					  password_len) < 0)
		return -1;
	if (sae->tmp->dh && sae_derive_pwe_ffc(sae, addr1, addr2, password,
					  password_len) < 0)
		return -1;
	if (sae_derive_commit(sae) < 0)
		return -1;
	return 0;
}


static int sae_derive_k_ecc(struct sae_data *sae, u8 *k)
{
	struct crypto_ec_point *K;
	int ret = -1;

	K = crypto_ec_point_init(sae->tmp->ec);
	if (K == NULL)
		goto fail;

	/*
	 * K = scalar-op(rand, (elem-op(scalar-op(peer-commit-scalar, PWE),
	 *                                        PEER-COMMIT-ELEMENT)))
	 * If K is identity element (point-at-infinity), reject
	 * k = F(K) (= x coordinate)
	 */

	if (crypto_ec_point_mul(sae->tmp->ec, sae->tmp->pwe_ecc,
				sae->peer_commit_scalar, K) < 0 ||
	    crypto_ec_point_add(sae->tmp->ec, K,
				sae->tmp->peer_commit_element_ecc, K) < 0 ||
	    crypto_ec_point_mul(sae->tmp->ec, K, sae->tmp->sae_rand, K) < 0 ||
	    crypto_ec_point_is_at_infinity(sae->tmp->ec, K) ||
	    crypto_ec_point_to_bin(sae->tmp->ec, K, k, NULL) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to calculate K and k");
		goto fail;
	}

	wpa_hexdump_key(MSG_DEBUG, "SAE: k", k, sae->tmp->prime_len);

	ret = 0;
fail:
	crypto_ec_point_deinit(K, 1);
	return ret;
}


static int sae_derive_k_ffc(struct sae_data *sae, u8 *k)
{
	struct crypto_bignum *K;
	int ret = -1;

	K = crypto_bignum_init();
	if (K == NULL)
		goto fail;

	/*
	 * K = scalar-op(rand, (elem-op(scalar-op(peer-commit-scalar, PWE),
	 *                                        PEER-COMMIT-ELEMENT)))
	 * If K is identity element (one), reject.
	 * k = F(K) (= x coordinate)
	 */

	if (crypto_bignum_exptmod(sae->tmp->pwe_ffc, sae->peer_commit_scalar,
				  sae->tmp->prime, K) < 0 ||
	    crypto_bignum_mulmod(K, sae->tmp->peer_commit_element_ffc,
				 sae->tmp->prime, K) < 0 ||
	    crypto_bignum_exptmod(K, sae->tmp->sae_rand, sae->tmp->prime, K) < 0
	    ||
	    crypto_bignum_is_one(K) ||
	    crypto_bignum_to_bin(K, k, SAE_MAX_PRIME_LEN, sae->tmp->prime_len) <
	    0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to calculate K and k");
		goto fail;
	}

	wpa_hexdump_key(MSG_DEBUG, "SAE: k", k, sae->tmp->prime_len);

	ret = 0;
fail:
	crypto_bignum_deinit(K, 1);
	return ret;
}


static int sae_derive_keys(struct sae_data *sae, const u8 *k)
{
	u8 null_key[SAE_KEYSEED_KEY_LEN], val[SAE_MAX_PRIME_LEN];
	u8 keyseed[SHA256_MAC_LEN];
	u8 keys[SAE_KCK_LEN + SAE_PMK_LEN];
	struct crypto_bignum *tmp;
	int ret = -1;

	tmp = crypto_bignum_init();
	if (tmp == NULL)
		goto fail;

	/* keyseed = H(<0>32, k)
	 * KCK || PMK = KDF-512(keyseed, "SAE KCK and PMK",
	 *                      (commit-scalar + peer-commit-scalar) modulo r)
	 * PMKID = L((commit-scalar + peer-commit-scalar) modulo r, 0, 128)
	 */

	os_memset(null_key, 0, sizeof(null_key));
	hmac_sha256(null_key, sizeof(null_key), k, sae->tmp->prime_len,
		    keyseed);
	wpa_hexdump_key(MSG_DEBUG, "SAE: keyseed", keyseed, sizeof(keyseed));

	crypto_bignum_add(sae->tmp->own_commit_scalar, sae->peer_commit_scalar,
			  tmp);
	crypto_bignum_mod(tmp, sae->tmp->order, tmp);
	crypto_bignum_to_bin(tmp, val, sizeof(val), sae->tmp->prime_len);
	wpa_hexdump(MSG_DEBUG, "SAE: PMKID", val, SAE_PMKID_LEN);
	sha256_prf(keyseed, sizeof(keyseed), "SAE KCK and PMK",
		   val, sae->tmp->prime_len, keys, sizeof(keys));
	os_memset(keyseed, 0, sizeof(keyseed));
	os_memcpy(sae->tmp->kck, keys, SAE_KCK_LEN);
	os_memcpy(sae->pmk, keys + SAE_KCK_LEN, SAE_PMK_LEN);
	os_memset(keys, 0, sizeof(keys));
	wpa_hexdump_key(MSG_DEBUG, "SAE: KCK", sae->tmp->kck, SAE_KCK_LEN);
	wpa_hexdump_key(MSG_DEBUG, "SAE: PMK", sae->pmk, SAE_PMK_LEN);

	ret = 0;
fail:
	crypto_bignum_deinit(tmp, 0);
	return ret;
}


int sae_process_commit(struct sae_data *sae)
{
	u8 k[SAE_MAX_PRIME_LEN];
	if (sae->tmp == NULL ||
	    (sae->tmp->ec && sae_derive_k_ecc(sae, k) < 0) ||
	    (sae->tmp->dh && sae_derive_k_ffc(sae, k) < 0) ||
	    sae_derive_keys(sae, k) < 0)
		return -1;
	return 0;
}


void sae_write_commit(struct sae_data *sae, struct wpabuf *buf,
		      const struct wpabuf *token)
{
	u8 *pos;

	if (sae->tmp == NULL)
		return;

	wpabuf_put_le16(buf, sae->group); /* Finite Cyclic Group */
	if (token) {
		wpabuf_put_buf(buf, token);
		wpa_hexdump(MSG_DEBUG, "SAE: Anti-clogging token",
			    wpabuf_head(token), wpabuf_len(token));
	}
	pos = wpabuf_put(buf, sae->tmp->prime_len);
	crypto_bignum_to_bin(sae->tmp->own_commit_scalar, pos,
			     sae->tmp->prime_len, sae->tmp->prime_len);
	wpa_hexdump(MSG_DEBUG, "SAE: own commit-scalar",
		    pos, sae->tmp->prime_len);
	if (sae->tmp->ec) {
		pos = wpabuf_put(buf, 2 * sae->tmp->prime_len);
		crypto_ec_point_to_bin(sae->tmp->ec,
				       sae->tmp->own_commit_element_ecc,
				       pos, pos + sae->tmp->prime_len);
		wpa_hexdump(MSG_DEBUG, "SAE: own commit-element(x)",
			    pos, sae->tmp->prime_len);
		wpa_hexdump(MSG_DEBUG, "SAE: own commit-element(y)",
			    pos + sae->tmp->prime_len, sae->tmp->prime_len);
	} else {
		pos = wpabuf_put(buf, sae->tmp->prime_len);
		crypto_bignum_to_bin(sae->tmp->own_commit_element_ffc, pos,
				     sae->tmp->prime_len, sae->tmp->prime_len);
		wpa_hexdump(MSG_DEBUG, "SAE: own commit-element",
			    pos, sae->tmp->prime_len);
	}
}


u16 sae_group_allowed(struct sae_data *sae, int *allowed_groups, u16 group)
{
	if (allowed_groups) {
		int i;
		for (i = 0; allowed_groups[i] > 0; i++) {
			if (allowed_groups[i] == group)
				break;
		}
		if (allowed_groups[i] != group) {
			wpa_printf(MSG_DEBUG, "SAE: Proposed group %u not "
				   "enabled in the current configuration",
				   group);
			return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
		}
	}

	if (sae->state == SAE_COMMITTED && group != sae->group) {
		wpa_printf(MSG_DEBUG, "SAE: Do not allow group to be changed");
		return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
	}

	if (group != sae->group && sae_set_group(sae, group) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Unsupported Finite Cyclic Group %u",
			   group);
		return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
	}

	if (sae->tmp == NULL) {
		wpa_printf(MSG_DEBUG, "SAE: Group information not yet initialized");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	if (sae->tmp->dh && !allowed_groups) {
		wpa_printf(MSG_DEBUG, "SAE: Do not allow FFC group %u without "
			   "explicit configuration enabling it", group);
		return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
	}

	return WLAN_STATUS_SUCCESS;
}


static void sae_parse_commit_token(struct sae_data *sae, const u8 **pos,
				   const u8 *end, const u8 **token,
				   size_t *token_len)
{
	if (*pos + (sae->tmp->ec ? 3 : 2) * sae->tmp->prime_len < end) {
		size_t tlen = end - (*pos + (sae->tmp->ec ? 3 : 2) *
				     sae->tmp->prime_len);
		wpa_hexdump(MSG_DEBUG, "SAE: Anti-Clogging Token", *pos, tlen);
		if (token)
			*token = *pos;
		if (token_len)
			*token_len = tlen;
		*pos += tlen;
	} else {
		if (token)
			*token = NULL;
		if (token_len)
			*token_len = 0;
	}
}


static u16 sae_parse_commit_scalar(struct sae_data *sae, const u8 **pos,
				   const u8 *end)
{
	struct crypto_bignum *peer_scalar;

	if (*pos + sae->tmp->prime_len > end) {
		wpa_printf(MSG_DEBUG, "SAE: Not enough data for scalar");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	peer_scalar = crypto_bignum_init_set(*pos, sae->tmp->prime_len);
	if (peer_scalar == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	/*
	 * IEEE Std 802.11-2012, 11.3.8.6.1: If there is a protocol instance for
	 * the peer and it is in Authenticated state, the new Commit Message
	 * shall be dropped if the peer-scalar is identical to the one used in
	 * the existing protocol instance.
	 */
	if (sae->state == SAE_ACCEPTED && sae->peer_commit_scalar &&
	    crypto_bignum_cmp(sae->peer_commit_scalar, peer_scalar) == 0) {
		wpa_printf(MSG_DEBUG, "SAE: Do not accept re-use of previous "
			   "peer-commit-scalar");
		crypto_bignum_deinit(peer_scalar, 0);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	/* 0 < scalar < r */
	if (crypto_bignum_is_zero(peer_scalar) ||
	    crypto_bignum_cmp(peer_scalar, sae->tmp->order) >= 0) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid peer scalar");
		crypto_bignum_deinit(peer_scalar, 0);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}


	crypto_bignum_deinit(sae->peer_commit_scalar, 0);
	sae->peer_commit_scalar = peer_scalar;
	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-scalar",
		    *pos, sae->tmp->prime_len);
	*pos += sae->tmp->prime_len;

	return WLAN_STATUS_SUCCESS;
}


static u16 sae_parse_commit_element_ecc(struct sae_data *sae, const u8 *pos,
					const u8 *end)
{
	u8 prime[SAE_MAX_ECC_PRIME_LEN];

	if (pos + 2 * sae->tmp->prime_len > end) {
		wpa_printf(MSG_DEBUG, "SAE: Not enough data for "
			   "commit-element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	if (crypto_bignum_to_bin(sae->tmp->prime, prime, sizeof(prime),
				 sae->tmp->prime_len) < 0)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	/* element x and y coordinates < p */
	if (os_memcmp(pos, prime, sae->tmp->prime_len) >= 0 ||
	    os_memcmp(pos + sae->tmp->prime_len, prime,
		      sae->tmp->prime_len) >= 0) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid coordinates in peer "
			   "element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-element(x)",
		    pos, sae->tmp->prime_len);
	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-element(y)",
		    pos + sae->tmp->prime_len, sae->tmp->prime_len);

	crypto_ec_point_deinit(sae->tmp->peer_commit_element_ecc, 0);
	sae->tmp->peer_commit_element_ecc =
		crypto_ec_point_from_bin(sae->tmp->ec, pos);
	if (sae->tmp->peer_commit_element_ecc == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	if (!crypto_ec_point_is_on_curve(sae->tmp->ec,
					 sae->tmp->peer_commit_element_ecc)) {
		wpa_printf(MSG_DEBUG, "SAE: Peer element is not on curve");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}


static u16 sae_parse_commit_element_ffc(struct sae_data *sae, const u8 *pos,
					const u8 *end)
{
	struct crypto_bignum *res;

	if (pos + sae->tmp->prime_len > end) {
		wpa_printf(MSG_DEBUG, "SAE: Not enough data for "
			   "commit-element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-element", pos,
		    sae->tmp->prime_len);

	crypto_bignum_deinit(sae->tmp->peer_commit_element_ffc, 0);
	sae->tmp->peer_commit_element_ffc =
		crypto_bignum_init_set(pos, sae->tmp->prime_len);
	if (sae->tmp->peer_commit_element_ffc == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	if (crypto_bignum_is_zero(sae->tmp->peer_commit_element_ffc) ||
	    crypto_bignum_is_one(sae->tmp->peer_commit_element_ffc) ||
	    crypto_bignum_cmp(sae->tmp->peer_commit_element_ffc,
			      sae->tmp->prime) >= 0) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid peer element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	/* scalar-op(r, ELEMENT) = 1 modulo p */
	res = crypto_bignum_init();
	if (res == NULL ||
	    crypto_bignum_exptmod(sae->tmp->peer_commit_element_ffc,
				  sae->tmp->order, sae->tmp->prime, res) < 0 ||
	    !crypto_bignum_is_one(res)) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid peer element (scalar-op)");
		crypto_bignum_deinit(res, 0);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	crypto_bignum_deinit(res, 0);

	return WLAN_STATUS_SUCCESS;
}


static u16 sae_parse_commit_element(struct sae_data *sae, const u8 *pos,
				    const u8 *end)
{
	if (sae->tmp->dh)
		return sae_parse_commit_element_ffc(sae, pos, end);
	return sae_parse_commit_element_ecc(sae, pos, end);
}


u16 sae_parse_commit(struct sae_data *sae, const u8 *data, size_t len,
		     const u8 **token, size_t *token_len, int *allowed_groups)
{
	const u8 *pos = data, *end = data + len;
	u16 res;

	/* Check Finite Cyclic Group */
	if (pos + 2 > end)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	res = sae_group_allowed(sae, allowed_groups, WPA_GET_LE16(pos));
	if (res != WLAN_STATUS_SUCCESS)
		return res;
	pos += 2;

	/* Optional Anti-Clogging Token */
	sae_parse_commit_token(sae, &pos, end, token, token_len);

	/* commit-scalar */
	res = sae_parse_commit_scalar(sae, &pos, end);
	if (res != WLAN_STATUS_SUCCESS)
		return res;

	/* commit-element */
	return sae_parse_commit_element(sae, pos, end);
}


static void sae_cn_confirm(struct sae_data *sae, const u8 *sc,
			   const struct crypto_bignum *scalar1,
			   const u8 *element1, size_t element1_len,
			   const struct crypto_bignum *scalar2,
			   const u8 *element2, size_t element2_len,
			   u8 *confirm)
{
	const u8 *addr[5];
	size_t len[5];
	u8 scalar_b1[SAE_MAX_PRIME_LEN], scalar_b2[SAE_MAX_PRIME_LEN];

	/* Confirm
	 * CN(key, X, Y, Z, ...) =
	 *    HMAC-SHA256(key, D2OS(X) || D2OS(Y) || D2OS(Z) | ...)
	 * confirm = CN(KCK, send-confirm, commit-scalar, COMMIT-ELEMENT,
	 *              peer-commit-scalar, PEER-COMMIT-ELEMENT)
	 * verifier = CN(KCK, peer-send-confirm, peer-commit-scalar,
	 *               PEER-COMMIT-ELEMENT, commit-scalar, COMMIT-ELEMENT)
	 */
	addr[0] = sc;
	len[0] = 2;
	crypto_bignum_to_bin(scalar1, scalar_b1, sizeof(scalar_b1),
			     sae->tmp->prime_len);
	addr[1] = scalar_b1;
	len[1] = sae->tmp->prime_len;
	addr[2] = element1;
	len[2] = element1_len;
	crypto_bignum_to_bin(scalar2, scalar_b2, sizeof(scalar_b2),
			     sae->tmp->prime_len);
	addr[3] = scalar_b2;
	len[3] = sae->tmp->prime_len;
	addr[4] = element2;
	len[4] = element2_len;
	hmac_sha256_vector(sae->tmp->kck, sizeof(sae->tmp->kck), 5, addr, len,
			   confirm);
}


static void sae_cn_confirm_ecc(struct sae_data *sae, const u8 *sc,
			       const struct crypto_bignum *scalar1,
			       const struct crypto_ec_point *element1,
			       const struct crypto_bignum *scalar2,
			       const struct crypto_ec_point *element2,
			       u8 *confirm)
{
	u8 element_b1[2 * SAE_MAX_ECC_PRIME_LEN];
	u8 element_b2[2 * SAE_MAX_ECC_PRIME_LEN];

	crypto_ec_point_to_bin(sae->tmp->ec, element1, element_b1,
			       element_b1 + sae->tmp->prime_len);
	crypto_ec_point_to_bin(sae->tmp->ec, element2, element_b2,
			       element_b2 + sae->tmp->prime_len);

	sae_cn_confirm(sae, sc, scalar1, element_b1, 2 * sae->tmp->prime_len,
		       scalar2, element_b2, 2 * sae->tmp->prime_len, confirm);
}


static void sae_cn_confirm_ffc(struct sae_data *sae, const u8 *sc,
			       const struct crypto_bignum *scalar1,
			       const struct crypto_bignum *element1,
			       const struct crypto_bignum *scalar2,
			       const struct crypto_bignum *element2,
			       u8 *confirm)
{
	u8 element_b1[SAE_MAX_PRIME_LEN];
	u8 element_b2[SAE_MAX_PRIME_LEN];

	crypto_bignum_to_bin(element1, element_b1, sizeof(element_b1),
			     sae->tmp->prime_len);
	crypto_bignum_to_bin(element2, element_b2, sizeof(element_b2),
			     sae->tmp->prime_len);

	sae_cn_confirm(sae, sc, scalar1, element_b1, sae->tmp->prime_len,
		       scalar2, element_b2, sae->tmp->prime_len, confirm);
}


void sae_write_confirm(struct sae_data *sae, struct wpabuf *buf)
{
	const u8 *sc;

	if (sae->tmp == NULL)
		return;

	/* Send-Confirm */
	sc = wpabuf_put(buf, 0);
	wpabuf_put_le16(buf, sae->send_confirm);
	sae->send_confirm++;

	if (sae->tmp->ec)
		sae_cn_confirm_ecc(sae, sc, sae->tmp->own_commit_scalar,
				   sae->tmp->own_commit_element_ecc,
				   sae->peer_commit_scalar,
				   sae->tmp->peer_commit_element_ecc,
				   wpabuf_put(buf, SHA256_MAC_LEN));
	else
		sae_cn_confirm_ffc(sae, sc, sae->tmp->own_commit_scalar,
				   sae->tmp->own_commit_element_ffc,
				   sae->peer_commit_scalar,
				   sae->tmp->peer_commit_element_ffc,
				   wpabuf_put(buf, SHA256_MAC_LEN));
}


int sae_check_confirm(struct sae_data *sae, const u8 *data, size_t len)
{
	u8 verifier[SHA256_MAC_LEN];

	if (len < 2 + SHA256_MAC_LEN) {
		wpa_printf(MSG_DEBUG, "SAE: Too short confirm message");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "SAE: peer-send-confirm %u", WPA_GET_LE16(data));

	if (sae->tmp == NULL) {
		wpa_printf(MSG_DEBUG, "SAE: Temporary data not yet available");
		return -1;
	}

	if (sae->tmp->ec)
		sae_cn_confirm_ecc(sae, data, sae->peer_commit_scalar,
				   sae->tmp->peer_commit_element_ecc,
				   sae->tmp->own_commit_scalar,
				   sae->tmp->own_commit_element_ecc,
				   verifier);
	else
		sae_cn_confirm_ffc(sae, data, sae->peer_commit_scalar,
				   sae->tmp->peer_commit_element_ffc,
				   sae->tmp->own_commit_scalar,
				   sae->tmp->own_commit_element_ffc,
				   verifier);

	if (os_memcmp_const(verifier, data + 2, SHA256_MAC_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "SAE: Confirm mismatch");
		wpa_hexdump(MSG_DEBUG, "SAE: Received confirm",
			    data + 2, SHA256_MAC_LEN);
		wpa_hexdump(MSG_DEBUG, "SAE: Calculated verifier",
			    verifier, SHA256_MAC_LEN);
		return -1;
	}

	return 0;
}
