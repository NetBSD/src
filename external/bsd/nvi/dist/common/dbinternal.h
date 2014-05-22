/*	$NetBSD: dbinternal.h,v 1.2.4.2 2014/05/22 15:50:33 yamt Exp $	*/
#if !defined(db_env_create) || defined(USE_DB1)
int db_env_create(DB_ENV **, u_int32_t);
#endif
int db_create(DB **, DB_ENV *, u_int32_t);
const char *db_strerror(int);
