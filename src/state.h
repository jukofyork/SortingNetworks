// state.cc
// ========
// This file contains the class used to operate on and hold a single 'state' of
// vectors for doing non-determinic beam search with.
// The code originaly came from old/nd_search14.cc, but will be cleaned up here.

#ifndef STATE_CC
#define STATE_CC

// *****************************************************************************
// *                           LIST ELEMENT STRUCTURE                          *
// *****************************************************************************

typedef struct ListElement
{ // This structure is used to hold the singly linked list holding all of the
  // unsorted vectors.
  // Shorts are fast but can only be used for net sizes < 16.
#if NET_SIZE < 16
  unsigned InList :1;        // 1=Yes, 0=No (Like Vectors array).
  unsigned Vector :15;       // Vector (& Vector index) held.
  short Next;                // Index to next in list (-1 == End).
#else
  unsigned InList :1;        // 1=Yes, 0=No (Like Vectors array).
  unsigned Vector :31;       // Vector (& Vector index) held.
  int Next;                  // Index to next in list (-1 == End).
#endif
} ListElement; // End ListElement structure.

// *****************************************************************************
// *                                 STATE CLASS                               *
// *****************************************************************************

class State
{ // Used to hold a state in the sorting process.

  public:

  // These store the Unsorted vectors as both a O(1) lookup and a linked
  // list for traversal. (Note: Now stores the is active in the list in a
  //                            starnge, but faster way (uses bitfields).
  ListElement UsedList[NUM_TESTS];        // List of unsorted vectors.
  int FirstUsed;                          // Index to first used.
  int NumUnsorted;                        // Number of unsorted vectors.

  // This is used to store the operations used to get to this state.
  byte Operations[MAX_OPS][2];            // Operations used so far.
  int CurrentLevel;                       // How many ops done so far.

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  State()
  { // Default constructor.
  } // End.

  State(State &S)
  { // Copy constructor.
    *this=S;                               // Use overloaded operator.
  } // End.

  int operator=(State &S)
  { // Assingment operator.

    int I;

    // Copy the state.
    for (I=0;I<NUM_TESTS;I++)
      UsedList[I]=S.UsedList[I];           // Copy the list.

    // Copy the first indexs.
    FirstUsed=S.FirstUsed;

    NumUnsorted=S.NumUnsorted;             // Copy Number of Vectors Unsorted.

    // Copy the operation history.
    for (I=0;I<S.CurrentLevel;I++) {
      Operations[I][0]=S.Operations[I][0];
      Operations[I][1]=S.Operations[I][1];
    }
    CurrentLevel=S.CurrentLevel;

    return 0;

  } // End.

  // --------------------------------------------------------------------------

  void SetStartState(void);
  void UpdateState(int Op1,int Op2);
  void DoRandomTransition(void);
  void PrintState(void);
  int GetDepth(void);
  double ScoreState(void);
  int FindSuccessors(int SuccOps[NET_SIZE][NET_SIZE]);
  int Find(void);

}; // End.

// =============================================================================

void State::SetStartState(void)
{ // Sets the state to all (unsorted) binary vetcors.

  int I;

  // Init the lists to empty.
  FirstUsed=-1;

  // Create all using Shift/AND.
  for (I=0;I<NUM_TESTS;I++) {

    // See if its sorted.
    if (IsSorted[I]) {
      UsedList[I].InList=0;                // Not in list.
    }
    else {

      UsedList[I].InList=1;                // In list.

      // Add into the used list.
      UsedList[I].Next=FirstUsed;
      UsedList[I].Vector=I;
      FirstUsed=I;

    }

  }

  // There are always NET_SIZE+1 initialy sorted vectors.
  NumUnsorted=NUM_TESTS-(NET_SIZE+1);    // Initial state.

  // There are no opertions done yet.
  CurrentLevel=0;

} // End.

// --------------------------------------------------------------------------

