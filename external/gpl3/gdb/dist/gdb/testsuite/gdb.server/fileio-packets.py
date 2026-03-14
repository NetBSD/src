# Copyright (C) 2025 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import stat


# Hex encode INPUT_STRING in the same way that GDB does.  Each
# character in INPUT_STRING is expanded to its two digit hex
# representation in the returned string.
#
# Only ASCII characters may appear in INPUT_STRING, this is more
# restrictive than GDB, but is good enough for testing.
def hex_encode(input_string):
    byte_string = input_string.encode("ascii")
    hex_string = byte_string.hex()
    return hex_string


# Binary remote data packets can contain some escaped bytes.  Decode
# the packet now.
def unescape_remote_data(buf):
    escaped = False
    res = bytearray()
    for b in buf:
        if escaped:
            res.append(b ^ 0x20)
            escaped = False
        elif b == ord("}"):
            escaped = True
        else:
            res.append(b)
    res = bytes(res)
    return res


# Decode the results of a remote stat like command from BUF.  Returns
# None if BUF is not a valid stat result (e.g. if it indicates an
# error, or the buffer is too short).  If BUF is valid then the fields
# are decoded according to the GDB remote protocol and placed into a
# dictionary, this dictionary is then returned.
def decode_stat_reply(buf, byteorder="big"):

    buf = unescape_remote_data(buf)

    if (
        buf[0] != ord("F")
        or buf[1] != ord("4")
        or buf[2] != ord("0")
        or buf[3] != ord(";")
        or len(buf) != 68
    ):
        l = len(buf)
        print(f"decode_stat_reply failed: {buf}\t(length = {l})")
        return None

    # Discard the 'F40;' prefix.  The rest is the 64 bytes of data to
    # be decoded.
    buf = buf[4:]

    st_dev = int.from_bytes(buf[0:4], byteorder=byteorder)
    st_ino = int.from_bytes(buf[4:8], byteorder=byteorder)
    st_mode = int.from_bytes(buf[8:12], byteorder=byteorder)
    st_nlink = int.from_bytes(buf[12:16], byteorder=byteorder)
    st_uid = int.from_bytes(buf[16:20], byteorder=byteorder)
    st_gid = int.from_bytes(buf[20:24], byteorder=byteorder)
    st_rdev = int.from_bytes(buf[24:28], byteorder=byteorder)
    st_size = int.from_bytes(buf[28:36], byteorder=byteorder)
    st_blksize = int.from_bytes(buf[36:44], byteorder=byteorder)
    st_blocks = int.from_bytes(buf[44:52], byteorder=byteorder)
    st_atime = int.from_bytes(buf[52:56], byteorder=byteorder)
    st_mtime = int.from_bytes(buf[56:60], byteorder=byteorder)
    st_ctime = int.from_bytes(buf[60:64], byteorder=byteorder)

    return {
        "st_dev": st_dev,
        "st_ino": st_ino,
        "st_mode": st_mode,
        "st_nlink": st_nlink,
        "st_uid": st_uid,
        "st_gid": st_gid,
        "st_rdev": st_rdev,
        "st_size": st_size,
        "st_blksize": st_blksize,
        "st_blocks": st_blocks,
        "st_atime": st_atime,
        "st_mtime": st_mtime,
        "st_ctime": st_ctime,
    }


# Perform an lstat of remote file FILENAME, and create a dictionary of
# the results, the keys are the fields of the stat structure.
def remote_lstat(filename):
    conn = gdb.selected_inferior().connection
    if not isinstance(conn, gdb.RemoteTargetConnection):
        raise gdb.GdbError("connection is the wrong type")

    filename_hex = hex_encode(filename)
    reply = conn.send_packet("vFile:lstat:%s" % filename_hex)

    stat = decode_stat_reply(reply)
    return stat


# Perform a stat of remote file FILENAME, and create a dictionary of
# the results, the keys are the fields of the stat structure.
def remote_stat(filename):
    conn = gdb.selected_inferior().connection
    if not isinstance(conn, gdb.RemoteTargetConnection):
        raise gdb.GdbError("connection is the wrong type")

    filename_hex = hex_encode(filename)
    reply = conn.send_packet("vFile:stat:%s" % filename_hex)

    stat = decode_stat_reply(reply)
    return stat


# Convert a stat_result object to a dictionary that should match the
# dictionary built from the remote protocol reply.
def stat_result_to_dict(res):
    # GDB doesn't support the S_IFLNK flag for the remote protocol, so
    # clear that flag in the local results.
    if stat.S_ISLNK(res.st_mode):
        st_mode = stat.S_IMODE(res.st_mode)
    else:
        st_mode = res.st_mode

    # GDB returns an integer for these fields, while Python returns a
    # floating point value.  Convert back to an integer to match GDB.
    st_atime = int(res.st_atime)
    st_mtime = int(res.st_mtime)
    st_ctime = int(res.st_ctime)

    return {
        "st_dev": res.st_dev,
        "st_ino": res.st_ino,
        "st_mode": st_mode,
        "st_nlink": res.st_nlink,
        "st_uid": res.st_uid,
        "st_gid": res.st_gid,
        "st_rdev": res.st_rdev,
        "st_size": res.st_size,
        "st_blksize": res.st_blksize,
        "st_blocks": res.st_blocks,
        "st_atime": st_atime,
        "st_mtime": st_mtime,
        "st_ctime": st_ctime,
    }


# Perform an lstat of local file FILENAME, and create a dictionary of
# the results, the keys are the fields of the stat structure.
def local_lstat(filename):
    res = os.lstat(filename)
    return stat_result_to_dict(res)


# Perform an lstat of local file FILENAME, and create a dictionary of
# the results, the keys are the fields of the stat structure.
def local_stat(filename):
    res = os.stat(filename)
    return stat_result_to_dict(res)


# Perform a remote lstat using GDB, and a local lstat using os.lstat.
# Compare the results to check they are the same.
#
# For this test to work correctly, gdbserver, and GDB (where this
# Python script is running), must see the same filesystem.
def check_lstat(filename):
    s1 = remote_lstat(filename)
    s2 = local_lstat(filename)

    print(f"remote = {s1}")
    print(f"local  = {s2}")

    assert s1 == s2
    print("PASS")


# Perform a remote stat using GDB, and a local stat using os.stat.
# Compare the results to check they are the same.
#
# For this test to work correctly, gdbserver, and GDB (where this
# Python script is running), must see the same filesystem.
def check_stat(filename):
    s1 = remote_stat(filename)
    s2 = local_stat(filename)

    print(f"remote = {s1}")
    print(f"local  = {s2}")

    assert s1 == s2
    print("PASS")
