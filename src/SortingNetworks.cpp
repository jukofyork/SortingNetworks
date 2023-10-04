// main.cc
// =======
// This file contains the code to run the non-deterministic beam search, used to
// find minimum length sorting networks.
// Was previously old/nd_search14.cc.
// new1: Have altered the BeamSearch() function so that it now just keeps a
//       beam of operation lists, rather than as beam of states.
//       This is much better on memory and also apears to be faster.
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
//   Using: NUM_TESTS=4 / BEAM_SIZE=100, it was possible to get 5 minimum
//          equaling nets for netsize 12 (ie. 39), 5 runs out of 5!
//   Also: NUM_TESTS=6 / BEAM_SIZE=150, it was possible to get 3 minimum
//         equaling nets for netsize 13 (ie. 45), 3 runs out of 3!
//   Also: NUM_TESTS=9 / BEAM_SIZE=225, it was possible to get 4 minimum
//         equaling nets for netsize 14 (ie. 51), 4 runs out of 4!
//   Also: NUM_TESTS=14 / BEAM_SIZE=338, it was possible to get 1 minimum
//         equaling net for netsize 15 (ie. 56), 1 run out of 1!

// Number of elements to sort.
#define NET_SIZE				18

// Number of states in beam ( < 2^16 ?).
#define BEAM_SIZE				1000 //500

// Number of tests per score run ( < 10 ?).
#define NUM_TEST_RUNS			12

// Set this to use the () mean, else we will use the meadian.
#define SCORE_USING_MEAN		true

// If we choose to use mean to score, then we will ignore upto this many outliers.
#define NUM_TEST_MEAN_OUTLIERS	2							// Must be < NUM_TEST_RUNS, can be 0.

// Set this to use the asymetry heuristic.
#define USE_ASYMITRY_HEURISTIC	false					// Only works well for even N nets.

// Set this to use depth at a tie breaker for scoreing.
#define USE_DEPTH				true

// The target size of the netork should be 1 less than best known.
// URL: https://bertdobbelaere.github.io/sorting_networks.html
#if NET_SIZE == 8
#define WANTED 18
#endif
#if NET_SIZE == 9
#define WANTED 24
#endif
#if NET_SIZE == 10
#define WANTED 28
#endif
#if NET_SIZE == 11
#define WANTED 34
#endif
#if NET_SIZE == 12
#define WANTED 38
#endif
#if NET_SIZE == 13
#define WANTED 44
#endif
#if NET_SIZE == 14
#define WANTED 50
#endif
#if NET_SIZE == 15
#define WANTED 55
#endif
#if NET_SIZE == 16
#define WANTED 59
#endif
#if NET_SIZE == 17
#define WANTED 70
#endif
#if NET_SIZE == 18
#define WANTED 76
#endif
#if NET_SIZE == 19
#define WANTED 84
#endif
#if NET_SIZE == 20
#define WANTED 90
#endif
#if NET_SIZE == 21
#define WANTED 98
#endif
#if NET_SIZE == 22
#define WANTED 105
#endif
#if NET_SIZE == 23
#define WANTED 113
#endif
#if NET_SIZE == 24
#define WANTED 119
#endif

// Maximum number of iterations to run for.
#define MAX_ITERS			1000000						// Ctrl-C will exit anyway.

// Basic paramters created from above.
#define BRANCHING_FACTOR	((NET_SIZE*(NET_SIZE-1))/2) // Of first level!
#define NUM_TESTS			(1<<NET_SIZE)				// Test sise (Zero/One theorm...)

// Maximum operations to store - overflow error will occur if this is exceeded.
#define MAX_OPS				(WANTED*2)

// This should be bigger than 2x the expected depth (for median needs 2x).
#define LENGTH_WEIGHT		100

// =============================================================================

#include <iostream>                           // For C++ stdio.
#include <time.h>                               // For clock function.
#include <signal.h>                             // For catching SIGINT.

// Own librarys.
#include "misc.h"                               // Misc stuff.
#include "random_number_generator.h"            // Random number generator.

// *****************************************************************************
// *                                 STRUCTURES                                *
// *****************************************************************************

