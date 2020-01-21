// Filename:	logic_sim.cpp
// Author:		Alex Nolan
// Date:		10/4/2018
// Description:	A modified version of the three value logic simulator
//				by Michael Hsiao, for the gate implication simulator

///////////////////////////////////////////////////////////////////////
//   Logic Simulator, written by Michael Hsiao
//   Began in 1992, revisions and additions of functions till 2013
///////////////////////////////////////////////////////////////////////

#include "logic_sim.h"

using namespace std;

////////////////////////////////////////////////////////////////////////
inline void LogicSim::insertEvent(int levelN, int gateN)
{
    levelEvents[levelN][levelLen[levelN]] = gateN;
    levelLen[levelN]++;
}

////////////////////////////////////////////////////////////////////////
// LogicSim class
////////////////////////////////////////////////////////////////////////

//default constructor, exits
LogicSim::LogicSim()
{
	exit(-1);
}

// constructor: reads in the *.lev file for the gate-level ckt
LogicSim::LogicSim(string cktName)
{
	//set initial values
	x_number = 4;			//distinguishing X starting value
	TIES = new int[512];	//initialize array for ties
	INIT0 = 0;				//don't initialize FF's
	fixedNodeCounter = 0;
	
	numSimulations = 0;
	numIndirectImplications = 0;

    ifstream yyin;
    string fName;
    int i, j, count;
    char c;
    int netnum, junk;
    int f1, f2, f3;
    int levelSize[MAXlevels];

    fName = cktName + ".lev";
    yyin.open(fName.c_str(), ios::in);
    if (!yyin)
    {
	cerr << "Can't open .lev file\n";
	exit(-1);
    }

    numpri = numgates = numout = maxlevels = numff = 0;
    maxLevelSize = 32;
    for (i=0; i<MAXlevels; i++)
	levelSize[i] = 0;

    yyin >>  count;	// number of gates
    yyin >> junk;

    // allocate space for gates
    gtype = new unsigned char[count+64];
    fanin = new short[count+64];
    fanout = new short[count+64];
    levelNum = new int[count+64];
    po = new unsigned[count+64];
    inlist = new int * [count+64];
    fnlist = new int * [count+64];
    sched = new char[count+64];

	//instantiate array for gate values
    GateValues = new unsigned int[count+64];
	OrigGateValues = new unsigned int[count + 64];

	//instantiate implication list arrays
	zeroList = new ImplicationList[count + 64];
	oneList = new ImplicationList[count + 64];

    // now read in the circuit
    numTieNodes = 0;
    for (i=1; i<count; i++)
    {
	yyin >> netnum;
	yyin >> f1;
	yyin >> f2;
	yyin >> f3;

	numgates++;
	gtype[netnum] = (unsigned char) f1;
	f2 = (int) f2;
	levelNum[netnum] = f2;
	levelSize[f2]++;

	if (f2 >= (maxlevels))
	    maxlevels = f2 + 5;
	if (maxlevels > MAXlevels)
	{
	    cerr << "MAXIMUM level (" << maxlevels << ") exceeded.\n";
	    exit(-1);
	}

	fanin[netnum] = (int) f3;
	if (f3 > MAXFanout)
	{
		cerr << "Fanin count (" << f3 << " exceeded\n";
		exit(-1);
	}
		
	if (gtype[netnum] == T_input)
	{
	    inputs[numpri] = netnum;
	    numpri++;
	}
	if (gtype[netnum] == T_dff)
	{
	    if (numff >= (MAXFFS-1))
	    {
		cerr << "The circuit has more than " << MAXFFS -1 << " FFs\n";
		exit(-1);
	    }
	    ff_list[numff] = netnum;
	    numff++;
	}

	sched[netnum] = 0;

	// now read in the faninlist
	inlist[netnum] = new int[fanin[netnum]];
	for (j=0; j<fanin[netnum]; j++)
	{
	    yyin >> f1;
	    inlist[netnum][j] = (int) f1;
	}

	for (j=0; j<fanin[netnum]; j++)	  // followed by close to samethings
	    yyin >> junk;

	// read in the fanout list
	yyin >> f1;
	fanout[netnum] = (int) f1;

	if (gtype[netnum] == T_output)
	{
	    po[netnum] = TRUE;
	    outputs[numout] = netnum;
	    numout++;
	}
	else
	    po[netnum] = 0;

	if (fanout[netnum] > MAXFanout)
	    cerr << "Fanout count (" << fanout[netnum] << ") exceeded\n";

	fnlist[netnum] = new int[fanout[netnum]];
	for (j=0; j<fanout[netnum]; j++)
	{
	    yyin >> f1;
	    fnlist[netnum][j] = (int) f1;
	}

	if (gtype[netnum] == T_tie1)
        {
	    TIES[numTieNodes] = netnum;
	    numTieNodes++;
	    if (numTieNodes > 511)
	    {
		cerr, "Can't handle more than 512 tied nodes\n";
		exit(-1);
	    }
			GateValues[netnum] = 1;
        }
        else if (gtype[netnum] == T_tie0)
        {
	    TIES[numTieNodes] = netnum;
	    numTieNodes++;
	    if (numTieNodes > 511)
	    {
		cerr << "Can't handle more than 512 tied nodes\n";
		exit(-1);
	    }
			GateValues[netnum] = 0;
        }
        else
        {
	    // assign all values to unknown (unassigned x)
	    GateValues[netnum] = ALLONES;
	}

	// read in and discard the observability values
        yyin >> junk;
        yyin >> c;    // some character here
        yyin >> junk;
        yyin >> junk;

    }	// for (i...)
    yyin.close();
    numgates++;
    numFaultFreeGates = numgates;

    // now compute the maximum width of the level
    for (i=0; i<maxlevels; i++)
    {
	if (levelSize[i] > maxLevelSize)
	    maxLevelSize = levelSize[i] + 1;
    }

    // allocate space for the faulty gates
    for (i = numgates; i < numgates+64; i+=2)
    {
        inlist[i] = new int[2];
        fnlist[i] = new int[MAXFanout];
        po[i] = 0;
        fanin[i] = 2;
        inlist[i][0] = i+1;
	sched[i] = 0;
    }

    goodState = new char[numff];
    ffMap = new int[numgates];
    // get the ffMap
    for (i=0; i<numff; i++)
    {
	ffMap[ff_list[i]] = i;
	goodState[i] = 'X';
    }

    setupWheel(maxlevels, maxLevelSize);
    setFaninoutMatrix();

    if (INIT0)	// if start from a initial state
    {
	RESET_FF1 = new unsigned int [numff+2];
	RESET_FF2 = new unsigned int [numff+2];

	fName = cktName + ".initState";
	yyin.open(fName.c_str(), ios::in);
	if (!yyin)
	{	cerr << "Can't open " << fName << "\n";
		exit(-1);}

	for (i=0; i<numff; i++)
	{
	    yyin >>  c;

	    if (c == '0')
	    {
		RESET_FF1[i] = 0;
		RESET_FF2[i] = 0;
	    }
	    else if (c == '1')
	    {
		RESET_FF1[i] = ALLONES;
		RESET_FF2[i] = ALLONES;
	    }
	    else 
	    {
		RESET_FF1[i] = 0;
		RESET_FF2[i] = ALLONES;
	    }
	}

	yyin.close();
    }

	//generate implication lists
	generateImplicationLists();
}

