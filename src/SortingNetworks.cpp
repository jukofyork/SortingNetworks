// main.cc
// =======
// This file contains the code to run the non-deterministic beam search, used to
// find minimum length sorting networks.
// Was previously old/nd_search14.cc.
// new1: Have altered the BeamSearch() function so that it now just keeps a
//       beam of operation lists, rather than as beam of states.
//       This is much better on memory and also appears to be faster.
// new2: Have now removed the NUM_TEST_RUNS, ie: many samples per beam element.
//       It appears that this was pointless and even made the code run worse.
//       Have found that using depth as the secondary fitness/score is useful
//       for larger sorting networks, this does not mean it's better for ones
//       with a 'different' length as fits to the pattern.
// new3: This version is pretty much the same as new2, but haven't touched the
//       code for nearly a year - so just to be safe!
//       This is the code to be run on the new (slow) machine...
// new5: Have added ability to use mean, mean ignoring outliers and median.

// Settings found from old/nd_search14.cc:
//   Using: NUM_TESTS=4 / MAX_BEAM_SIZE=100, it was possible to get 5 minimum
//          equalling nets for netsize 12 (ie. 39), 5 runs out of 5!
//   Also: NUM_TESTS=6 / MAX_BEAM_SIZE=150, it was possible to get 3 minimum
//         equalling nets for netsize 13 (ie. 45), 3 runs out of 3!
//   Also: NUM_TESTS=9 / MAX_BEAM_SIZE=225, it was possible to get 4 minimum
//         equalling nets for netsize 14 (ie. 51), 4 runs out of 4!
//   Also: NUM_TESTS=14 / MAX_BEAM_SIZE=338, it was possible to get 1 minimum
//         equalling net for netsize 15 (ie. 56), 1 run out of 1!

using namespace std;

// Maximum number of iterations to run for.
#define MAX_ITERS				1000000					// Ctrl-C will exit anyway.

// Number of elements to sort.
#define NET_SIZE				16

// Number of states in beam ( < 2^16 ?).
#define MAX_BEAM_SIZE			1000

// Number of tests per score run ( < 10 ?).
#define NUM_TEST_RUNS			10 //12

// The number of "elites" we take the average of to get the scores.
#define NUM_TEST_RUN_ELITES		1 //10 						// Must be in the range [1,NUM_TEST_RUNS].

// This weight we put on minimising the depth.
// NOTE: If DEPTH_WEIGHT < 0.5 we assume we are optimizing the length, else optimizing the depth.
#define DEPTH_WEIGHT			0.0001

// Set this to use the asymmetry heuristic.
#define USE_ASYMMETRY_HEURISTIC	true                  // Only works well for even N nets.

// The target size of the network should be 1 less than best known.
// URL: https://bertdobbelaere.github.io/sorting_networks.html
#if NET_SIZE == 8
#define LENGTH_UPPER_BOUND 19
#define DEPTH_UPPER_BOUND  6
#endif
#if NET_SIZE == 9
#define LENGTH_UPPER_BOUND 25
#define DEPTH_UPPER_BOUND  7
#endif
#if NET_SIZE == 10
#define LENGTH_UPPER_BOUND 29
#define DEPTH_UPPER_BOUND  7
#endif
#if NET_SIZE == 11
#define LENGTH_UPPER_BOUND 35
#define DEPTH_UPPER_BOUND  8
#endif
#if NET_SIZE == 12
#define LENGTH_UPPER_BOUND 39
#define DEPTH_UPPER_BOUND  8
#endif
#if NET_SIZE == 13
#define LENGTH_UPPER_BOUND 45
#define DEPTH_UPPER_BOUND  9
#endif
#if NET_SIZE == 14
#define LENGTH_UPPER_BOUND 51
#define DEPTH_UPPER_BOUND  9
#endif
#if NET_SIZE == 15
#define LENGTH_UPPER_BOUND 56
#define DEPTH_UPPER_BOUND  9
#endif
#if NET_SIZE == 16
#define LENGTH_UPPER_BOUND 60
#define DEPTH_UPPER_BOUND  9
#endif
#if NET_SIZE == 17
#define LENGTH_UPPER_BOUND 71
#define DEPTH_UPPER_BOUND  10
#endif
#if NET_SIZE == 18
#define LENGTH_UPPER_BOUND 77
#define DEPTH_UPPER_BOUND  11
#endif
#if NET_SIZE == 19
#define LENGTH_UPPER_BOUND 85
#define DEPTH_UPPER_BOUND  11
#endif
#if NET_SIZE == 20
#define LENGTH_UPPER_BOUND 91
#define DEPTH_UPPER_BOUND  11
#endif
#if NET_SIZE == 21
#define LENGTH_UPPER_BOUND 99
#define DEPTH_UPPER_BOUND  12
#endif
#if NET_SIZE == 22
#define LENGTH_UPPER_BOUND 106
#define DEPTH_UPPER_BOUND  12
#endif
#if NET_SIZE == 23
#define LENGTH_UPPER_BOUND 114
#define DEPTH_UPPER_BOUND  12
#endif
#if NET_SIZE == 24
#define LENGTH_UPPER_BOUND 120
#define DEPTH_UPPER_BOUND  12
#endif

