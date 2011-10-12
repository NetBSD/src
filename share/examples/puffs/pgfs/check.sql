-- $NetBSD: check.sql,v 1.1 2011/10/12 01:05:00 yamt Exp $

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

-- filesystem consistency checks.  ie. something like "fsck -n"

BEGIN;
SET TRANSACTION ISOLATION LEVEL REPEATABLE READ;
SET TRANSACTION READ ONLY;
SET search_path TO pgfs;
SELECT count(*) AS "unreferenced files (dirent)"
	FROM file f LEFT JOIN dirent d
	ON f.fileid = d.child_fileid
	WHERE f.fileid <> 1 AND d.child_fileid IS NULL;
SELECT count(*) AS "unreferenced files (nlink)"
	FROM file f
	WHERE f.nlink = 0;
SELECT count(*) AS "regular files without datafork"
	FROM file f LEFT JOIN datafork df
	ON f.fileid = df.fileid
	WHERE df.fileid IS NULL AND f.type IN ('regular', 'link');
SELECT count(*) AS "broken datafork reference"
	FROM file f INNER JOIN datafork df
	ON f.fileid = df.fileid
	WHERE f.type NOT IN ('regular', 'link');
SELECT count(*) AS "unreferenced dataforks"
	FROM file f RIGHT JOIN datafork df
	ON f.fileid = df.fileid
	WHERE f.fileid IS NULL;
SELECT count(*) AS "dataforks without large object"
	FROM datafork df LEFT JOIN pg_largeobject_metadata lm
	ON df.loid = lm.oid
	WHERE lm.oid IS NULL;
SELECT count(*) AS "unreferenced large objects"
	FROM datafork df RIGHT JOIN pg_largeobject_metadata lm
	ON df.loid = lm.oid
	WHERE df.loid IS NULL;
SELECT count(*) AS "dirent broken parent_fileid references"
	FROM dirent d LEFT JOIN file f
	ON d.parent_fileid = f.fileid
	WHERE f.fileid IS NULL OR f.type <> 'directory';
SELECT count(*) AS "dirent broken child_fileid references"
	FROM dirent d LEFT JOIN file f
	ON d.child_fileid = f.fileid
	WHERE f.fileid IS NULL;
SELECT count(*) AS "dirent loops" FROM file f WHERE EXISTS (
	WITH RECURSIVE r AS
	(
			SELECT d.* FROM dirent d
				WHERE d.child_fileid = f.fileid
		UNION ALL
			SELECT d.* FROM dirent d INNER JOIN r
				ON d.child_fileid = r.parent_fileid
	)
	SELECT * FROM r WHERE r.parent_fileid = f.fileid);
SELECT count(*) AS "broken nlink"
	FROM
	(
	SELECT coalesce(fp.fileid, fc.fileid) AS fileid,
		coalesce(fp.nlink, 0) + coalesce(fc.nlink, 0) +
		CASE
			WHEN coalesce(fp.fileid, fc.fileid) = 1 THEN 1
			ELSE 0
		END
		AS nlink
		FROM
		(
		SELECT child_fileid AS fileid, count(*) AS nlink
			FROM dirent
			GROUP BY child_fileid
		) fp
		FULL JOIN
		(
		SELECT count(*) AS nlink, d.parent_fileid AS fileid
			FROM dirent d
			JOIN file f
			ON d.child_fileid = f.fileid
			WHERE f.type = 'directory'
			GROUP BY parent_fileid
		) fc
		ON fp.fileid = fc.fileid
	) d
	FULL JOIN file f
	ON d.fileid = f.fileid
	WHERE (d.nlink IS NULL AND (f.fileid <> 1 AND f.nlink <> 0))
	    OR f.nlink IS NULL
	    OR d.nlink <> f.nlink;
COMMIT;