void LogicSim::generateImplicationLists()
{
	startDirect = clock();
	genDirectImplications();
	cout << "Finished finding all direct implications\n";
	endDirect = clock();
	elapsedMsDirect = ((endDirect - startDirect) * 1000) / CLOCKS_PER_SEC;
	genIndirectImplications();
	cout << "Finished finding all indirect implications\n";
	endIndirect = clock();
	elapsedMsIndirect = ((endIndirect - endDirect) * 1000) / CLOCKS_PER_SEC;
}

ImplicationList LogicSim::getImplicationList(uint32_t imp)
{
	//treats implications like a linked list. Traverses the list, starting at the specified node
	currentList.clear();
	traversedList.clear();
	badImpValue = false;
	//mark first node as traversed
	traversedList.insert(imp);
	if (imp & VALUE)
	{
		for (auto it = oneList[imp & GATE].begin(); it != oneList[imp & GATE].end(); ++it)
		{
			recursiveListGen(*it);
		}
	}
	else
	{
		for (auto it = zeroList[imp & GATE].begin(); it != zeroList[imp & GATE].end(); ++it)
		{
			recursiveListGen(*it);
		}
	}
	return currentList;
}

void LogicSim::recursiveListGen(uint32_t imp)
{
	//check for conflicting implications
	if (badImpValue)
	{
		return;
	}
	if (currentList.count(imp ^ (1<<31)) != 0)
	{
		badImpValue = true;
		return;
	}

	//no conflicting implications
	currentList.insert(imp);
	if (traversedList.count(imp) == 0)
	{
		traversedList.insert(imp);
		if (imp & VALUE)
		{
			for (auto it = oneList[imp & GATE].begin(); it != oneList[imp & GATE].end(); ++it)
			{
				recursiveListGen(*it);
			}
		}
		else
		{
			for (auto it = zeroList[imp & GATE].begin(); it != zeroList[imp & GATE].end(); ++it)
			{
				recursiveListGen(*it);
			}
		}
	}
}

