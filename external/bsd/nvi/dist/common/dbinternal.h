/*	$NetBSD: dbinternal.h,v 1.2.8.2 2014/08/19 23:51:51 tls Exp $	*/
#if !defined(db_env_create) || defined(USE_DB1)
int db_env_create(DB_ENV **, u_int32_t);
#endif
int db_create(DB **, DB_ENV *, u_int32_t);
const char *db_strerror(int);
