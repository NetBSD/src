/*	$NetBSD: namespace.h,v 1.1 2022/05/15 16:25:09 christos Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

/* argon2b.c */
#define	argon2_ctx	__libcrypt_internal_argon2_ctx
#define	argon2_encodedlen	__libcrypt_internal_argon2_encodedlen
#define	argon2_error_message	__libcrypt_internal_argon2_error_message
#define	argon2_hash	__libcrypt_internal_argon2_hash
#define	argon2_type2string	__libcrypt_internal_argon2_type2string
#define	argon2_verify	__libcrypt_internal_argon2_verify
#define	argon2_verify_ctx	__libcrypt_internal_argon2_verify_ctx
#define	argon2d_ctx	__libcrypt_internal_argon2d_ctx
#define	argon2d_hash_encoded	__libcrypt_internal_argon2d_hash_encoded
#define	argon2d_hash_raw	__libcrypt_internal_argon2d_hash_raw
#define	argon2d_verify	__libcrypt_internal_argon2d_verify
#define	argon2d_verify_ctx	__libcrypt_internal_argon2d_verify_ctx
#define	argon2i_ctx	__libcrypt_internal_argon2i_ctx
#define	argon2i_hash_encoded	__libcrypt_internal_argon2i_hash_encoded
#define	argon2i_hash_raw	__libcrypt_internal_argon2i_hash_raw
#define	argon2i_verify	__libcrypt_internal_argon2i_verify
#define	argon2i_verify_ctx	__libcrypt_internal_argon2i_verify_ctx
#define	argon2id_ctx	__libcrypt_internal_argon2id_ctx
#define	argon2id_hash_encoded	__libcrypt_internal_argon2id_hash_encoded
#define	argon2id_hash_raw	__libcrypt_internal_argon2id_hash_raw
#define	argon2id_verify	__libcrypt_internal_argon2id_verify
#define	argon2id_verify_ctx	__libcrypt_internal_argon2id_verify_ctx

/* blake2b.c */
#define	blake2b	__libcrypt_internal_blake2b
#define	blake2b_final	__libcrypt_internal_blake2b_final
#define	blake2b_init	__libcrypt_internal_blake2b_init
#define	blake2b_init_key	__libcrypt_internal_blake2b_init_key
#define	blake2b_init_param	__libcrypt_internal_blake2b_init_param
#define	blake2b_long	__libcrypt_internal_blake2b_long
#define	blake2b_update	__libcrypt_internal_blake2b_update

/* core.c */
#define	allocate_memory	__libcrypt_internal_allocate_memory
#define	clear_internal_memory	__libcrypt_internal_clear_internal_memory
#define	copy_block	__libcrypt_internal_copy_block
#define	fill_first_blocks	__libcrypt_internal_fill_first_blocks
#define	fill_memory_blocks	__libcrypt_internal_fill_memory_blocks
#define	finalize	__libcrypt_internal_finalize
#define	free_memory	__libcrypt_internal_free_memory
#define	index_alpha	__libcrypt_internal_index_alpha
#define	init_block_value	__libcrypt_internal_init_block_value
#define	initial_hash	__libcrypt_internal_initial_hash
#define	initialize	__libcrypt_internal_initialize
#define	secure_wipe_memory	__libcrypt_internal_secure_wipe_memory
#define	validate_inputs	__libcrypt_internal_validate_inputs
#define	xor_block	__libcrypt_internal_xor_block

/* crypt-argon2.c */
#define	estimate_argon2_params	__libcrypt_internal_estimate_argon2_params

/* encoding.c */
#define	b64len	__libcrypt_internal_b64len
#define	decode_string	__libcrypt_internal_decode_string
#define	encode_string	__libcrypt_internal_encode_string
#define	numlen	__libcrypt_internal_numlen

/* ref.c */
#define	fill_segment	__libcrypt_internal_fill_segment

/* util.c */
#define	getnum	__libcrypt_internal_getnum
