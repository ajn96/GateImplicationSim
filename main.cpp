// Filename:	main.cpp
// Author:		Alex Nolan
// Date:		10/4/2018
// Description:	Main entry point for the gate implication simulator

#include "circuit_repl.h"

using namespace std;

int main(int argc, char *argv[])
{
	if ((argc != 2))
	{
		cerr << "ERROR: Please Specify a circuit path as a command line argument to the program" << endl;
		exit(EXIT_FAILURE);
	}

	//create a control REPL
	CircuitREPL repl(argv[1]);

	//start the REPL
	repl.startREPL();

	//return
	exit(EXIT_SUCCESS);
}