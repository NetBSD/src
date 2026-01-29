# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

import hashlib
import os
from re import compile as Re
import shutil

import pytest

import isctest.mark


pytestmark = [
    isctest.mark.softhsm2_environment,
    pytest.mark.extra_artifacts(
        [
            "*.example.db",
            "*.example.db.signed",
            "K*",
            "dsset-*",
            "keyfromlabel.out.*",
            "pin",
            "pkcs11-tool.out.*",
            "signer.out.*",
        ],
    ),
]


EMPTY_OPENSSL_CONF_ENV = {**os.environ, "OPENSSL_CONF": ""}

HSMPIN = "1234"


@pytest.fixture(autouse=True)
def token_init_and_cleanup():

    # Create pin file for the $KEYFRLAB command
    with open("pin", "w", encoding="utf-8") as pinfile:
        pinfile.write(HSMPIN)

    token_init_command = [
        "softhsm2-util",
        "--init-token",
        "--free",
        "--pin",
        HSMPIN,
        "--so-pin",
        HSMPIN,
        "--label",
        "softhsm2-keyfromlabel",
    ]

    token_cleanup_command = [
        "softhsm2-util",
        "--delete-token",
        "--token",
        "softhsm2-keyfromlabel",
    ]

    isctest.run.cmd(
        token_cleanup_command,
        env=EMPTY_OPENSSL_CONF_ENV,
        raise_on_exception=False,
    )

    try:
        cmd = isctest.run.cmd(token_init_command, env=EMPTY_OPENSSL_CONF_ENV)
        assert "The token has been initialized and is reassigned to slot" in cmd.out
        yield
    finally:
        cmd = isctest.run.cmd(
            token_cleanup_command,
            env=EMPTY_OPENSSL_CONF_ENV,
            raise_on_exception=False,
        )
        assert Re("Found token (.*) with matching token label") in cmd.out


# pylint: disable-msg=too-many-locals
@pytest.mark.parametrize(
    "alg_name,alg_type,alg_bits",
    [
        ("rsasha256", "rsa", "2048"),
        ("rsasha512", "rsa", "2048"),
        ("ecdsap256sha256", "EC", "prime256v1"),
        ("ecdsap384sha384", "EC", "prime384v1"),
        # Edwards curves are not yet supported by OpenSC
        # ("ed25519","EC","edwards25519"),
        # ("ed448","EC","edwards448")
    ],
)
def test_keyfromlabel(alg_name, alg_type, alg_bits):

    def keygen(alg_type, alg_bits, zone, key_id):
        label = f"{key_id}-{zone}"
        p11_id = hashlib.sha1(label.encode("utf-8")).hexdigest()

        pkcs11_command = [
            "pkcs11-tool",
            "--module",
            os.environ.get("SOFTHSM2_MODULE"),
            "--token-label",
            "softhsm2-keyfromlabel",
            "-l",
            "-k",
            "--key-type",
            f"{alg_type}:{alg_bits}",
            "--label",
            label,
            "--id",
            p11_id,
            "--pin",
            HSMPIN,
        ]

        cmd = isctest.run.cmd(pkcs11_command, env=EMPTY_OPENSSL_CONF_ENV)

        assert "Key pair generated" in cmd.out

    def keyfromlabel(alg_name, zone, key_id, key_flag):
        key_flag = key_flag.split() if key_flag else []

        keyfrlab_command = [
            os.environ["KEYFRLAB"],
            *os.environ.get("ENGINE_ARG", "").split(),
            "-a",
            alg_name,
            "-y",
            "-l",
            f"pkcs11:token=softhsm2-keyfromlabel;object={key_id}-{zone};pin-source=pin",
            *key_flag,
            zone,
        ]

        cmd = isctest.run.cmd(keyfrlab_command)
        keyfile = cmd.out.rstrip() + ".key"

        assert os.path.exists(keyfile)

        return keyfile

    if f"{alg_name.upper()}_SUPPORTED" not in os.environ:
        pytest.skip(f"{alg_name} is not supported")

    # Generate keys for the $zone zone
    zone = f"{alg_name}.example"

    keygen(alg_type, alg_bits, zone, "keyfromlabel-zsk")
    keygen(alg_type, alg_bits, zone, "keyfromlabel-ksk")

    # Get ZSK
    zsk_file = keyfromlabel(alg_name, zone, "keyfromlabel-zsk", "")

    # Get KSK
    ksk_file = keyfromlabel(alg_name, zone, "keyfromlabel-ksk", "-f KSK")

    # Sign zone with KSK and ZSK
    zone_file = f"zone.{alg_name}.example.db"

    with open(zone_file, "w", encoding="utf-8") as outfile:
        for f in ["template.db.in", ksk_file, zsk_file]:
            with open(f, "r", encoding="utf-8") as fd:
                shutil.copyfileobj(fd, outfile)

    signer_command = [
        os.environ["SIGNER"],
        *os.environ.get("ENGINE_ARG", "").split(),
        "-S",
        "-a",
        "-g",
        "-o",
        zone,
        zone_file,
    ]
    isctest.run.cmd(signer_command)

    assert os.path.exists(f"{zone_file}.signed")
