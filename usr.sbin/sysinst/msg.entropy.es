/*      $NetBSD: msg.entropy.es,v 1.5 2022/04/21 17:30:15 martin Exp $  */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

message Configure_entropy	{Set up entropy}

message continue_without_entropy	{Not now, continue!}

message not_enough_entropy
{This system seems to lack a cryptographically strong pseudo random
number generator. There is not enough entropy available to create secure
keys (e.g. ssh host keys). 
 
If you plan to use this installation for production work and will
for example have ssh host keys generated, we strongly advise to complete
the entropy setup now! 
 
You may use random data generated on another computer and load it
here, or you could enter random characters manually. 
 
If you have a USB random number device, connect it now and select
the "Re-test" option.}

message entropy_add_manually		{Manually input random characters}
message entropy_download_raw		{Load raw binary random data}
message	entropy_download_seed		{Import a NetBSD entropy file}
message entropy_retry			{Re-test}

message entropy_enter_manual1
{Enter one line of random characters.}

message entropy_enter_manual2
{They should contain at last 256 bits of randomness, as in 256 coin
tosses, 100 throws of a 6-sided die, 64 random hexadecimal digits, or
(if you are able to copy & paste output from another machine into this
installer) the output from running the following command on another
machine whose randomness you trust:}

message entropy_enter_manual3
{A line of any length and content will be accepted and assumed to
contain at least 256 bits of randomness.  If it actually contains
less, the installed system may not be secure.}

message entropy_select_file
{Please select how you want to transfer the random data file
to this machine:}

message entropy_add_download_ftp
{Download via ftp}

message entropy_add_download_http
{Download via http}

message download_entropy
{Start download}

message entropy_add_nfs
{Load from a NFS share}

message entropy_add_local
{Load from a local file system (e.g. a USB device)}

message entropy_file
{Path/file}

message load_entropy
{Load random data}

message set_entropy_file
{Random data file path}

/* Called with:				Example
 *  $0 = content of file		NetBSD entropy seed file
 */
message entropy_via_nfs
{Select a server, a share and the file path to load the $0.}

/* Called with:				Example
 *  $0 = content of file		NetBSD entropy seed file
 */
message entropy_via_download
{Since not enough entropy is available on this system, all crytographic
operations are suspect to replay attacks. 
Please only use trustworthy local networks.}

message entropy_data
{random data binary file}

message entropy_data_hdr
{On a system with cryptographically strong pseudo random number generator
you can create a file with random binary data like this:}

message entropy_seed
{NetBSD entropy seed file}

message entropy_seed_hdr
{On a NetBSD system with cryptographically strong pseudo random number
generator you can create an entropy snapshot like this:}

message entropy_path_and_file
{Path and filename}

message entropy_localfs
{Enter the unmounted local device and directory on that device where
the random data is located.}