/*
*/
void LogicSim::genDirectImplications()
{
	//add base direct implications for each gate
	for (int i = 1; i < numgates; i++)
	{
		//add identity
		zeroList[i].insert(i);
		oneList[i].insert(i | VALUE);

		//add first level direct implications
		firstLevelImplications(i);
		firstLevelImplications(i | VALUE);
	}
}

void LogicSim::firstLevelImplications(uint32_t imp)
{
	//Adds the first level implications for the given node
	//Implications added are ONLY based on fanin and fanout
	std::vector<uint32_t> localList;
	int index;
	int gateNum;
	int gateVal;
	//find gate number and values
	gateNum = imp & GATE;
	gateVal = imp & VALUE;
	switch (gtype[gateNum])
	{
	case T_and:
		if (gateVal)
		{
			//AND at 1 implies all fanin is 1
			for (index = 0; index < fanin[gateNum]; index++)
			{
				localList.push_back(inlist[gateNum][index] | VALUE);
			}
		}
		break;
	case T_nand:
		if (!gateVal)
		{
			//NAND at 0 implies all fanin is 1
			for (index = 0; index < fanin[gateNum]; index++)
			{
				localList.push_back(inlist[gateNum][index] | VALUE);
			}
		}
		break;
	case T_or:
		if (!gateVal)
		{
			//OR at 0 implies all fanin is 0
			for (index = 0; index < fanin[gateNum]; index++)
			{
				localList.push_back(inlist[gateNum][index]);
			}
		}
		break;
	case T_nor:
		if (gateVal)
		{
			//NOR at 1 implies all fanin is 0
			for (index = 0; index < fanin[gateNum]; index++)
			{
				localList.push_back(inlist[gateNum][index]);
			}
		}
		break;
	case T_output:
		//implies that value is present on fanin
		localList.push_back(inlist[gateNum][0] | gateVal);
		break;
	case T_buf:
		//implies that value is present on fanin
		localList.push_back(inlist[gateNum][0] | gateVal);
		break;
	case T_not:
		//implies opposite value is present on fanin
		if (gateVal)
		{
			//add 0 implication for input
			localList.push_back(inlist[gateNum][0] & GATE);
		}
		else
		{
			//add 1 implication for input
			localList.push_back(inlist[gateNum][0] | VALUE);
		}
		break;
	default:
		//no direct implications for others
		break;
	}
	//add the current list to the array for the specified implication
	for (index = 0; index < localList.size(); index++)
	{
		if (gateVal)
		{
			oneList[gateNum].insert(localList[index]);
		}
		else
		{
			zeroList[gateNum].insert(localList[index]);
		}
	}

	//add the contrapositives for the current list
	uint32_t newImp;
	if (gateVal)
	{
		//if the base implication is 1 then contrapositive is 0
		newImp = gateNum;
	}
	else
	{
		//if the base implication is 0 then contrapositive is 1
		newImp = gateNum | VALUE;
	}

	//if a -> b then ~b -> ~a
	for (index = 0; index < localList.size(); index++)
	{
		if (localList[index] & VALUE)
		{
			zeroList[localList[index] & GATE].insert(newImp);
		}
		else
		{
			oneList[localList[index] & GATE].insert(newImp);
		}
	}
}

