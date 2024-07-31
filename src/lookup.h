// lookup.h
// ========
// This file contains the various lookup tables used and the code to generate 
// them all.

#pragma once

#ifndef LOOKUP_H
#define LOOKUP_H

using namespace std;

// ***************************************************************************
// *                                  INCLUDES                               *
// ***************************************************************************

// *****************************************************************************
// *                            GLOBAL LOOKUP TABLES                           *
// *****************************************************************************

// This is used so we can say instantly if a vector (in n-bit int) is sorted.
bool IsSorted[NUM_TESTS];							// YES or NO.

// For checking which ops are allowed for RandomTransition() function.
BYTE AllowedOps[NUM_TESTS][BRANCHING_FACTOR][2];	// Source, Target.
int NumAllowedOps[NUM_TESTS];						// Number of allowed operations.

// *****************************************************************************
// *                       LOOKUP TABLE GENERATION FUNCTIONS                   *
// *****************************************************************************

void CreateIsSorted(void) { // Sets up the IsSorted vector so we can test for if sorted in 1 operation
							// rather than NetSize-1 in a loop.

	int V, I;

	// For all vectors.
	for (V = 0; V < NUM_TESTS; V++) {

		// Simply test for one out-of-order.
		for (I = 0;; I++) {
			if (I == (NET_SIZE - 1)) {
				IsSorted[V] = true;					// Is Sorted.
				break;
			}
			if (((V >> I) & 1) == 0 && ((V >> (I + 1)) & 1) == 1) { // REMEMBER: Highest First!
				IsSorted[V] = false;				// Is NOT Sorted.
				break;
			}
		}

	}

} // End CreateIsSorted.

// -----------------------------------------------------------------------------

void CreateAllowedOps(void) { // Creates tables used to check which ops are allowed for RandomTransition()
							  // function.

	int I, N1, N2;

	// For each vector find the list.
	for (I = 0; I < NUM_TESTS; I++) {

		// Clear ready for adding to.
		NumAllowedOps[I] = 0;

		// Now find all operation allowed on the vector.
		for (N1 = 0; N1 < NET_SIZE - 1; N1++) {
			for (N2 = N1 + 1; N2 < NET_SIZE; N2++) {
				if (((I >> N1) & 1) == 0 && ((I >> N2) & 1) == 1) {
					AllowedOps[I][NumAllowedOps[I]][0] = N1; // Source.
					AllowedOps[I][NumAllowedOps[I]][1] = N2; // Target.
					NumAllowedOps[I]++;                    // One more allowed.
				}
			}
		}

	}

} // End CreateAllowedOps.

// -----------------------------------------------------------------------------

void InitLookups(void) { // Creates the lookup tables needed by State class.

	// Call the function to create the sorted vectors.
	CreateIsSorted();

	// Create the lists of allowed ops.
	CreateAllowedOps();

} // End InitLookups.

#endif