void State::UpdateState(int Op1,int Op2)
{ // Update the current state using the opertion specified - Expects Op1<Op2.
  // We know that it is safe to update the state in place as the new updated
  // (target) vector can't be altered by the same operation being done on it!
  // This version uses the linked list reprisentation.

  int NextIndex;                        // Next in list to use.
  int Vector;                           // Current Vector.

  int UsedIndex,LastIndex=-1;

  // Go through all the transitions in the list (exactly (2^NumVectors)/4).
  for (UsedIndex=FirstUsed;UsedIndex!=-1;UsedIndex=NextIndex) {

    // Save the Next one to use after this one in the list.
    NextIndex=UsedList[UsedIndex].Next;

    // Get the source.
    Vector=UsedList[UsedIndex].Vector;

    // Only bother if the operation allowed (Don't bother with lookup table!).
    if (((Vector>>Op1)&1)==0 && ((Vector>>Op2)&1)==1) {

      // Clear the source.
      UsedList[Vector].InList=0;

      // Do the transition to find the output (No lookup table!).
      Vector|=(1<<Op1);
      Vector&=(~(1<<Op2));

      // Update the target.
      if (UsedList[Vector].InList==1 || IsSorted[Vector]) {

        // One less unsorted.
        NumUnsorted--;

        // Remove source from used list.
        if (LastIndex!=-1)
          UsedList[LastIndex].Next=NextIndex;
        else
          FirstUsed=NextIndex;

      }
      else {

        // Set as used.
        UsedList[Vector].InList=1;

        // Add into the used list (in this position as we know it's empty!).
        UsedList[UsedIndex].Vector=Vector;
        if (LastIndex!=-1)
          UsedList[LastIndex].Next=UsedIndex;
        else
          FirstUsed=UsedIndex;

        // Last is still active.
        LastIndex=UsedIndex;

      }

    }
    else {
      // Last is still active.
      LastIndex=UsedIndex;
    }

  }

  // Save the operation used.
  Operations[CurrentLevel][0]=Op1;
  Operations[CurrentLevel][1]=Op2;
  CurrentLevel++;                          // Now on next level.

  if (CurrentLevel>MAX_OPS) {
    cout << "Error." << endl;
    exit(1);
  }

} // End.

// --------------------------------------------------------------------------

void State::DoRandomTransition(void)
{ // Does a random transition on the current state.

  int RandIndex;                        // Index (Skipping) of Random Vector.
  int TrueIndex;                        // Real Index of chosen vector.
  int RandOp;                           // Which one to choose.
  int I,N;

  // Find a random vector index.
  RandIndex=RandInt(NumUnsorted-1);  // Pick a random vector index.
  for (I=FirstUsed,N=0;I!=-1;I=UsedList[I].Next,N++) {
    if (N==RandIndex) {
      TrueIndex=UsedList[I].Vector;
      break;
    }
  }

  // Pick one of the allowed ops to do.
  RandOp=RandInt(NumAllowedOps[TrueIndex]-1);     // Find an index.

  // Do the opertion chosen.
  UpdateState(AllowedOps[TrueIndex][RandOp][0],
              AllowedOps[TrueIndex][RandOp][1]);

} // End.

// --------------------------------------------------------------------------

void State::PrintState(void)
{ // Print the current state (Debugging...).

  int I;

  // Print each of the vector values.
  for (I=0;I<NUM_TESTS;I++) {
    cout << I << ':' << UsedList[I].InList << endl;
  }

  for (I=0;I<NUM_TESTS;I++) {
    cout << I << ':' << UsedList[I].Vector << " =>" << UsedList[I].Next
         << endl;//<< " <=" << UsedList[I].Prev << endl;
  }
  cout << FirstUsed << endl;

  cout << "Number Unsorted:" << NumUnsorted << endl << endl;

} // End.

// --------------------------------------------------------------------------

int State::GetDepth(void)
{ // Returns the depth (i.e. The number of parallel steps required) of the
  // sorting network.

  int I,J;

  // Used to update which have been used so far.
  int UsedOps[NET_SIZE];

  // Number of parallel sections used.
  int NumUsed=1;                            // Init to 1 as min of 1 sections.

  // First clear ready for putting 1's into.
  for (J=0;J<NET_SIZE;J++)
    UsedOps[J]=0;

  // For each operation test for when it is interdependant on lower opp.
  for (I=0;I<CurrentLevel;I++) {

     // See if we have this op already.
     if (UsedOps[Operations[I][0]]==1 || UsedOps[Operations[I][1]]==1) {

       // Clear ready for putting 1's into.
       for (J=0;J<NET_SIZE;J++)
         UsedOps[J]=0;

       // One more section used.
       NumUsed++;

     }

     // These are set at the begining of a new paralell section.
     UsedOps[Operations[I][0]]=1;
     UsedOps[Operations[I][1]]=1;
  }

  // Return the number of parallel sections used.
  return NumUsed;

} // End.

// --------------------------------------------------------------------------

