/*	$NetBSD: vi_db.h,v 1.2 2013/11/22 15:52:05 christos Exp $	*/

#include <db.h>

#ifndef DB_BUFFER_SMALL
#define DB_BUFFER_SMALL		ENOMEM
#endif

#ifdef USE_BUNDLED_DB

typedef void DB_ENV;

typedef recno_t db_recno_t;
#define DB_MAX_RECORDS MAX_REC_NUMBER

#define db_env_close(env,flags)
#define db_env_create(env,flags)					\
	(((void)env), 1)
#define db_env_remove(env,path,flags)					\
	1
#define db_open(db,file,type,flags,mode)				\
    (db)->open(db, file, NULL, type, flags, mode)
#define db_get_low(db,key,data,flags)					\
    (db)->get(db, key, data, flags)
#define db_close(db)							\
    (db)->close(db)

#else

#if DB_VERSION_MAJOR >= 3 && DB_VERSION_MINOR >= 1
#define db_env_open(env,path,flags,mode)				\
    (env)->open(env, path, flags, mode)
#define db_env_remove(env,path,flags)					\
    (env)->remove(env, path, flags)
#else
#define db_env_open(env,path,flags,mode)				\
    (env)->open(env, path, NULL, flags, mode)
#define db_env_remove(env,path,flags)					\
    (env)->remove(env, path, NULL, flags)
#endif

#define db_env_close(env,flags)						\
    (env)->close(env, flags)

#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
#define db_open(db,file,type,flags,mode)				\
    (db)->open(db, NULL, file, NULL, type, flags, mode)
#else
#define db_open(db,file,type,flags,mode)				\
    (db)->open(db, file, NULL, type, flags, mode)
#endif
#define db_get_low(db,key,data,flags)					\
    (db)->get(db, NULL, key, data, flags)
#define db_close(db)							\
    (db)->close(db, DB_NOSYNC)

#endif

#ifdef USE_DYNAMIC_LOADING
#define db_create   	nvi_db_create
#define db_env_create   nvi_db_env_create
#define db_strerror 	nvi_db_strerror

extern int   (*nvi_db_create) __P((DB **, DB_ENV *, u_int32_t));
extern int   (*nvi_db_env_create) __P((DB_ENV **, u_int32_t));
extern char *(*nvi_db_strerror) __P((int));
#endif