/*
This function generates all the indirect implications for each node
It works by, for each node, inserting all the implication values directly 
generated in the previous step. At the end of simulation, every value
which is different from the OrigGateValues (sim with all X inputs) is added
to the implication list for that node (and each other node that was set).
A seperate list tracks the number of elements in each node list. This function
only returns when the number of elements does not change between simulation 
passes, indicating that all indirect implications have been found.
*/
void LogicSim::genIndirectImplications()
{
	//run initial simulation to set OrigGateValues
	initialSim();
	//for each gate, perform simulations until done
	for (int i = 1; i < numgates; i++)
	{
		indirectImplicationSim(i);
		indirectImplicationSim(i | VALUE);
	}
}

void LogicSim::indirectImplicationSim(uint32_t imp)
{
	bool done = false;
	int index;
	int gateN;
	int successor;
	int sucLevel;
	ImplicationList currentList;
	while (!done)
	{
		//reset the circuit to its default state (simulated with all X inputs)
		resetCircuit();
		//for the given node, add all implications successors to the event wheel
		currentList = getImplicationList(imp);
		//if there was a bad implication value (conflicting) clear the list for the current imp
		if (badImpValue)
		{
			fixedNodeCounter++;
			badImpValue = false;
			if (imp & VALUE)
			{
				oneList[imp & GATE].clear();
			}
			else
			{
				zeroList[imp & GATE].clear();
			}
			return;
		}
		for (auto it = currentList.begin(); it != currentList.end(); ++it)
		{
			gateN = *it & GATE;
			GateValues[gateN] = (*it & VALUE) >> 31;
			for (index = 0; index<fanout[gateN]; index++)
			{
				successor = fnlist[gateN][index];
				sucLevel = levelNum[successor];
				insertEvent(sucLevel, successor);
				sched[successor] = 1;
			}
		}
		//run simulation
		goodsim(false);
		if (changes.size() > 0)
		{
			numIndirectImplications = numIndirectImplications + changes.size();
			done = false;
			//add the changes found from simulation
			for (index = 0; index < changes.size(); index++)
			{
				if (imp & VALUE)
				{
					oneList[imp & GATE].insert(changes[index]);
				}
				else
				{
					zeroList[imp & GATE].insert(changes[index]);
				}
			}
		}
		else
		{
			//done with this node if no new implications are found
			done = true;
		}
	}
}

//restores circuit values to defaults, when all inputs are X
void LogicSim::resetCircuit()
{
	for (int i = 0; i < numgates; i++)
	{
		GateValues[i] = OrigGateValues[i];
	}
	x_number = x_number_reset;
	changes.clear();
}

//performs initial simulation with all X inputs. These "default" values 
//are used to check for implications during the indirect implication stage
void LogicSim::initialSim()
{
	char * vec = new char[numpri];
	for (int i = 0; i < numpri; i++)
	{
		vec[i] = 'x';
	}
	applyVector(vec);
	goodsim(false);
	for (int i = 0; i < numgates; i++)
	{
		OrigGateValues[i] = GateValues[i];
	}
	x_number_reset = x_number;
}

void LogicSim::printGateInfo(int gateNumber)
{
	if (gateNumber > numgates)
	{
		cout << "ERROR: Invalid gate number" << endl;
		return;
	}
	cout << "Gate Type: ";
	switch (gtype[gateNumber])
	{
	case T_and:
		cout << "AND" << endl;
		break;
	case T_nand:
		cout << "NAND" << endl;
		break;
	case T_or:
		cout << "OR" << endl;
		break;
	case T_nor:
		cout << "NOR" << endl;
		break;
	case T_xor:
		cout << "XOR" << endl;
		break;
	case T_xnor:
		cout << "XNOR" << endl;
		break;
	case T_not:
		cout << "Inverter" << endl;
		break;
	case T_buf:
		cout << "Buffer" << endl;
		break;
	case T_dff:
		cout << "D Flip Flop" << endl;
		break;
	case T_output:
		cout << "Primary Output" << endl;
		break;
	case T_input:
		cout << "Primary Input" << endl;
		break;
	default:
		cout << "ERROR: Invalid Gate Type" << endl;
		return;
		break;
	}
	cout << "Direct Fan-In:";
	for (int i = 0; i < fanin[gateNumber]; i++)
	{
		cout << " " << inlist[gateNumber][i];
	}
	cout << endl << "Direct Fan-Out:";
	for (int i = 0; i < fanout[gateNumber]; i++)
	{
		cout << " " << fnlist[gateNumber][i];
	}
	cout << endl;
}