double State::ScoreState(void)
{ // Returns the average score (in number of compartors needed) for state
  // to reach a terminal state (i.e. No more Unsorted vectors left).
  // NOTE: Destroys the input!

	// Keep static so .NET not go wrong (+Speed?).
	State* TempStatePtr=new State;                     // Used to do the testing on.

	// This holds the level and depth we founf for each test run.
	int Level[NUM_TEST_RUNS];
	int Depth[NUM_TEST_RUNS];

	// Do many tests for the state to get a good idea of score.
	for (int Test=0;Test<NUM_TEST_RUNS;Test++) {

		// Make a copy.
		*TempStatePtr=*this;                    // Make a copy to use.

		// Do while there are no more UnSorted vectors left.
		while (TempStatePtr->NumUnsorted>0)
			TempStatePtr->DoRandomTransition();   // Do a random transiton.

		// Save the values we found.
		Level[Test]=TempStatePtr->CurrentLevel;
		Depth[Test]=TempStatePtr->GetDepth();

	}

	delete TempStatePtr;

	// Lets sort into ascending order based on length using depth for ties.
	// NOTE: Bubble sort OK for small N.
	for (int I=0;I<NUM_TEST_RUNS-1;I++) {
		for (int J=I+1;J<NUM_TEST_RUNS;J++) {
			if (Level[I]>Level[J] || (Level[I]==Level[J] && Depth[I]>Depth[J])) {
				int Temp=Level[I];
				Level[I]=Level[J];
				Level[J]=Temp;
				Temp=Depth[I];
				Depth[I]=Depth[J];
				Depth[J]=Temp;
			}
		}
	}

	// Do we want to find the (outlier-free) mean?
	if (SCORE_USING_MEAN==true) {
		int LevelSum=0;
		int DepthSum=0;
		for (int Test=0;Test<(NUM_TEST_RUNS-NUM_TEST_MEAN_OUTLIERS);Test++) {
			LevelSum+=Level[Test];
			DepthSum+=Depth[Test];
		}
		if (USE_DEPTH==true)
			return (double)(LENGTH_WEIGHT*LevelSum)+((double)DepthSum/(double)(NUM_TEST_RUNS-NUM_TEST_MEAN_OUTLIERS));
		else
			return (double)(LENGTH_WEIGHT*LevelSum);
	}

	// We must want to use the median then.
	else {
		if (USE_DEPTH==true) {
			if (NUM_TEST_RUNS>=2 && NUM_TEST_RUNS%2==0)
				return 0.5*(double)((LENGTH_WEIGHT*(Level[NUM_TEST_RUNS/2]+Level[(NUM_TEST_RUNS/2)-1]))+(Depth[NUM_TEST_RUNS/2]+Depth[(NUM_TEST_RUNS/2)-1]));
			else
				return (double)((LENGTH_WEIGHT*Level[NUM_TEST_RUNS/2])+Depth[NUM_TEST_RUNS/2]);
		}
		else {
			if (NUM_TEST_RUNS>=2 && NUM_TEST_RUNS%2==0)
				return 0.5*(double)((LENGTH_WEIGHT*(Level[NUM_TEST_RUNS/2]+Level[(NUM_TEST_RUNS/2)-1])));
			else
				return (double)((LENGTH_WEIGHT*Level[NUM_TEST_RUNS/2]));
		}
	}

} // End.

// --------------------------------------------------------------------------

int State::FindSuccessors(int SuccOps[NET_SIZE][NET_SIZE])
{ // Finds ALL the Successors to the current state.
  // Returns false if no succesors found.

  int Allowed=0;

  int I,N1,N2;

  // Clear ready for putting the 1's into it.
  for (N1=0;N1<NET_SIZE-1;N1++) {
    for (N2=N1+1;N2<NET_SIZE;N2++) {
      SuccOps[N1][N2]=0;               // Clear it.
    }
  }

  // Try all active vectors.
  for (I=FirstUsed;I!=-1;I=UsedList[I].Next) {

    // Now find all opertion allowed on the vector.
    for (N1=0;N1<NET_SIZE-1;N1++) {
      for (N2=N1+1;N2<NET_SIZE;N2++) {
        if (((UsedList[I].Vector>>N1)&1)==0
            && ((UsedList[I].Vector>>N2)&1)==1) {
          SuccOps[N1][N2]=1;            // Is allowed.
        }
      }
    }

  }

  // Find the total number of successors.
  for (N1=0;N1<NET_SIZE-1;N1++) {
    for (N2=N1+1;N2<NET_SIZE;N2++) {
      if (SuccOps[N1][N2]==1)
        Allowed++;
    }
  }

  // Return the number allowed.
  return Allowed;

} // End.

#endif
