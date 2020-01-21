// Filename:	circuit_repl.cpp
// Author:		Alex Nolan
// Date:		10/4/2018
// Description:	A Read-Eval-Print Loop for inerfacing with the gate
//				implication simulator via a command line.

#include "circuit_repl.h"

//Default constructor
CircuitREPL::CircuitREPL()
{
	CircuitREPL("badPath");
}

//Overloadeded constructor
CircuitREPL::CircuitREPL(std::string circuitPath)
{
	//print welcome message
	printWelcome();
	//create the circuit from the file
	sim = new LogicSim(circuitPath);
	cktPath = circuitPath;
	std::cout << "Enter a command, or help to begin" << std::endl;
}

//start the REPL on the standard input
void CircuitREPL::startREPL()
{
	bool running = true;
	std::string currentCommand;
	while (running)
	{
		std::cout << ">";
		std::getline(std::cin, currentCommand);
		running = parseLine(currentCommand);
	}
}

bool CircuitREPL::parseLine(std::string line)
{
	Command currentCommand;
	int commandIndex = line.find(" ");
	if (commandIndex == -1)
	{
		commandIndex = line.length();
	}
	std::string command = line.substr(0, commandIndex);
	currentCommand = parseCommand(command);
	switch (currentCommand)
	{
	case Quit:
		return false;
		break;
	case Unknown:
		std::cout << "Error: Unknown command " << line << std::endl;
		std::cout << "Enter help for command list" << std::endl;
		break;
	case Help:
		printHelp();
		break;
	case GetImplication:
		printImplication(line.substr(commandIndex + 1, line.length()));
		break;
	case GetCktInfo:
		printCktInfo();
		break;
	case GetGateInfo:
		printGate(line.substr(commandIndex + 1, line.length()));
		break;
	case SimVector:
		simVector(line.substr(commandIndex + 1, line.length()));
		break;
	case Stats:
		printStats();
		break;
	default:
		std::cout << "Error: Unknown Error" << std::endl;
		break;
	}
	return true;
}

Command CircuitREPL::parseCommand(std::string command)
{
	if (command == "imp")
		return GetImplication;
	if (command == "help")
		return Help;
	if (command == "quit")
		return Quit;
	if (command == "gate")
		return GetGateInfo;
	if (command == "ckt")
		return GetCktInfo;
	if (command == "sim")
		return SimVector;
	if (command == "stats")
		return Stats;
	//else return unknown
	return Unknown;
}

void CircuitREPL::printWelcome()
{
	std::cout << "Welcome to Gate Implication Simulator" << std::endl;
	std::cout << "Designed for ECE 4520 by Alex Nolan" << std::endl << std::endl;
}

void CircuitREPL::printStats()
{
	std::cout << "Found a total of " << sim->numIndirectImplications << " implications via logic simulation\n";
	std::cout << "Found a total of " << sim->fixedNodeCounter << " fixed gates which can only take a single value\n";
	std::cout << "Circuit was logic simulated " << sim->numSimulations << " times\n";
	std::cout << "Calculated all direct implications in " << sim->elapsedMsDirect << " milliseconds\n";
	std::cout << "Calculated all indirect implications in " << sim->elapsedMsIndirect << " milliseconds\n";
}

void CircuitREPL::printHelp()
{
	std::cout << "Gate Implication Simulator Help" << std::endl;
	std::cout << "Please enter one of the following commands:" << std::endl << std::endl;
	std::cout << "imp <gate number> <gate value>" << std::endl;
	std::cout << "This command prints the list of logical implications for the specified gate" << std::endl;
	std::cout << "Example Usage to show implications of gate 1 at value 0: >imp 1 0" << std::endl << std::endl;
	std::cout << "sim <input vector>" << std::endl;
	std::cout << "This command prints the circuit PO's for the specified input vector" << std::endl;
	std::cout << "Example usage to simulate the vector 1X0 on the current circuit: >sim 1X0" << std::endl << std::endl;
	std::cout << "gate <gate number>" << std::endl;
	std::cout << "This command prints a set of parameters for the specified gate" << std::endl;
	std::cout << "Example Usage to show the information for gate 1: >gate 1" << std::endl << std::endl;
	std::cout << "ckt" << std::endl;
	std::cout << "This command prints a list of the parameters for the current circuit" << std::endl << std::endl;
	std::cout << "stats" << std::endl;
	std::cout << "This command prints some statistics about the implication finding process" << std::endl << std::endl;
	std::cout << "quit" << std::endl;
	std::cout << "This command quits the simulator" << std::endl;
}

void CircuitREPL::printImplication(std::string command)
{
	ImplicationList selectedList;
	int gateNum;
	int impVal;
	int spaceIndex;
	spaceIndex = command.find(" ");
	//read input gate and value
	try
	{
		gateNum = std::stoi(command.substr(0, spaceIndex));
		impVal = std::stoi(command.substr(spaceIndex, command.length()));
	}
	catch (std::exception ex)
	{
		std::cout << "ERROR: Invalid command format" << std::endl;
		return;
	}
	//check for gate number range
	if (gateNum >= sim->numgates || gateNum < 0)
	{
		std::cout << "ERROR: Invalid Gate Number " << gateNum << std::endl;
		return;
	}
	//check for value
	switch (impVal)
	{
	case 0:
		selectedList = sim->getImplicationList(gateNum);
		break;
	case 1:
		selectedList = sim->getImplicationList(gateNum | VALUE);
		break;
	default:
		std::cout << "ERROR: Invalid implication value (must be 0 or 1)" << std::endl;
		return;
		break;
	}
	//only case for impossible state
	if (selectedList.size() == 0)
	{
		std::cout << "Gate " << gateNum << " at value " << impVal << " is not reachable in this circuit" << std::endl;
		return;
	}
	std::cout << "Gate " << gateNum << " at value " << impVal << " implies:" << std::endl;
	for (auto it = selectedList.begin(); it != selectedList.end(); ++it)
	{
		std::cout << "Gate " << (*it & GATE) << " at value " << ((*it & VALUE) >> 31) << std::endl;
	}
}

void CircuitREPL::printGate(std::string command)
{
	int gateNum;
	try
	{
		gateNum = std::stoi(command);
	}
	catch (std::exception ex)
	{
		std::cout << "ERROR: Invalid command format" << std::endl;
		return;
	}
	sim->printGateInfo(gateNum);
}

void CircuitREPL::printCktInfo()
{
	std::cout << "Circuit: " << cktPath << std::endl;
	sim->printCircuitInfo();
}

void CircuitREPL::simVector(std::string command)
{
	char * vector;
	int vecIndex = 0;
	vector = new char[command.length()];
	//convert string to c style and remove invalid chars
	for (size_t i = 0; i < command.length(); i++)
	{
		if (command[i] == '0' || command[i] == '1' || command[i] == 'x' || command[i] == 'X')
		{
			vector[vecIndex] = command[i];
			vecIndex++;
		}
		else if (command[i] != ' ')
		{
			std::cout << "ERROR: Bad input value " << command[i] << std::endl;
			return;
		}
	}
	if (vecIndex + 1 < sim->numpri)
	{
		std::cout << "ERROR: Bad input vector, too few values" << std::endl;
		return;
	}
	sim->applyVector(vector);
	sim->goodsim(true);
}