void LogicSim::printCircuitInfo()
{
	cout << "\t" << numpri << " PIs.\n";
	cout << "\t" << numout << " POs.\n";
	cout << "\t" << numff << " Dffs.\n";
	cout << "\t" << numFaultFreeGates << " total number of gates.\n";
	cout << "\t" << maxlevels / 5 << " levels in the circuit.\n";
}

////////////////////////////////////////////////////////////////////////
// setFaninoutMatrix()
//	This function builds the matrix of succOfPredOutput and 
// predOfSuccInput.
////////////////////////////////////////////////////////////////////////
void LogicSim::setFaninoutMatrix()
{
    int i, j, k;
    int predecessor, successor;
    int checked[MAXFanout];
    int checkID;	// needed for gates with fanouts to SAME gate
    int prevSucc, found;

    predOfSuccInput = new int *[numgates+64];
    succOfPredOutput = new int *[numgates+64];
    for (i=0; i<MAXFanout; i++)
	checked[i] = 0;
    checkID = 1;

    prevSucc = -1;
    for (i=1; i<numgates; i++)
    {
	predOfSuccInput[i] = new int [fanout[i]];
	succOfPredOutput[i] = new int [fanin[i]];

	for (j=0; j<fanout[i]; j++)
	{
	    if (prevSucc != fnlist[i][j])
		checkID++;
	    prevSucc = fnlist[i][j];

	    successor = fnlist[i][j];
	    k=found=0;
	    while ((k<fanin[successor]) && (!found))
	    {
		if ((inlist[successor][k] == i) && (checked[k] != checkID))
		{
		    predOfSuccInput[i][j] = k;
		    checked[k] = checkID;
		    found = 1;
		}
		k++;
	    }
	}

	for (j=0; j<fanin[i]; j++)
	{
	    if (prevSucc != inlist[i][j])
		checkID++;
	    prevSucc = inlist[i][j];

	    predecessor = inlist[i][j];
	    k=found=0;
	    while ((k<fanout[predecessor]) && (!found))
	    {
		if ((fnlist[predecessor][k] == i) && (checked[k] != checkID))
		{
		    succOfPredOutput[i][j] = k;
		    checked[k] = checkID;
		    found=1;
		}
		k++;
	    }
	}
    }

    for (i=numgates; i<numgates+64; i+=2)
    {
	predOfSuccInput[i] = new int[MAXFanout];
	succOfPredOutput[i] = new int[MAXFanout];
    }
}

////////////////////////////////////////////////////////////////////////
// setTieEvents()
//	This function set up the events for tied nodes
////////////////////////////////////////////////////////////////////////
void LogicSim::setTieEvents()
{
    int predecessor, successor;
    int i, j;

    for (i = 0; i < numTieNodes; i++)
    {
	  // different from previous time frame, place in wheel
	  for (j=0; j<fanout[TIES[i]]; j++)
	  {
	    successor = fnlist[TIES[i]][j];
	    if (sched[successor] == 0)
	    {
	    	insertEvent(levelNum[successor], successor);
			sched[successor] = 1;
	    }
	  }
    }	// for (i...)

    // initialize state if necessary
    if (INIT0 == 1)
    {
cout << "Initialize circuit to values in *.initState!\n";
	for (i=0; i<numff; i++)
	{
	    GateValues[ff_list[i]] = RESET_FF1[i];

	  for (j=0; j<fanout[ff_list[i]]; j++)
	  {
	    successor = fnlist[ff_list[i]][j];
	    if (sched[successor] == 0)
	    {
	    	insertEvent(levelNum[successor], successor);
		sched[successor] = 1;
	    }
	  }	// for j

	    predecessor = inlist[ff_list[i]][0];
		GateValues[predecessor] = RESET_FF1[i];
	  for (j=0; j<fanout[predecessor]; j++)
	  {
	    successor = fnlist[predecessor][j];
	    if (sched[successor] == 0)
	    {
	    	insertEvent(levelNum[successor], successor);
		sched[successor] = 1;
	    }
	  }	// for j

	}	// for i
    }	// if (INIT0)
}