// Maximum operations to store - overflow error will occur if this is exceeded.
#define MAX_OPS					(LENGTH_UPPER_BOUND*2)

// Basic parameters created from above.
#define BRANCHING_FACTOR		((NET_SIZE*(NET_SIZE-1))/2) // Of first level!
#define NUM_TESTS				(1<<NET_SIZE)				// Test size (Zero/One theorem...)

// =============================================================================

#include <iostream>                           	// For C++ stdio.
#include <time.h>                               // For clock function.
#include <signal.h>                             // For catching SIGINT.
#include <sys/resource.h>						// setpriority()

// Own libraries.
#include "misc.h"                               // Misc stuff.
#include "random_number_generator.h"            // Random number generator.

// *****************************************************************************
// *                                 STRUCTURES                                *
// *****************************************************************************

typedef struct StateSuccessor { // This is used for the beam search on the set of states.
// It is then sorted by qsort.
	word BeamIndex;         // Index in beam of states.
	byte Op1, Op2;
	double Score;             // (Heuristically) Predicted length of sorting net.
} StateSuccessor; // End StateSuccessor structure.

typedef struct Operation { // Stores a single operation (in one byte).
	byte Op1, Op2;
} Operation; // End Operation structure.

// =============================================================================

// The beam of states and temp copy to swap into.
Operation Beam[MAX_BEAM_SIZE][MAX_OPS];
int CurrentBeamSize;

Operation TempBeam[MAX_BEAM_SIZE][MAX_OPS];

// The successors found for all of the states in the beam.
StateSuccessor BeamSucc[MAX_BEAM_SIZE * BRANCHING_FACTOR];
int NumBeamSucc;

// =============================================================================

// Signal handling.
int ExitFlag = 0;      // Start as 0 - Run while 0.

void Signal_INT(int SigNum) { // Signal handler for SIGINT from user.

	// Exit if pressed a second time.
	if (ExitFlag == 1)
		exit(1);

	ExitFlag = 1;        // Tell main loop to exit and then tidy up after.

	// Re-Set up the signal handler, so we only stop after a full iter!.
	signal(SIGINT, Signal_INT);

} // End SIGINT Handler.

// =============================================================================

// The rest of the code needed to do the search.
#include "lookup.h"                            // Lookup tables used.
#include "state.h"                             // The state class.
#include "search.h"                            // The beam search code.

// =============================================================================

int main(void) {

	// Made static bc .NET messed up with too much on stack.
	static State S;

	// For timing.
	clock_t StartTime = clock();

	// Set up the signal handler.
	signal(SIGINT, Signal_INT);

	// Lets make ourself run at lowest priority.
	setpriority(PRIO_PROCESS, 0, 19);

	// Init the lookup tables.
	InitLookups();

	cout << "NET_SIZE                = " << NET_SIZE << endl;
	cout << "MAX_BEAM_SIZE           = " << MAX_BEAM_SIZE << endl;
	cout << "NUM_TEST_RUNS           = " << NUM_TEST_RUNS << endl;
	cout << "NUM_TEST_RUN_ELITES     = " << NUM_TEST_RUN_ELITES << endl;
	cout << "USE_ASYMMETRY_HEURISTIC = " << (USE_ASYMMETRY_HEURISTIC==true?"Yes":"No") << endl;
	cout << "DEPTH_WEIGHT            = " << DEPTH_WEIGHT << endl;
	cout << "LENGTH_UPPER_BOUND      = " << LENGTH_UPPER_BOUND << endl;
	cout << "DEPTH_UPPER_BOUND       = " << DEPTH_UPPER_BOUND << endl;
	cout << endl;

	int Iter;
	for (Iter = 0; Iter < MAX_ITERS && ExitFlag == 0; Iter++) {

		// Do the Beam search.
		cout << "Iteration " << (Iter + 1) << ':' << endl;
		int Length = BeamSearch(S);

		//cout << "Before: " << S.GetDepth() << endl;
		S.MinimiseDepth();
		//cout << "After: " << S.GetDepth() << endl;

		int Depth = S.GetDepth();

		// Print the operations.
		for (int I = 0; I < S.CurrentLevel; I++) {
			cout << '+' << (I + 1) << ":(" << (int) S.Operations[I][0] << ','
					<< (int) S.Operations[I][1] << ")" << endl;
		}

		// Print depth at end.
		cout << "+Length: " << Length << endl;
		cout << "+Depth : " << Depth << endl;
		cout << endl;

		// Exit if we have found a new upper bound.
		if (Length < LENGTH_UPPER_BOUND || Depth < DEPTH_UPPER_BOUND) {
			Iter++;
			break;
		}

	}

	cout << "Total Iterations  : " << Iter << endl;
	cout << "Total Time        : "
			<< (double) (clock() - StartTime) / (double) CLOCKS_PER_SEC
			<< " seconds" << endl;

	return 0;

} // End main.

