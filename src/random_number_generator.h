// ###########################################################################
// #                      RANDOM NUMBER GENERATION CLASSES                   #
// ###########################################################################
// * This class uses the Numerical Recipes "Quick and Dirty" method to
//   generate it's numbers. We now use it to generate a single 64 bit values
//   rather than two separate 32 bit values like we had before...
// * If it can, then it now used "thread local storage" to store a seed for
//   each thread, but the way it's written it shouldn't go too badly wrong
//   if used without (it won't crash, but might repeat or skip numbers...).
// * We now don't have any way to seed the RNG as it would get really messy
//   doing this nicely (ie: we might think we have seeded it from main(),
//   only to not realize that each thread would have their own seed...).

#pragma once

#ifndef RANDOM_NUMBER_GENERATOR_H
#define RANDOM_NUMBER_GENERATOR_H

using namespace std;

// ***************************************************************************
// *                                  INCLUDES                               *
// ***************************************************************************

#include <pthread.h>			// For pthread_self() function.
#include <time.h>				// For the time() function the use as a seed.

// ***************************************************************************
// *                                  CONSTANTS                              *
// ***************************************************************************

#define SEED_FUNCTION() (pthread_self()+time(0))	// Use thread ID.

// ***************************************************************************
// *                   Random Number Generator Functions                     *
// ***************************************************************************

inline unsigned long long Rand64(void) {// Adaption of the "quick and dirty" to combine generate a 64bit value.
										// NOTE: We keep track of a separate seed for each calling thread and use
										//       both the "Thread ID" and the time from time(0) to create the seed.

	// This is the 64bit seed (stored onces for each thread using TLS).
	static __thread unsigned long long N = 0ULL;	// Not seeded yet.

	// Do we need to seed for this thread?
	if (N == 0ULL)
		N = SEED_FUNCTION();					// Will wrap: N MOD 2^64 anyway.

	// Lets get the next value in the progressions and replace the seed.
	N = ((N * 2862933555777941757ULL) + 3037000493ULL);

	// Lets return the value.
	return N;

} // End Rand64.

// ===========================================================================

inline unsigned int Rand32(void) {// Adaption of the "quick and dirty" to combine generate a 32bit value.
								  // NOTE: We keep track of a separate seed for each calling thread and use
								  //       both the "Thread ID" and the time from time(0) to create the seed.

	// This is the 32bit seed (stored once for each thread using TLS).
	static __thread unsigned int N;
	static __thread bool Initialized = false;		// Not seeded yet.

	// Do we need to seed for this thread?
	if (Initialized == false) {
		N = SEED_FUNCTION();					// Will wrap: N MOD 2^32 anyway.
		Initialized = true;
	}

	// Lets get the next value in the progressions and replace the seed.
	N = ((N * 1664525U) + 1013904223U);

	// Lets return the value.
	return N;

} // End Rand32.

// ===========================================================================

inline double Rand01(void) {	// Lets return a value between 0 and 1.
	return (double) (Rand64() >> 11) * (1.0 / 9007199254740991.0);
} // End Rand01.

// ===========================================================================

inline unsigned int RandInt(unsigned int N) { // Returns a random unsigned Integer from 0 to N inclusive (up to 32bits).
											  // NOTE: If Rand01() happens to return exactly 1.0 we get N+1, so round down.
	return Min((unsigned int) (Rand01() * (double) (N + 1)), N);
} // End RandInt.

// ===========================================================================

#endif
