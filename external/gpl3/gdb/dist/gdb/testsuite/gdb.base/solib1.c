/* This program is intended to be built as an HP-UX SOM shared
   library, for use by the solib.exp testcase.  It simply returns
   the square of its integer argument.
   */
#if defined(__cplusplus) || defined(__STDCPP__)
extern "C"
#endif
int
solib_main (int arg)
{
  return arg*arg;		/* HERE */
}				/* STEP */
