# OS_Elevator_Scheduler_Project
This is the repository to handle the coding regarding the project
Project title : OperatinSystems_Elevator_Scheduler_Project

# Description of Project 
=========================

This code written in C++ to serve as a scheduler for an elevator operating system whose core functionality is written completely in python. The scheduling is done using a "First Come First Serve" algorithm.


# Multithreading Requirement
==========================
  
This program makes use of 3 threads( Input communication thread, Output Communication thread and Scheduler Computation thread) that will be running concurrently, and provides any necessary protection from race conditions.

# Testing Requirements
==========================
  
For testing, our code will be written to be able to compile using GNU version 5.4.0 running on the HPCC Quanah Cluster.
Our code will not surpass the resources that the final grading script will request for our job wheen run in the HPCC.

# Timing Requirements
==========================
  
Our code will be capable of completing in the required runtime, itll be able to run atleast 10% faster than professor's simplistic code.


# Usage instructions
=========================

The code needs the python API provided by the professor to run and our makefile, which is included. 
The code takes in the buidling file as an argument. 


# How to decide which elevator to put someone on: chooseAnElvator() function
==============================================================================

1.  Firstly, Since all Elevators don’t go to all the floors, we’ll have to make sure that the elevator chosen for this person, is able to get to the person’s start and destination in the first place.

  a.  This could then narrow down the options and we can choose from the remaining options.

  b.  Once its narrowed down, ElevatorStatus is called and used to update info(current floor, direction, remaining capacity etc) for the elevators.

2.  Secondly, If any elevator is full, disregard it.

  a.  If all are full, then skip this round

3.  If there is an elevator that has stopped on the person’s floor then that elevator is chosen for this person.

4.  If any elevators are not moving and don’t have anyone in them, then they are not doing anything. For our function we’ll have these elevators pick someone up immediately. If there’s multiple, the function chooses the closest one.

5.  Once the options are narrowed down , and no elevator has been chosen then more specific choices are made:

6.  For each person on a floor:

  a.  The function first separates all the elevators into ABOVE & UNDER elevators.

  b.  If a passenger is going up:(an elevator UNDER is preferred)

                     i.    Check if any elevators UNDER them are about to go up to a floor past them or the floor they’re at(using last passengers drop off), if so choose the first one found(this simplifies the algorithm)- and leave the function and return that elevator. If none of the elevators UNDER them are going up then choose whichever one’s last passenger is the closest but before its chosen, it’ll first have to be compared to the ABOVE elevators.

                    ii.  If an elevator is ABOVE a passenger, check if its going down, then check if its last passenger’s drop off is  equal to current person’s start floor, if so, then this elevator is returned. Else, check whichever one’s last passenger has the closest drop off and if its better than UNDER’s closest last passenger drop off its returned as the person’s elevator.

  c.  If a passenger is going down:(an elevator UNDER is preferred)

                     i.   Check if any elevators ABOVE them are about to go down to a floor past them or the floor they’re on( using last passengers drop off), if so, choose the first one found(this simplifies the algorithm)- and leave the function and return that elevator. If none of the elevators ABOVE them are going down then choose whichever one’s last passenger is the closest but before its chosen, it’ll first have to be compared to the UNDER elevators.

                    ii.  If an elevator is UNDER a passenger, check if its going up, then check if its last passenger drop off is equal to current start floor, if so, then this elevator is returned. Else, check whichever one’s last passenger has the closest drop off and if its better than ABOVE’s closest last passenger drop off.
