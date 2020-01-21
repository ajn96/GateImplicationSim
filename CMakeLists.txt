cmake_minimum_required(VERSION 3.5)

project(GateImplicationSim CXX)
	 
SET (SOURCE_FILES circuit_repl.cpp circuit_repl.h implication_structure.h logic_sim.cpp logic_sim.h main.cpp)
	 
add_executable(GateImplicationSim ${SOURCE_FILES})