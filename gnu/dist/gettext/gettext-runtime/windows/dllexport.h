
#ifdef BUILDING_LIBINTL
#define LIBINTL_DLL_EXPORTED __declspec(dllexport)
#else
#define LIBINTL_DLL_EXPORTED __declspec(dllimport)
#endif
