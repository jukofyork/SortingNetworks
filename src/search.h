// search.h
// ========
// This file contains the code used to do the beam search.

#pragma once

#ifndef SEARCH_H
#define SEARCH_H

using namespace std;

// ***************************************************************************
// *                                  INCLUDES                               *
// ***************************************************************************

#include <iostream>
#include <stdlib.h>                    // For qsort() function.

// *****************************************************************************
// *                              BEAM SEARCH CODE                             *
// *****************************************************************************

int CompSucc(const void *E1, const void *E2) { // For qsort.
	if (((StateSuccessor*) E1)->Score < ((StateSuccessor*) E2)->Score)
		return -1;
	if (((StateSuccessor*) E1)->Score > ((StateSuccessor*) E2)->Score)
		return 1;
	return 0;
} // End CompSucc.

// --------------------------------------------------------------------------

int BeamSearch(State &Result) { // Finds a sorting network by slowly reducing the depth of the tree
								// using a Non-Deterministic (Topological) Search.
								// Returns the level found.

	int I;

	static State BeamState;                      // Used to make copy from beam.

	int SuccOps[NET_SIZE][NET_SIZE];       // List of operations allowed.
	int NumSuccs;                         // Number of successors.

	// Do until one is found.
	for (int Level = 0;; Level++) {

		cout << Level << ' ';
		cout.flush();

		// For all states in the beam.
		NumBeamSucc = 0;
		for (I = 0; I < BEAM_SIZE; I++) {

			// Make the state to use.
			BeamState.SetStartState();
			for (int J = 0; J < Level; J++)
				BeamState.UpdateState(Beam[I][J].Op1, Beam[I][J].Op2);

			// Find all the active successors for the whole set of vectors.
			// Stop when no more successors to the moved-into state.
			NumSuccs = BeamState.FindSuccessors(SuccOps);
			if (NumSuccs == 0) {
				cout << endl;
				Result = BeamState;
				return Result.CurrentLevel;
			}

			// If we can simply do the 'inverse' opperation to the last, then do it.
			// ### THIS WAS ALLREADY DONE IN THE OLD CODE - IN LINUX!!!! ###
			int N1, N2;
			if (USE_ASYMITRY_HEURISTIC == true && Level >= 1) {
				N1 = Beam[I][Level - 1].Op1;
				N2 = Beam[I][Level - 1].Op2;
			}
			if (USE_ASYMITRY_HEURISTIC == true && Level >= 1
					&& N1 != (NET_SIZE - 1) - N1 && N1 != (NET_SIZE - 1) - N2
					&& N2 != (NET_SIZE - 1) - N1 && N2 != (NET_SIZE - 1) - N2
					&& SuccOps[(NET_SIZE - 1) - N2][(NET_SIZE - 1) - N1] == 1) {

				// Make a copy.
				State *TempStatePtr = new State;
				*TempStatePtr = BeamState;
				TempStatePtr->UpdateState((NET_SIZE - 1) - N2,
						(NET_SIZE - 1) - N1);  // Update it.

				// Score it.
				double score = TempStatePtr->ScoreState();

				delete TempStatePtr;

				// Update the successors.
				BeamSucc[NumBeamSucc].BeamIndex = I;
				BeamSucc[NumBeamSucc].Op1 = (NET_SIZE - 1) - N2;
				BeamSucc[NumBeamSucc].Op2 = (NET_SIZE - 1) - N1;
				BeamSucc[NumBeamSucc].Score = score;
				NumBeamSucc++;
			}

			// Just do normal then.
			else {

				// For each of the living successors get a score using ND search.
				// URL: https://stackoverflow.com/questions/47758947/c-openmp-nested-loops-where-the-inner-iterator-depends-on-the-outer-one
				//for (N1=0;N1<NET_SIZE-1;N1++) {
				//	for (N2=N1+1;N2<NET_SIZE;N2++) {
				#pragma omp parallel for
				for (int k = 0; k < NET_SIZE * (NET_SIZE - 1) / 2; k++) {

					// Get N1 and N2.
					int N1, N2;
					N1 = k / NET_SIZE;
					N2 = k % NET_SIZE;
					if (N2 <= N1) {
						N1 = NET_SIZE - N1 - 2;
						N2 = NET_SIZE - N2 - 1;
					}

					// If the operation is allowed, then find how well it does.
					if (SuccOps[N1][N2] == 1) {

						// Make a copy.
						State *TempStatePtr = new State;
						*TempStatePtr = BeamState;
						TempStatePtr->UpdateState(N1, N2);     // Update it.

						// Score it.
						double score = TempStatePtr->ScoreState();

						delete TempStatePtr;

						// Update the successors.
						#pragma omp critical
						{
							BeamSucc[NumBeamSucc].BeamIndex = I;
							BeamSucc[NumBeamSucc].Op1 = N1;
							BeamSucc[NumBeamSucc].Op2 = N2;
							BeamSucc[NumBeamSucc].Score = score;
							NumBeamSucc++;
						}

					}

				}

			}

		}

		// Sort the list of successors.
		qsort(BeamSucc, NumBeamSucc, sizeof(StateSuccessor), CompSucc);

		// Create the new set of states.
		for (I = 0; I < BEAM_SIZE; I++) {
			for (int J = 0; J < Level; J++) {
				TempBeam[I][J] = Beam[BeamSucc[I].BeamIndex][J];
			}
			TempBeam[I][Level].Op1 = BeamSucc[I].Op1;
			TempBeam[I][Level].Op2 = BeamSucc[I].Op2;
		}
		for (I = 0; I < BEAM_SIZE; I++) {
			for (int J = 0; J < (Level + 1); J++) {
				Beam[I][J] = TempBeam[I][J];
			}
		}

	}

} // End.

#endif