////////////////////////////////////////////////////////////////////////
// applyVector()
//	This function applies the vector to the inputs of the ckt.
////////////////////////////////////////////////////////////////////////
void LogicSim::applyVector(char *vec)
{
    int successor;
    int i, j;

    for (i = 0; i < numpri; i++)
    {
	  switch (vec[i])
	  {
	    case '0':
		GateValues[inputs[i]] = 0;
		break;
	    case '1':
		GateValues[inputs[i]] = 1;
		break;
	    case 'x':
	    case 'X':
		//assign to current X instance
		GateValues[inputs[i]] = x_number;
		//increment the x counter
		x_number = x_number + 2;
		break;
	    default:
		cerr << vec[i] << ": error in the input vector.\n";
		exit(-1);
	  }	// switch

	  // different from previous time frame, place in wheel
	  for (j=0; j<fanout[inputs[i]]; j++)
	  {
	    successor = fnlist[inputs[i]][j];
	    if (sched[successor] == 0)
	    {
	    	insertEvent(levelNum[successor], successor);
			sched[successor] = 1;
	    }
	  }
    }	// for (i...)
}

////////////////////////////////////////////////////////////////////////
// lowWheel class
////////////////////////////////////////////////////////////////////////

void LogicSim::setupWheel(int numLevels, int levelSize)
{
    int i;

    numlevels = numLevels;
    levelLen = new int[numLevels];
    levelEvents = new int * [numLevels];
    for (i=0; i < numLevels; i++)
    {
	levelEvents[i] = new int[levelSize];
	levelLen[i] = 0;
    }
    activation = new int[levelSize];
    
    actFFList = new int[numff + 1];
}

////////////////////////////////////////////////////////////////////////
int LogicSim::retrieveEvent()
{
    while ((levelLen[currLevel] == 0) && (currLevel < maxlevels))
	currLevel++;

    if (currLevel < maxlevels)
    {
    	levelLen[currLevel]--;
        return(levelEvents[currLevel][levelLen[currLevel]]);
    }
    else
	return(-1);
}

// Gate Evaluation functions. Each returns the gate output (1, 0, or X id)
unsigned int LogicSim::evalAND(int gateN)
{
	//read fanin values into the gatevalues vector
	int i, j;
	bool allEqual = true;
	uint32_t val = GateValues[inlist[gateN][0]];

	evalValues.clear();
	for (i = 0; i < fanin[gateN]; i++)
	{
		evalValues.push_back(GateValues[inlist[gateN][i]]);
		//check for controlling value (0)
		if (GateValues[inlist[gateN][i]] == 0)
		{
			return 0;
		}
		//check for same x inputs on all
		if (val != GateValues[inlist[gateN][i]])
		{
			allEqual = false;
		}
		val = GateValues[inlist[gateN][i]];
	}
	//if same input on all return that
	if (allEqual)
	{
		return val;
	}
	//different X input (complements squash to 0)
	for (i = 0; i < evalValues.size(); i++)
	{
		if (evalValues[i] & 0x1) //if odd
		{
			for (j = 0; j < evalValues.size(); j++)
			{
				if (evalValues[j] == (evalValues[i] - 1))
				{
					return 0;
				}
			}
		}
		else //if even
		{
			for (j = 0; j < evalValues.size(); j++)
			{
				if (evalValues[j] == (evalValues[i] + 1))
				{
					return 0;
				}
			}
		}
	}
	//else return a new X value
	val = x_number;
	x_number = x_number + 2;
	return val;
}

