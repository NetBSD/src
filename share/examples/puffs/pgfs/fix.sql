-- $NetBSD: fix.sql,v 1.2 2011/10/12 16:24:39 yamt Exp $

-- Copyright (c)2011 YAMAMOTO Takashi,
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

-- remove orphaned files unless there's mount_pgfs connectd this db

BEGIN;
SET TRANSACTION ISOLATION LEVEL REPEATABLE READ;
SET search_path TO pgfs;
WITH
pgfs_clients AS (SELECT -count(*) FROM pg_stat_activity WHERE
	application_name = 'pgfs' AND datid IN (SELECT oid FROM pg_database
	WHERE datname = current_database())),
files_to_remove AS (DELETE FROM file WHERE nlink IN (SELECT * FROM pgfs_clients)
	RETURNING fileid),
removed_files AS (DELETE FROM datafork WHERE fileid IN (SELECT * FROM
	files_to_remove) RETURNING fileid, loid)
SELECT fileid AS "orphaned files" FROM removed_files WHERE lo_unlink(loid) = 1;
COMMIT;
