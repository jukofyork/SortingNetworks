// misc.h
// ======
// Misc functions that are used often, but not included in C++.
// This will only work in C++.

// Have now made it so this will work with a MFC message box to report errors
// if asked to do by setting GUI_ERRORS to yes.
// Also logs errors to file now.

#pragma once

// Disable silly warnings from Intel compiler when using -W4.
/*
#pragma warning (disable:981)	// "remark #981: operands are evaluated in unspecified order"
#pragma warning (disable:1418)	// "remark #1418: external function definition with no prior declaration"
#pragma warning (disable:858)	// "remark #858: type qualifier on return type is meaningless"
#pragma warning (disable:383)	// "remark #383: value copied to temporary, reference to temporary used"
#pragma warning (disable:869)	// "remark #869: parameter "XXXXXXXXXX" was never referenced"
#pragma warning (disable:1786)	// was declared "deprecated

// This might be important sometimes, but the cases it talks about are todo with the pokersrc
// library and they are safe (we just get the address to print...).
#pragma warning (disable:1563)	// "warning #1563: taking the address of a temporary"
*/

#ifndef MISC_H
#define MISC_H

// Use this so we don't get lots of warnings about the "_s" fuctions.
#define _CRT_SECURE_NO_DEPRECATE

// Use this for new VC.
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
#include <stdarg.h>                      // For multiple args to Error().
#include <float.h>                       // For EPSILION, MAX_FLOAT, etc

// ***************************************************************************
// *                     DATA TYPES / CONSTANTS / MACROS                     *
// ***************************************************************************

// To save on typeing the unsigned modifier.
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

// For standard data types based in bit size (Change for compiler!).
typedef bool            bit;             // Best to use C++ bool if possible.
typedef unsigned char   nibble;          // 4-bits (No 4-bit type in C/C++!) 
typedef unsigned char   byte;            // 8-Bits.
typedef unsigned short  word;            // 16-Bits.
typedef unsigned int    dword;           // 32-Bits (Double word).

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

// Returns the Minumum and Maximum using macro (for C polymorphism sake).
#define Min(N1,N2)      ((N1<=N2)?N1:N2)
#define Max(N1,N2)      ((N1>N2)?N1:N2)

// For common maths values (Some may be already in C/C++).
#ifndef PI
#define PI              3.14159265359    // PI for maths stuff.
#endif
// The smallest floating point error we expect to see.
#define EPSILON         DBL_EPSILON //0.000001 // 1/10^6 seems OK.

// Report errors in a standard way - used in as ErrorType to Eroor() function.
#define WARNING         0                // Print message and don't stop.
#define FATAL           1                // Fatal error, i.e. Stop.
#define ERROR_STREAM    cout             // Use cout/cerr (I hate realy cerr!)
#define REPORT_WARNINGS YES              // YES or NO - to report or silent.
#define LOG_ERRORS      YES              // Dump to error.log file.
#define DELIBERATE_SEG  NO              // Break, for debugger back-trace.
#ifndef GUI_ERRORS
#define GUI_ERRORS      NO               // Use a Win32 message box for error.
#endif

// This may be used so that we can use ANSI 'strcasecmp' when windows wants
// 'stricmp' for case insensitive string comparisons.
//#define strcasecmp      stricmp          // Only define for windows.

// Used for all that must have a static tect buffer passed to them.
#define BUFFER_SIZE     1000             // Size of static text buffer.

// Maximum floating point/double initing value for very large number.
#define MAX_FLOAT       DBL_MAX //1.0e99 // May need to be larger/smaller later.

// ###########################################################################
// #                                  FUNCTIONS                              #
// ###########################################################################

inline void Error(int ErrorType,
				  const char* ErrorMessage1,
				  const char* ErrorMessage2=NULL,
				  const char* ErrorMessage3=NULL)
{ // Prints an error message to the screen which is either Fatal or a Warning.
  // Also, if asked, will log errors to the error.log file.
  // NOTE: Now can print upto 3 messages if passed.

  ofstream LogFile;

  // To track down bugs - do this for good segmentation fault!
#if DELIBERATE_SEG==YES
  (*((int*)0))=1;
#endif

  // Lets make one big error message from the 3 we have been passed.
  char ErrorMessage[BUFFER_SIZE+1];
  strcpy(ErrorMessage,ErrorMessage1);
  if (ErrorMessage2!=NULL) {
    strcat(ErrorMessage,"|");
    strcat(ErrorMessage,ErrorMessage2);
  }
  if (ErrorMessage3!=NULL) {
    strcat(ErrorMessage,"|");
    strcat(ErrorMessage,ErrorMessage3);
  }

  // Find out what king of error we are going to display.
  if (ErrorType==WARNING && REPORT_WARNINGS==YES) {
#if GUI_ERRORS==YES
       MessageBox(0,ErrorMessage,"Warning...",MB_OK|MB_ICONWARNING);
#endif
    if (LOG_ERRORS==YES) {
      LogFile.open("error.log",ios::app);
      LogFile << "WARNING: [" << ErrorMessage << "] Continuing..." << endl;
      LogFile.close();
    }
    ERROR_STREAM << "WARNING: [" << ErrorMessage << "] Continuing..." << endl;
  }
  else {
#if GUI_ERRORS==YES
      MessageBox (0,ErrorMessage,"Error...",MB_OK|MB_ICONERROR);
#endif
    if (LOG_ERRORS==YES) {
      LogFile.open("error.log",ios::app);
      LogFile << "FATAL: [" << ErrorMessage << "] Stopping..." << endl;
      LogFile.close();
    }
    ERROR_STREAM << "FATAL: [" << ErrorMessage << "] Stopping..." << endl;
    exit(1);                             // Stopping with error.
  }

} // End Error.

#endif
