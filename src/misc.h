// misc.h
// ======
// Misc functions that are used often, but not included in C++.
// This will only work in C++.

// Have now made it so this will work with a MFC message box to report errors
// if asked to do by setting GUI_ERRORS to yes.
// Also logs errors to file now.

#pragma once

#ifndef MISC_H
#define MISC_H

using namespace std;

// ***************************************************************************
// *                                  INCLUDES                               *
// ***************************************************************************

// Headers used by this header's functions.
#include <stdlib.h>                      // For exit() function.
#include <fstream>                       // For the cout/cerr (+log) function.
#include <iostream>                      // For the cout/cerr (+log) function.
#include <string.h>                      // For filename stuff.
#include <assert.h>						 // So we can use assert in code.
#include <stdarg.h>                      // For multiple arguments to Error().
#include <float.h>                       // For EPSILION, MAX_FLOAT, etc

// ***************************************************************************
// *                     DATA TYPES / CONSTANTS / MACROS                     *
// ***************************************************************************

// To save on typeing the unsigned modifier.
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int UINT;
typedef unsigned long ULONG;

// For standard data types based in bit size (Change for compiler!).
typedef bool BIT;             // Best to use C++ bool if possible.
typedef uint8_t NIBBLE;          // 4-bits (No 4-bit type in C/C++!)
typedef uint8_t BYTE;            // 8-Bits.
typedef uint16_t WORD;            // 16-Bits.
typedef uint32_t DWORD;           // 32-Bits (Double word).

// For Boolean use and testing in C++.
#define FALSE           0               // Like C++ 'false'.
#define TRUE            1               // Like C++ 'true'.

// For use as No and Yes.
#define NO              0               // To make reading easier.
#define YES             1               // To make reading easier.

// For turning the Bit type On or Off.
#define OFF             0               // Bit is off.
#define ON              1               // Bit is on.

// So we can use ASSERT rather than assert.
#ifndef ASSERT
#define ASSERT			assert
#endif

// Returns the Minimum and Maximum using macro (for C polymorphism sake).
#define Min(N1,N2)      ((N1<=N2)?N1:N2)
#define Max(N1,N2)      ((N1>N2)?N1:N2)

// For common maths values (Some may be already in C/C++).
#ifndef PI
#define PI              3.14159265359    // PI for maths stuff.
#endif
// The smallest floating point error we expect to see.
#define EPSILON         DBL_EPSILON //0.000001 // 1/10^6 seems OK.

// Report errors in a standard way - used in as ErrorType to Error() function.
#define WARNING         0                // Print message and don't stop.
#define FATAL           1                // Fatal error, i.e. Stop.
#define ERROR_STREAM    cout             // Use cout/cerr (I hate really cerr!)
#define REPORT_WARNINGS YES              // YES or NO - to report or silent.
#define LOG_ERRORS      YES              // Dump to error.log file.
#define DELIBERATE_SEG  NO              // Break, for debugger back-trace.
#ifndef GUI_ERRORS
#define GUI_ERRORS      NO               // Use a Win32 message box for error.
#endif

// Used for all that must have a static text buffer passed to them.
#define BUFFER_SIZE     1000             // Size of static text buffer.

// Maximum floating point/double initialization value for very large number.
#define MAX_FLOAT       DBL_MAX //1.0e99 // May need to be larger/smaller later.

// ###########################################################################
// #                                  FUNCTIONS                              #
// ###########################################################################

inline void Error(int ErrorType, const char *ErrorMessage1,
		const char *ErrorMessage2 = NULL, const char *ErrorMessage3 = NULL) { // Prints an error message to the screen which is either Fatal or a Warning.
																			  // Also, if asked, will log errors to the error.log file.
																			  // NOTE: Now can print up to 3 messages if passed.

	ofstream LogFile;

	// To track down bugs - do this for good segmentation fault!
#if DELIBERATE_SEG==YES
  (*((int*)0))=1;
#endif

	// Lets make one big error message from the 3 we have been passed.
	char ErrorMessage[BUFFER_SIZE + 1];
	strcpy(ErrorMessage, ErrorMessage1);
	if (ErrorMessage2 != NULL) {
		strcat(ErrorMessage, "|");
		strcat(ErrorMessage, ErrorMessage2);
	}
	if (ErrorMessage3 != NULL) {
		strcat(ErrorMessage, "|");
		strcat(ErrorMessage, ErrorMessage3);
	}

	// Find out what king of error we are going to display.
	if (ErrorType == WARNING && REPORT_WARNINGS == YES) {
#if GUI_ERRORS==YES
       MessageBox(0,ErrorMessage,"Warning...",MB_OK|MB_ICONWARNING);
#endif
		if (LOG_ERRORS == YES) {
			LogFile.open("error.log", ios::app);
			LogFile << "WARNING: [" << ErrorMessage << "] Continuing..."
					<< endl;
			LogFile.close();
		}
		ERROR_STREAM << "WARNING: [" << ErrorMessage << "] Continuing..."
				<< endl;
	} else {
#if GUI_ERRORS==YES
      MessageBox (0,ErrorMessage,"Error...",MB_OK|MB_ICONERROR);
#endif
		if (LOG_ERRORS == YES) {
			LogFile.open("error.log", ios::app);
			LogFile << "FATAL: [" << ErrorMessage << "] Stopping..." << endl;
			LogFile.close();
		}
		ERROR_STREAM << "FATAL: [" << ErrorMessage << "] Stopping..." << endl;
		exit(1);                             // Stopping with error.
	}

} // End Error.

#endif
