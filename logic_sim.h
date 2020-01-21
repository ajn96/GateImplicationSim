// Filename:	logic_sim.h
// Author:		Alex Nolan
// Date:		10/4/2018
// Description:	Header file for the gate implication simulator.

#ifndef LOGIC_SIM
#define LOGIC_SIM

//STL includes
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <map>
#include <vector>

//user defined includes
#include "implication_structure.h"

#define GATE 0x7FFFFFFF
#define VALUE 0x80000000

#define HZ 100
#define RETURN '\n'
#define EOS '\0'
#define COMMA ','
#define SPACE ' '
#define TAB '\t'
#define COLON ':'
#define SEMICOLON ';'

#define SPLITMERGE 'M'

#define T_INPUT 1
#define T_OUTPUT 2
#define T_SIGNAL 3
#define T_MODULE 4
#define T_COMPONENT 5
#define T_EXIST 9
#define T_COMMENT 10
#define T_END 11

#define TABLESIZE 5000
#define MAXIO 5000
#define MAXMODULES 5000
#define MAXDFF 10560

#define GOOD 1
#define FAULTY 2
#define DONTCARE -1
#define ALLONES 0xffffffff

#define MAXlevels 10000
#define MAXIOS 5120
#define MAXFanout 10192
#define MAXFFS 40048
#define MAXGATES 100000
#define MAXevents 100000

#define TRUE 1
#define FALSE 0

#define EXCITED_1_LEVEL 1
#define POTENTIAL 2
#define LOW_DETECT 3
#define HIGH_DETECT 4
#define REDUNDANT 5

enum
{
	JUNK,           /* 0 */
	T_input,        /* 1 */
	T_output,       /* 2 */
	T_xor,          /* 3 */
	T_xnor,         /* 4 */
	T_dff,          /* 5 */
	T_and,          /* 6 */
	T_nand,         /* 7 */
	T_or,           /* 8 */
	T_nor,          /* 9 */
	T_not,          /* 10 */
	T_buf,          /* 11 */
	T_tie1,         /* 12 */
	T_tie0,         /* 13 */
	T_tieX,         /* 14 */
	T_tieZ,         /* 15 */
	T_mux_2,        /* 16 */
	T_bus,          /* 17 */
	T_bus_gohigh,   /* 18 */
	T_bus_golow,    /* 19 */
	T_tristate,     /* 20 */
	T_tristateinv,  /* 21 */
	T_tristate1     /* 22 */
};


////////////////////////////////////////////////////////////////////////
// LogicSim class
////////////////////////////////////////////////////////////////////////
class LogicSim
{
	int x_number; //starts at 4 to avoid conflicts with 0/1
	int x_number_reset; //x_number used to reset circuit to default state
	int numTieNodes;
	int *TIES;
	int INIT0;

	// circuit information
	int numFaultFreeGates;	// number of fault free gates
	int numout;		// number of POs
	int maxlevels;	// number of levels in gate level ckt
	int maxLevelSize;	// maximum number of gates in one given level
	int levelSize[MAXlevels];	// levelSize for each level
	int inputs[MAXIOS];
	int outputs[MAXIOS];
	int ff_list[MAXFFS];
	int *ffMap;
	unsigned char *gtype;// gate type
	short *fanin;		// number of fanin, fanouts
	short *fanout;
	int *levelNum;		// level number of gate
	unsigned *po;
	int **inlist;		// fanin list
	int **fnlist;		// fanout list
	char *sched;		// scheduled on the wheel yet?
	unsigned int * GateValues;	//gate values (0, 1 or X number)
	unsigned int * OrigGateValues;	//original gate values, with all X inputs to circuit
	int **predOfSuccInput;      // predecessor of successor input-pin list
	int **succOfPredOutput;     // successor of predecessor output-pin list
	int **levelEvents;	// event list for each level in the circuit
	int *levelLen;	// evenlist length
	int numlevels;	// total number of levels in wheel
	int currLevel;	// current level
	int *activation;	// activation list for the current level in circuit
	int actLen;		// length of the activation list
	int *actFFList;	// activation list for the FF's
	int actFFLen;	// length of the actFFList

public:
	int numgates;	// total number of gates (faulty included)
	int numpri;		// number of PIs
	int numff;		// number of FF's
	unsigned int *RESET_FF1;	// value of reset ffs read from *.initState
	unsigned int *RESET_FF2;	// value of reset ffs read from *.initState

	//number of indirect implications found
	int numIndirectImplications;
	//number of simulations performed
	int numSimulations;
	//number of fixed notes encountered
	int fixedNodeCounter;

	double elapsedMsDirect, elapsedMsIndirect;

	LogicSim(std::string path);	// constructor with path
	LogicSim();					//default constructor

	//functions added for interfacing with REPL
	void printGateInfo(int gateNumber);
	void printCircuitInfo();
	ImplicationList getImplicationList(uint32_t imp);

	void setFaninoutMatrix();	// builds the fanin-out map matrix
	void applyVector(char *);	// apply input vector
	void setupWheel(int, int);
	void insertEvent(int, int);
	int retrieveEvent();
	void goodsim(bool verbose);		// logic sim (no faults inserted)
	void setTieEvents();	// inject events from tied node
	void observeOutputs();	// print the fault-free outputs
	char *goodState;		// good state (without scan)

private:
	//Gate evaluation functions
	unsigned int evalAND(int gateN);
	unsigned int evalNAND(int gateN);
	unsigned int evalOR(int gateN);
	unsigned int evalNOR(int gateN);
	unsigned int evalXOR(int gateN);
	unsigned int evalXNOR(int gateN);

	//functions to generate implication lists for each gate
	void generateImplicationLists();	//controlling function to generate all static implications
	void genDirectImplications();		//function which populates direct implication list for each function
	void firstLevelImplications(uint32_t imp);
	void genIndirectImplications();		//function which finishes implication lists using logic simulation to find indirect implications
	void indirectImplicationSim(uint32_t imp);		//function which runs simulations to determine indirect implications for a set of nodes
	void resetCircuit();	//resets gate values to default (input all X)
	void initialSim();		//applys all X input vector and stores gate results from simulation.
	void recursiveListGen(uint32_t imp);

	//list of implications for all gates at 0
	ImplicationList * zeroList;
	//list of implications for all gates at 1
	ImplicationList * oneList;

	//clocks for measuring performance
	clock_t startDirect, endDirect, endIndirect;

	//used for generating combined implication lists
	ImplicationList currentList;
	ImplicationList traversedList;

	//track if current gate is fixed or not
	bool badImpValue;

	std::vector<uint32_t> changes;

	std::vector<uint32_t> evalValues;
};

#endif