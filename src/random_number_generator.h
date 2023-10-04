// ###########################################################################
// #                      RANDOM NUMBER GENERATION CLASSES                   #
// ###########################################################################
// * This class uses the Numerical Recipies "Qucik and Dirty" method to
//   generate it's numbers. We now use it to generate a single 64 bit values
//   rather than two seperate 32 bit values like we had before...
// * If it can, then it now used "thread local storage" to store a seed for
//   each thread, but the way it's written it shouldn't go too badly wrong
//   if used without (it won't crash, but might repeat or skip numbers...).
// * We now don't have any way to seed the RNG as it would get really messy
//   doing this nicely (ie: we might think we have seeded it from main(),
//   only to not realize that each thread would have their own seed...).

#pragma once

#ifndef RANDOM_NUMBER_GENERATOR_H
#define RANDOM_NUMBER_GENERATOR_H

// ***************************************************************************
// *                                  INCLUDES                               *
// ***************************************************************************

//#include <Windows.h>			// For the GetCurrentThreadId() function.
#include <time.h>				// For the time() function the use as a seed.
#include <pthread.h>

// ***************************************************************************
// *                                  CONSTANTS                              *
// ***************************************************************************

// If we can, then lets use thread local storage.
// NOTE: The way the RNG works means it won't go "TOO" wrong without this...
#if 1
#define thread __thread
#define SEED_FUNCTION() (pthread_self()+time(0))	// Use thread ID.
#else
#define thread 
#define SEED_FUNCTION() time(0)							// Just use time.
#pragma message("RNG is NOT using Thread-Local-Storage...")
#endif

// ***************************************************************************
// *                   Random Number Generator Functions                     *
// ***************************************************************************

inline unsigned long long Rand64(void)
{	// Adpation of the "quick and dirty" to combine generate a 64bit value.
	// NOTE: We keep track of a seperate seed for each calling thread and use
	//       both the "Thread ID" and the time from time(0) to create the seed.

	// This is the 64bit seed (stored onces for each thread using TLS).
	static thread unsigned long long N=0ULL;	// Not seeded yet.

	// Do we need to seed for this thread?
	if (N==0ULL)
		N=SEED_FUNCTION();						// Will wrap: N MOD 2^64 anyway.

	// Lets get the next value in the progressions and replace the seed.
	N=((N*2862933555777941757ULL)+3037000493ULL);

	// Lets return the value.
	return N;

} // End Rand64.

// ===========================================================================

inline unsigned int Rand32(void)
{	// Adpation of the "quick and dirty" to combine generate a 32bit value.
	// NOTE: We keep track of a seperate seed for each calling thread and use
	//       both the "Thread ID" and the time from time(0) to create the seed.

	// This is the 32bit seed (stored onces for each thread using TLS).
	static thread unsigned int N;
	static thread bool Initialized=false;		// Not seeded yet.

	// Do we need to seed for this thread?
	if (Initialized==false) {
		N=SEED_FUNCTION();						// Will wrap: N MOD 2^32 anyway.
		Initialized=true;
	}

	// Lets get the next value in the progressions and replace the seed.
	N=((N*1664525U)+1013904223U);

	// Lets return the value.
	return N;

} // End Rand32.

// ===========================================================================

inline double Rand01(void)
{	// Lets return a value between 0 and 1.
	return (double)(Rand64() >> 11)*(1.0/9007199254740991.0);
	//return (double)(Rand64() >> 11)*(1.0/9007199254740992.0);
	//return ((double)(Rand64() >> 12)+ 0.5)*(1.0/4503599627370496.0);
	//return (double)Rand64()/(double)(0xFFFFFFFFFFFFFFFFULL);

	//return (double)Rand32()/(double)0xFFFFFFFFU;
} // End Rand01.

// ===========================================================================

inline unsigned int RandInt(unsigned int N)
{ 	// Returns a random unsinged Integer from 0 to N inclusive (upto 32bits).
	// NOTE: If Rand01() happens to return exactly 1.0 we get N+1, so round down.
	return Min((unsigned int)(Rand01()*(double)(N+1)),N);
} // End RandInt.

// ===========================================================================

#endif
