/* This program is intended to be built as an HP-UX SOM shared
   library, for use by the solib.exp testcase.  It simply returns
   the cube of its integer argument.
   */
#ifdef __cplusplus
extern "C" {
#endif
int  solib_main (int  arg)
{
  return arg*arg*arg;
}
#ifdef __cplusplus
}
#endif