unsigned int LogicSim::evalNAND(int gateN)
{
	unsigned int ANDVal = evalAND(gateN);
	if (ANDVal == 1)
	{
		return 0;
	}
	else if (ANDVal == 0)
	{
		return 1;
	}
	else if (ANDVal & 0x1) //odd number X
	{
		return ANDVal - 1;
	}
	else
	{
		//even number X
		return ANDVal + 1;
	}
}

unsigned int LogicSim::evalOR(int gateN)
{
	//read fanin values into the gatevalues vector
	int i, j;
	bool allEqual = true;
	uint32_t val = GateValues[inlist[gateN][0]];

	evalValues.clear();
	for (i = 0; i < fanin[gateN]; i++)
	{
		evalValues.push_back(GateValues[inlist[gateN][i]]);
		//check for controlling value (1)
		if (GateValues[inlist[gateN][i]] == 1)
		{
			return 1;
		}
		//check for same x inputs on all
		if (val != GateValues[inlist[gateN][i]])
		{
			allEqual = false;
		}
		val = inlist[gateN][i];
	}
	//if same input on all return that
	if (allEqual)
	{
		return val;
	}
	//different X input (complements squash to 0)
	for (i = 0; i < evalValues.size(); i++)
	{
		if (evalValues[i] & 0x1) //if odd
		{
			for (j = 0; j < evalValues.size(); j++)
			{
				if (evalValues[j] == (evalValues[i] - 1))
				{
					return 1;
				}
			}
		}
		else //if even
		{
			for (j = 0; j < evalValues.size(); j++)
			{
				if (evalValues[j] == (evalValues[i] + 1))
				{
					return 1;
				}
			}
		}
	}
	//else return a new X value
	val = x_number;
	x_number = x_number + 2;
	return val;
}

unsigned int LogicSim::evalNOR(int gateN)
{
	unsigned int ORVal = evalOR(gateN);
	if (ORVal == 1)
	{
		return 0;
	}
	else if (ORVal == 0)
	{
		return 1;
	}
	else if (ORVal & 0x1) //odd number X
	{
		return ORVal - 1;
	}
	else
	{
		//even number X
		return ORVal + 1;
	}
}

unsigned int LogicSim::evalXOR(int gateN)
{
	unsigned int val1, val2;
	//get gate inputs
	if (fanin[gateN] > 1)
	{
		//2 input
		val1 = GateValues[inlist[gateN][0]];
		val2 = GateValues[inlist[gateN][1]];
	}
	else
	{
		//1 input
		val1 = GateValues[inlist[gateN][0]];
		val2 = val1;
	}
	//check for non-x values (1 or 0)
	if (val1 < 2 && val2 < 2)
	{
		return (val1 ^ val2);
	}
	//check for same x input
	if (val1 == val2)
	{
		//cout << "Xs on inputs of gate " << gateN << " squashed!" << endl;
		return 0;
	}
	//different X inputs
	if (((val1 & 0x1) && val2 == (val1 - 1)) || ((val2 & 0x1) && val1 == (val2 - 1)))
	{
		//cout << "Xs on inputs of gate " << gateN << " squashed!" << endl;
		return 1;
	}
	//else return a new X value
	val1 = x_number;
	x_number = x_number + 2;
	return val1;
}

unsigned int LogicSim::evalXNOR(int gateN)
{
	unsigned int XORVal = evalXOR(gateN);
	if (XORVal == 1)
	{
		return 0;
	}
	else if (XORVal == 0)
	{
		return 1;
	}
	else if (XORVal & 0x1) //odd number X
	{
		return XORVal - 1;
	}
	else
	{
		//even number X
		return XORVal + 1;
	}
}

