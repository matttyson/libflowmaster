#ifndef FLOWMASTER_INTERNAL_H
#define FLOWMASTER_INTERNAL_H

#if defined _WIN32
	#ifdef FM_BUILDING_DLL
		/* Win32 build */
		#if defined _MSC_VER
			#define DLLEXPORT __declspec(dllexport)
		#elif defined __GNUC__
			#define DLLEXPORT __attribute__((dllexport))
		#endif
	#elif defined FM_STATIC
		#define DLLEXPORT
	#else
		/* Win32 include*/
		#if defined _MSC_VER
			#define DLLEXPORT __declspec(dllimport)
		#elif defined __GNUC__
			#define DLLEXPORT __attribute__((dllimport))
		#endif
	#endif

	#include "flowmaster_win32.h"
#else
 /* Linux */
	#define DLLEXPORT __attribute__((visibility ("default")))

	#include "flowmaster_linux.h"
#endif

#endif
