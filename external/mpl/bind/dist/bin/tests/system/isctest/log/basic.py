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

from pathlib import Path

import logging
import textwrap

LOG_FORMAT = "%(asctime)s %(levelname)7s:%(name)s  %(message)s"
LOG_INDENT = 4

LOGGERS: dict[str, logging.Logger | None] = {
    "conftest": None,
    "module": None,
    "test": None,
}


def init_conftest_logger():
    """
    This initializes the conftest logger which is used for pytest setup
    and configuration before tests are executed -- aka any logging in this
    file that is _not_ module-specific.
    """
    LOGGERS["conftest"] = logging.getLogger("conftest")
    LOGGERS["conftest"].setLevel(logging.DEBUG)
    file_handler = logging.FileHandler("pytest.conftest.log.txt")
    file_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    LOGGERS["conftest"].addHandler(file_handler)


def avoid_duplicated_logs():
    """
    Remove direct root logger output to file descriptors.
    This default is causing duplicates because all our messages go through
    regular logging as well and are thus displayed twice.
    """
    todel = []
    for handler in logging.root.handlers:
        if handler.__class__ == logging.StreamHandler:
            # Beware: As for pytest 7.2.2, LiveLogging and LogCapture
            # handlers inherit from logging.StreamHandler
            todel.append(handler)
    for handler in todel:
        logging.root.handlers.remove(handler)


def init_module_logger(system_test_name: str, testdir: Path):
    logger = logging.getLogger(system_test_name)
    logger.handlers.clear()
    logger.setLevel(logging.DEBUG)
    handler = logging.FileHandler(testdir / "pytest.log.txt", mode="w")
    handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(handler)
    LOGGERS["module"] = logger


def deinit_module_logger():
    for handler in LOGGERS["module"].handlers:
        handler.flush()
        handler.close()
    LOGGERS["module"] = None


def init_test_logger(system_test_name: str, test_name: str):
    LOGGERS["test"] = logging.getLogger(f"{system_test_name}.{test_name}")


def deinit_test_logger():
    LOGGERS["test"] = None


def indent_message(msg):
    lines = msg.splitlines()
    first = lines[0] + "\n"
    to_indent = "\n".join(lines[1:])
    return first + textwrap.indent(to_indent, " " * LOG_INDENT)


def log(lvl: int, msg: str, *args, **kwargs):
    """Log message with the most-specific logger currently available."""
    logger = LOGGERS["test"]
    if logger is None:
        logger = LOGGERS["module"]
    if logger is None:
        logger = LOGGERS["conftest"]
    assert logger is not None
    if "\n" in msg:
        msg = indent_message(msg)
    logger.log(lvl, msg, *args, **kwargs)


def debug(msg: str, *args, **kwargs):
    log(logging.DEBUG, msg, *args, **kwargs)


def info(msg: str, *args, **kwargs):
    log(logging.INFO, msg, *args, **kwargs)


def warning(msg: str, *args, **kwargs):
    log(logging.WARNING, msg, *args, **kwargs)


def error(msg: str, *args, **kwargs):
    log(logging.ERROR, msg, *args, **kwargs)


def critical(msg: str, *args, **kwargs):
    log(logging.CRITICAL, msg, *args, **kwargs)
