
#ifdef BUILDING_LIBASPRINTF
#define LIBASPRINTF_DLL_EXPORTED __declspec(dllexport)
#else
#define LIBASPRINTF_DLL_EXPORTED __declspec(dllimport)
#endif
