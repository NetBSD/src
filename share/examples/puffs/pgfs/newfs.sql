-- $NetBSD: newfs.sql,v 1.4 2012/04/11 14:28:46 yamt Exp $

-- Copyright (c)2010,2011 YAMAMOTO Takashi,
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
--
-- THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
-- ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
-- FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
-- DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
-- OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
-- LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
-- OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
-- SUCH DAMAGE.

-- create a new pgfs filesystem.
-- usage: psql -f newfs.sql [dbname]

-- remove pgfs schema if already exists.
-- XXX this leaves large objects
BEGIN;
DROP SCHEMA pgfs CASCADE;
COMMIT;

BEGIN;
-- we put everything in the pgfs schema.  (except large objects)
CREATE SCHEMA pgfs;
SET search_path TO pgfs;

-- define various types
CREATE DOMAIN fileid AS int8 NOT NULL CHECK(VALUE > 0);
CREATE DOMAIN uid AS int8 NOT NULL CHECK(VALUE >= 0);
CREATE DOMAIN gid AS int8 NOT NULL CHECK(VALUE >= 0);
CREATE DOMAIN mode AS int8 NOT NULL CHECK(VALUE >= 0 AND VALUE <= 7*8*8+7*8+7);
CREATE DOMAIN nlink AS int8 NOT NULL CHECK(VALUE >= 0);
CREATE SEQUENCE fileid_seq START WITH 1; -- 1 will be used for root directory
CREATE SEQUENCE dircookie_seq START WITH 3; -- see PGFS_DIRCOOKIE_*
CREATE TYPE filetype AS ENUM (
	'regular',
	'directory',
	'link');

-- a row in the file table describes a file.
-- having nlink here is somehow redundant as it could be calculated from
-- filetype and the dirent table.  however, users expect that getattr is
-- executed in a nearly constant time.
CREATE TABLE file (
	fileid fileid PRIMARY KEY,
	type filetype NOT NULL,
	mode mode,
	uid uid,
	gid gid,
	nlink nlink,
	rev int8 NOT NULL,
	atime timestamptz NOT NULL,
	ctime timestamptz NOT NULL,
	mtime timestamptz NOT NULL,
	btime timestamptz NOT NULL);

-- datafork table maintains the association between our files and its backing
-- large objects.
CREATE TABLE datafork (
	fileid fileid PRIMARY KEY, -- REFERENCES file
	loid Oid NOT NULL UNIQUE);
-- we want the following but lo lives in system catalogs.
--	loid REFERENCES pg_largeobject_metadata(oid);

-- a row in the dirent table describes a directory entry.
-- the ".." and "." entries are handled differently and never appear here.
CREATE TABLE dirent (
	parent_fileid fileid NOT NULL, -- REFERENCES file,
	name text NOT NULL,
	cookie int8 NOT NULL UNIQUE DEFAULT nextval('dircookie_seq'),
	child_fileid fileid NOT NULL, -- REFERENCES file,
	CONSTRAINT dirent_pkey PRIMARY KEY(parent_fileid, name),
	CONSTRAINT dirent_notdot CHECK(name <> '.'),
	CONSTRAINT dirent_notdotdot CHECK(name <> '..'),
	CONSTRAINT dirent_noself CHECK(parent_fileid <> child_fileid));
CREATE INDEX dirent_child ON dirent (child_fileid);

-- create the root directory.
INSERT INTO file (fileid, type, mode, uid, gid, nlink, rev,
		atime, ctime, mtime, btime)
	VALUES (nextval('fileid_seq'), 'directory', 7*8*8 + 5*8 + 5, 0, 0, 1, 0,
		current_timestamp,
		current_timestamp,
		current_timestamp,
		current_timestamp);

-- create a dummy sequence.  see pgfs_node_fsync().
CREATE SEQUENCE dummyseq;

RESET search_path;
COMMIT;
