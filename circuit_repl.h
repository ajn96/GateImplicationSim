// Filename:	circuit_repl.h
// Author:		Alex Nolan
// Date:		10/4/2018
// Description:	Header file for the command line interface to the gate
//				implication simulator.

#ifndef CIRCUIT_REPL
#define CIRCUIT_REPL

//user defined includes
#include "logic_sim.h"
#include "implication_structure.h"

//STL includes
#include <string>
#include <iostream>
#include <vector>

enum Command
{
	Unknown,
	Help,
	GetImplication,
	GetGateInfo,
	GetCktInfo,
	SimVector,
	Quit,
	Stats
};

class CircuitREPL
{
public:
	//overloaded constructor which takes a file path
	CircuitREPL(std::string circuitPath);
	//default constructor
	CircuitREPL();
	//function to start the REPL
	void startREPL();
private:
	//function to parse each line and call needed handler
	bool parseLine(std::string line);
	//function to parse a command from a string
	Command parseCommand(std::string command);
	//function to print welcome message
	void printWelcome();
	//function to print help message
	void printHelp();
	//function to print implication
	void printImplication(std::string command);
	//function to return info for one gate
	void printGate(std::string command);
	//function to return info about current circuit
	void printCktInfo();
	//function to send a vector input and print PO
	void simVector(std::string command);
	//function to print statistics
	void printStats();

	//simulator
	LogicSim *sim;

	//path to the circuit file
	std::string cktPath;
};

#endif