////////////////////////////////////////////////////////////////////////
// goodsim() -
//	Logic simulate. (no faults inserted)
////////////////////////////////////////////////////////////////////////
void LogicSim::goodsim(bool verbose)
{
    int sucLevel;
    int gateN, predecessor, successor;
    int i;
	unsigned int newVal;
	numSimulations++;

    currLevel = 0;
    actLen = actFFLen = 0;
    while (currLevel < maxlevels)
    {
    	gateN = retrieveEvent();
		if (gateN != -1)// if a valid event
		{
			sched[gateN]= 0;
    		switch (gtype[gateN])
    		{
			case T_and:
				newVal = evalAND(gateN);
	    		break;
			case T_nand:
				newVal = evalNAND(gateN);
	    		break;
			case T_or:
				newVal = evalOR(gateN);
	    		break;
			case T_nor:
				newVal = evalNOR(gateN);
	    		break;
			case T_xor:
				newVal = evalXOR(gateN);
				break;
			case T_xnor:
				newVal = evalXNOR(gateN);
				break;
			case T_not:
				newVal = GateValues[inlist[gateN][0]];
				if (newVal == 0)
				{
					newVal = 1;
				}
				else if (newVal == 1)
				{
					newVal = 0;
				}
				else if (newVal & 0x1) //odd X
				{
					newVal = newVal - 1;
				}
				else //even X
				{
					newVal = newVal + 1;
				}
	    		break;
			case T_buf:
	    		predecessor = inlist[gateN][0];
				newVal = GateValues[predecessor];
				break;
			case T_dff:
	    		predecessor = inlist[gateN][0];
				newVal = GateValues[predecessor];
				actFFList[actFFLen] = gateN;
				actFFLen++;
	    		break;
			case T_output:
				predecessor = inlist[gateN][0];
				newVal = GateValues[predecessor];
				break;
			case T_input:
			case T_tie0:
			case T_tie1:
			case T_tieX:
			case T_tieZ:
	    		newVal = GateValues[gateN];
	    		break;
			  default:
			cerr << "illegal gate type1 " << gateN << " " << gtype[gateN] << "\n";
			exit(-1);
    		}	// switch

			// if gate value changed
    		if (newVal != GateValues[gateN])
			{
				//if value changed to 1 or 0 add to changes list for implications
				if (newVal == 0)
				{
					changes.emplace_back(gateN);
				}
				else if (newVal == 1)
				{
					changes.emplace_back(gateN | VALUE);
				}

				GateValues[gateN] = newVal;

				for (i=0; i<fanout[gateN]; i++)
				{
					successor = fnlist[gateN][i];
					sucLevel = levelNum[successor];
					if (sched[successor] == 0)
					{
					  if (sucLevel != 0)
					insertEvent(sucLevel, successor);
					  else	// same level, wrap around for next time
					  {
					activation[actLen] = successor;
					actLen++;
					  }
					  sched[successor] = 1;
					}
				}	// for (i...)

		}	// if (newVal..)

		}	// if (gateN...)
    }	// while (currLevel...)
    // now re-insert the activation list for the FF's
    for (i=0; i < actLen; i++)
    {
	insertEvent(0, activation[i]);
	sched[activation[i]] = 0;

        predecessor = inlist[activation[i]][0];
        gateN = ffMap[activation[i]];
        if (GateValues[predecessor] == 1)
            goodState[gateN] = '1';
        else if (GateValues[predecessor] == 0)
            goodState[gateN] = '0';
        else
            goodState[gateN] = 'X';
    }
	if (verbose)
	{
		//Print the PO outputs
		cout << "output: ";
		for (i = 0; i < numout; i++)
		{
			if (GateValues[outputs[i]] == 1)
				cout << "1";
			else if (GateValues[outputs[i]] == 0)
				cout << "0";
			else
				cout << "X";
		}
		cout << endl;
	}
}

////////////////////////////////////////////////////////////////////////
// observeOutputs()
//	This function prints the outputs of the fault-free circuit.
////////////////////////////////////////////////////////////////////////
void LogicSim::observeOutputs()
{
    int i;

    cout << "\t";
    for (i=0; i<numout; i++)
    {
	if (GateValues[outputs[i]] == 1)
	    cout << "1";
	else if (GateValues[outputs[i]] == 0)
	    cout << "0";
	else
	    cout << "X";
    }

    cout << "\n";
    for (i=0; i<numff; i++)
    {
	if (GateValues[ff_list[i]] == 1)
	    cout << "1";
	else if (GateValues[ff_list[i]] == 0)
	    cout << "0";
	else
	    cout << "X";
    }
    cout << "\n";
}