typedef struct StateSuccessor
{ // This is used for the beam search on the set of states.
  // It is then sorted by qsort.
  word     BeamIndex;         // Index in beam of states.
  byte     Op1,Op2;
  double   Score;             // (Heuristicaly) Predicted length of sorting net.
} StateSuccessor; // End StateSuccessor structure.

typedef struct Operation
{ // Stores a single operation (in one byte).
  byte Op1,Op2;
} Operation; // End Operation structure.

// =============================================================================

// The beam of states and temp copy to swap into.
Operation Beam[BEAM_SIZE][MAX_OPS];
Operation TempBeam[BEAM_SIZE][MAX_OPS];

// The successors found for all of the states in the beam.
StateSuccessor BeamSucc[BEAM_SIZE*BRANCHING_FACTOR];
int NumBeamSucc;

// =============================================================================

// Signal handleing.
int ExitFlag=0;      // Start as 0 - Run while 0.

void Signal_INT(int SigNum)
{ // Signal handeler for SIGINT from user.
  //exit(1);

  ExitFlag=1;        // Tell main loop to exit and then tidy up after.

  // Re-Set up the signal handeler, so we only stop after a full iter!.
  signal(SIGINT,Signal_INT);

} // End SIGINT Handeler.

// =============================================================================

// The rest of the code needed to do the search.
#include "lookup.h"                            // Lookup tables used.
#include "state.h"                             // The state class.
#include "search.h"                            // The beam search code.

// =============================================================================

int main(void)
{

  // Made static bc .NET messed up with too much on stack.
  static State S;

  // For timing.
  clock_t StartTime=clock();

  int Iter;

  int Level;
  int MeanLevel=0;
  int BestLevel=MAX_OPS;
  int WorstLevel=0;

  // Init the lookup tables.
  InitLookups();

  // Set up the signal handeler.
  signal(SIGINT,Signal_INT);

  // NEW (2008): Lets make ourself run at bellow normal priority.
  //SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS);

  cout << "NET_SIZE               = " << NET_SIZE << endl;
  cout << "BEAM_SIZE              = " << BEAM_SIZE << endl;
  cout << "NUM_TEST_RUNS          = " << NUM_TEST_RUNS << endl;
  cout << "SCORE_USING            = " << (SCORE_USING_MEAN==true?"Mean":"Median") << endl;
  if (SCORE_USING_MEAN==true)
    cout << "NUM_TEST_MEAN_OUTLIERS = " << NUM_TEST_MEAN_OUTLIERS << endl;
  cout << "USE_ASYMITRY_HEURISTIC = " << (USE_ASYMITRY_HEURISTIC==true?"Yes":"No") << endl;
  cout << "USE_DEPTH              = " << (USE_DEPTH==true?"Yes":"No") << endl;
  cout << "WANTED                 = " << WANTED << endl;
  cout << endl;

  for (Iter=0;Iter<MAX_ITERS && ExitFlag==0;Iter++) {

	// Do the Beamsearch.
    cout << "Iteration " << (Iter+1) << ':' << endl;
    Level=BeamSearch(S);

	// Print the operations.
    for (int I=0;I<S.CurrentLevel;I++) {
      cout << '+' << (I+1) << ":(" << (int) S.Operations[I][0]
           << ','
           << (int) S.Operations[I][1] << ")" << endl;
    }

    // Print depth at end.
    cout << "+Length: " << S.CurrentLevel << endl;
    cout << "+Depth : " << S.GetDepth() << endl;

    cout << endl;

    // Update stats.
    MeanLevel+=Level;
    if (Level<BestLevel)
      BestLevel=Level;
    if (Level>WorstLevel)
      WorstLevel=Level;

    if (Level<=WANTED) {
      Iter++;
      break;
    }

  }

  cout << "Total Iters  : " << Iter << endl;
  cout << "Total Time   : " << (double)(clock()-StartTime)
                              /(double)CLOCKS_PER_SEC
                           << " seconds" << endl;
  cout << "Wanted Level : " << WANTED << endl;
  cout << "Mean Level   : " << ((double)MeanLevel/(double)Iter) << endl;
  cout << "Best Level   : " << BestLevel << endl;
  cout << "Worst Level  : " << WorstLevel << endl;

  return 0;

} // End main.

