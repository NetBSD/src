/*
 * Autofs is not supported on this platform,
 * so disable it if it gets detected.
 */

#ifdef MNTTYPE_AUTOFS
# undef MNTTYPE_AUTOFS
#endif /* MNTTYPE_AUTOFS */
#ifdef MNTTAB_TYPE_AUTOFS
# undef MNTTAB_TYPE_AUTOFS
#endif /* MNTTAB_TYPE_AUTOFS */
#ifdef HAVE_FS_AUTOFS
# undef HAVE_FS_AUTOFS
#endif /* HAVE_FS_AUTOFS */
