#!@PYTHON@
############################################################################
# Copyright (C) 2013  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
############################################################################

# Id

# ntpath rewrote in Python

import win32con
import win32api

BIND_SUBKEY = "Software\\ISC\\BIND"

def base():
    hKey = None
    keyFound = True
    try:
        hKey = win32api.RegOpenKeyEx(win32con.HKEY_LOCAL_MACHINE, BIND_SUBKEY)
    except:
        keyFound = False
    if keyFound:
        try:
            (namedBase, _) = win32api.RegQueryValueEx(hKey, "InstallDir")
        except:
            keyFound = False
        win32api.RegCloseKey(hKey)
    if keyFound:
        return namedBase
    return win32api.GetSystemDirectory()
