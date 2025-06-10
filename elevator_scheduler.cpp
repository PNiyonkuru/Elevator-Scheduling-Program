/*
=============================================================================
Title : elevator_scheduler.cpp
Description : This program serves as a first come first serve scheduler for an elevator operating system whose core functionality is written in python.
Authors : Adebola Badru, Ynes Ineza, Prince Niyonkuru, Jeffery Obagbemi
ID no: R11800639,R11733782, R11762556, R11726183 (respectively)
Date : 04/23/2024
Version : 5.0
Usage : Compile and run this program using the GNU C++ compiler
C++ Version : Specify your C++ version
=============================================================================
*/


#include <iostream>
#include <curl/curl.h>
#include <string>
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <cmath>
#include <condition_variable>
#include <atomic>
#include <set>



using namespace std;
std::queue<std::vector<std::string>> peopleToAddToElevator;  //vector queue that holds people in queue ready to be added to elevator
std::mutex responsesMutex;   //Response Mutex
std::mutex elevatorMutex; //Elevator Mutex
atomic<bool> simulationIsRunning(true); //Boolean for simulation is running
int building_bottomFloor = 1; //integer variable to set the bottom floor number
int building_topFloor; //Variable to hold address of topFloor

// Callback function for libcurl
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

//will use this for the scheduling algorithm to access each person's details
class Person {
public:
    string personID;
    int startFloor;
    int endFloor;

    //Default Constructor
    Person(){
    }

    // Constructor taking a string in the format "self.id|self.startFloor|self.endFloor"
    Person(string& personData) {
        stringstream ss(personData);
        string segment;
        getline(ss, segment, '|');
        personID = segment;
        getline(ss, segment, '|');
        startFloor = stoi(segment);
        getline(ss, segment, '|');
        endFloor = stoi(segment);
    }
};
std::vector<Person> responses;  //vector to hold responses from API

//will use this for the scheduling algorithm to store the current status of the elevators
class Elevator {
public:
//Variables to hold needed items for Elevator to run
    std::string elevatorID;
    int lowestFloor;
    int highestFloor;
    int currentFloor;
    int totalCapacity;
    int passengerCount;
    int remainingCapacity;
    std::string directionString;
    Person lastPersonReceived;

    // Method to parse the string and set attributes
    void changeElevatorData(string& elevatorData) {
        std::stringstream ss(elevatorData);
        std::string segment;
        std::getline(ss, segment, '|');
        //elevatorID = segment;
        std::getline(ss, segment, '|');
        currentFloor = stoi(segment);
        std::getline(ss, segment, '|');
        directionString = segment;
        std::getline(ss, segment, '|');
        passengerCount = stoi(segment);
        std::getline(ss, segment, '|');
        remainingCapacity = stoi(segment);
    }
    void UpdateElevatorStatus(){
         // Initialize libcurl
        curl_global_init(CURL_GLOBAL_ALL);

        // Create a new curl handle for the GET request
        CURL* curl = curl_easy_init();

         string elevatorStatusURL = "http://localhost:5432/ElevatorStatus/" + elevatorID;

        // Create a string buffer to hold the response data
        std::string buffer;
        if (curl) {
            // Set the URL for the GET request
            curl_easy_setopt(curl, CURLOPT_URL, elevatorStatusURL.c_str());
              curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

            // Perform the GET request
            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            // Cleanup the curl handle
            curl_easy_cleanup(curl);
            //once elevator info is received, then the current data of the elevator is updated
            changeElevatorData(buffer);

        }

        // Cleanup libcurl
        curl_global_cleanup();
    }
};

// This will store all the elevators in the building, with the elevator ID as the key
std::unordered_map<std::string, Elevator> allElevatorsSet;


void startSimulation(const string& command){
      // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    // Create a curl handle
    CURL* curl = curl_easy_init();

    if (curl) {
          // Construct URL based on command
        std::string url;
        if (command == "start" || command == "stop") {
            url = "http://localhost:5432/Simulation/" + command;
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        } else if (command == "check") {
            url = "http://localhost:5432/Simulation/check";
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        } else {
            std::cerr << "Invalid command" << std::endl;
            return;
        }

        struct curl_slist *headers = NULL;

        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        string buffer;
        if (command == "check") {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        }
        // Perform the PUT request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup the curl handle
        curl_easy_cleanup(curl);
        // If the command is check, see if the simulation is complete
        if (command == "check") {
            if(buffer == "Simulation is complete."){
                simulationIsRunning.store(false);
            }
        }
    }
    curl_global_cleanup();

}
void getNextInput(){
    while(simulationIsRunning.load()){
        // Initialize libcurl
        curl_global_init(CURL_GLOBAL_ALL);
        CURLcode res;
        // Create a new curl handle for the GET request
        CURL* curl = curl_easy_init();
        // Create a string buffer to hold the response data
        std::string response;
        if (curl) {
            // Set the URL to be requested
            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5432/NextInput");
            // Set the callback function to handle the received data
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            // Set the pointer to the response data buffer
            curl_easy_setopt(curl, CURLOPT_WRITEDATA,  &response);
            // Perform the GET request
            res = curl_easy_perform(curl);

            // Check if the request was successful
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }
            if(response == "NONE"){
                // Wait for a little bit after each when there's no response
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            Person newPerson(response);
            // Lock the mutex before accessing the shared vector
            std::unique_lock<std::mutex> responsesLock(responsesMutex);
            // Store the response data in the shared vector
            responses.push_back(newPerson);
            responsesLock.unlock();

            // Clean up libcurl resources
            curl_easy_cleanup(curl);

            // Print the response body
            std::cout << "\nRetrieved String: " << response << std::endl;

        }

        // Cleanup libcurl
        curl_global_cleanup();
    }
    printf("GetNextInput Thread is Done \n");
}
void addPersonToElevator(){
    // Lock the mutex for exclusive access to the next PersonID and ElevatorID
    //std::lock_guard<std::mutex> elevatorLock(elevatorMutex);
    while(simulationIsRunning.load()){
        std::unique_lock<std::mutex> elevatorLock(elevatorMutex);
        if (peopleToAddToElevator.empty()) {
            // If there are no people to add, release the lock and skip this iteration
            elevatorLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); //add a small wait
            continue;
        }
        vector<string> person_to_add = peopleToAddToElevator.front();
        peopleToAddToElevator.pop();
        elevatorLock.unlock();

        string personToAdd = person_to_add[0];
        string ElevatorID = person_to_add[1];
        // Initialize libcurl
        curl_global_init(CURL_GLOBAL_ALL);

        // Create a curl handle
        CURL* curl = curl_easy_init();

        string addToElevatorURL = "http://localhost:5432/AddPersonToElevator/" + personToAdd + "/" + ElevatorID;
        if (curl) {
            struct curl_slist *headers = NULL;
// Sdet the curl header
            headers = curl_slist_append(headers, "Content-Type: application/json");
// Set the curl
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_URL, addToElevatorURL.c_str());
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

            // Perform the PUT request
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            cout << "\nsuccessfully sent: " << personToAdd << " to Elevator: " << ElevatorID << endl;

            // Cleanup the curl handle
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
    printf("AddPersonToElevator Thread is Done \n");
}


void readBuildingLayout(ifstream& input_file){
    building_topFloor = 1;
    // Read data from the file line by line
    std::string line;
    while (std::getline(input_file, line)) {
        // Parse the line
        std::istringstream iss(line);
        string elevatorID;
        int lowestFloor, highestFloor, currentFloor, totalCapacity;
        if (!(iss >> elevatorID >> lowestFloor >> highestFloor >> currentFloor >> totalCapacity)) {
            std::cerr << "Error parsing line: " << line << std::endl;
            continue; // Skip to the next line
        }
        if (highestFloor > building_topFloor){
            building_topFloor = highestFloor;
        }
        // Create a HotelData object
        Elevator initialElevator;
        initialElevator.elevatorID = elevatorID;
        initialElevator.lowestFloor = lowestFloor;
        initialElevator.highestFloor = highestFloor;
        initialElevator.currentFloor = currentFloor;
        initialElevator.totalCapacity = totalCapacity;

        // Store the object in the map
        allElevatorsSet[elevatorID] = initialElevator;
        cout << elevatorID <<" "<< lowestFloor <<" "<< highestFloor <<" "<< currentFloor <<" "<< totalCapacity << endl;
    }
    input_file.close(); //close the building file

}
//This function compares the start and end floors of a person to the range of an elevator to see if the elevator can reach them.
bool isElevatorInRange(Elevator current_elevator, Person current_person ){
    if ((current_elevator.lowestFloor <= current_person.startFloor &&
              current_person.startFloor <= current_elevator.highestFloor) &&
            (current_elevator.lowestFloor <= current_person.endFloor &&
             current_person.endFloor <= current_elevator.highestFloor)){

             return true;
    }
    return false;
}

//After a person is assigned an elevator this function is used to find anyone else on the same floor going to the same destination
void sameFloorsameDirection(Person nextPerson, string elevator_ID){
    //acquire the responses lock for exclusive access to the vector
    std::unique_lock<std::mutex> responsesLock(responsesMutex);
    if (responses.empty()){
        printf("responses is empty so leaving sameFloorsameDirection function\n");
        responsesLock.unlock();
        return;
    }
//Iterate over each person left in responses and see if they are on same floor, going to same destination
    for(auto eachPerson = responses.begin(); eachPerson != responses.end(); ) {
        allElevatorsSet[elevator_ID].UpdateElevatorStatus();
        if(isElevatorInRange(allElevatorsSet[elevator_ID], *eachPerson)){
            if (allElevatorsSet[elevator_ID].remainingCapacity < 1 ){
                return;
            }
        } else {
            ++eachPerson;  // Move to the next element
            continue;
        }
   
        printf("checking for sameFloorsameDirection people\n");
        if( eachPerson->startFloor == nextPerson.startFloor){
            if(nextPerson.endFloor > nextPerson.startFloor && eachPerson->endFloor > nextPerson.startFloor){
                std::unique_lock<std::mutex> elevatorLock(elevatorMutex);
                peopleToAddToElevator.push({eachPerson->personID, elevator_ID});
                elevatorLock.unlock();
                eachPerson = responses.erase(eachPerson);  // Erase and get the iterator to the next element
                printf("sameFloorsameDirection person found\n");
                continue; // Move to the next iteration without incrementing eachPerson
            }
            else if(nextPerson.endFloor < nextPerson.startFloor && eachPerson->endFloor < nextPerson.startFloor){
                std::unique_lock<std::mutex> elevatorLock(elevatorMutex);
                peopleToAddToElevator.push({eachPerson->personID, elevator_ID});
                elevatorLock.unlock();
                eachPerson = responses.erase(eachPerson);  // Erase and get the iterator to the next element
                printf("sameFloorsameDirection person found\n");
                continue; // Move to the next iteration without incrementing eachPerson
            }
        }
       
        ++eachPerson;  // Move to the next element
    }

    //release the responses lock
    responsesLock.unlock();

}
//Function thats used to choose an elevator for a person
string chooseAnElevator(Person& next_person){
//Vectors to hold elevator position to choose from
    vector<string> elevatorsUnderPerson;
    vector<string> elevatorsAbovePerson;
    vector<string> idleElevators;
    for(auto& elevator: allElevatorsSet ){
        Elevator& curr_elevator = elevator.second;
        //Elevators don't go to all floors so we need to
        //Make sure that the elevator we choose is able to reach the next person's floors.
        if (isElevatorInRange(curr_elevator, next_person)){
                curr_elevator.UpdateElevatorStatus();
                if( curr_elevator.directionString == "S" && curr_elevator.passengerCount == 0 ){
                    idleElevators.push_back(curr_elevator.elevatorID);
                }
                //Only choose an elevator if it still has space
                if(curr_elevator.remainingCapacity > 0){
                   if( curr_elevator.currentFloor == next_person.startFloor && curr_elevator.directionString == "S" ){
                         printf("Elevator %s is on floor %d same as the person:%s , so it is chosen\n", curr_elevator.elevatorID.c_str(), curr_elevator.currentFloor, next_person.personID.c_str());
                        return curr_elevator.elevatorID;
                    }
                    if(curr_elevator.currentFloor > next_person.startFloor){
                    elevatorsAbovePerson.push_back(curr_elevator.elevatorID);
                    }
                    else if(curr_elevator.currentFloor < next_person.startFloor){
                    elevatorsUnderPerson.push_back(curr_elevator.elevatorID);
                    }
                }
        }

    }
    //Go through all elevators that are currently unassigned and choose the closest one to next_person's floor
    if(!idleElevators.empty()){
    printf("choosing from idle elevators\n");
        string idleElevator;
        int distance = building_topFloor;
        for(const auto& elevator: idleElevators){
            int new_distance = abs(allElevatorsSet[elevator].currentFloor - next_person.startFloor);
            if( new_distance < distance ){
                idleElevator = elevator;
                distance = new_distance;
            }
        }
        return idleElevator;
    }
    // If there are no availbale elevators, return None
    if (elevatorsAbovePerson.empty()){
        if(elevatorsUnderPerson.empty()){
            printf("There are no available elevators currently\n");
            return "None";
        }
    }

    string closest_Under_elevator;
    int U_distance = building_topFloor;
    string closest_Above_elevator;
    int A_distance = building_topFloor;
    // If statement for when a person is trying to go up, we first check the elevators UNDER them
    if(next_person.endFloor > next_person.startFloor){
    printf("This person is going up\n");
        for(const auto& elevator: elevatorsUnderPerson ){
            // If the elevator is on its way to the next_person's floor
            if(allElevatorsSet[elevator].lastPersonReceived.endFloor == next_person.startFloor){
                printf("There is an elevator under this person, thats on the way to their floor, so its chosen\n");
                return elevator;
            }
            // If the elevator is UNDER next_person and its supposed to pass by their floor
            // could be improved by further finding the closest current floor among these elevators
            else if(allElevatorsSet[elevator].lastPersonReceived.endFloor > next_person.startFloor){
                printf("There is an elevator under this person, that will pass by and is going up, so its chosen\n");
                return elevator;
            }
            //If the last person added is not going to pass by next_person
            //Then check which of the lastPersonReceived from elevatorsUnderPerson is closest to next_person
            else {
                int new_U_distance = abs(allElevatorsSet[elevator].lastPersonReceived.endFloor  - next_person.startFloor);
                if( new_U_distance < U_distance ){
                    U_distance = new_U_distance;
                    closest_Under_elevator = elevator;
                }
            }

        }
        // If an elevator UNDER the next_person is not chosen, see if there's a better choice in ABOVE elevators
        for(const auto& elevator: elevatorsAbovePerson) {
              printf("none of the elevators UNDER this person work, so we try the ones ABOVE\n");
             if(allElevatorsSet[elevator].lastPersonReceived.endFloor == next_person.startFloor){
                 printf("There is an elevator coming down to this person's floor so its chosen\n");
                return elevator;
             }
             //If the last person added is not going to pass by next_person
            //Then check which of the lastPersonReceived from elevatorsAbovePerson is closest to next_person's startFloor
             else {
                 printf("This ABOVE elevator is not going to the next person's floor, it'll be considered later\n");
                int new_A_distance = abs(allElevatorsSet[elevator].lastPersonReceived.endFloor  - next_person.startFloor);
                if( new_A_distance < A_distance ){
                    A_distance = new_A_distance;
                    closest_Above_elevator = elevator;
                }
             }
        }
        //If no choice was made, find the closest elevator between the ABOVE and UNDER elevators
        if(U_distance < A_distance){
            return closest_Under_elevator;
        }
        else{
             return closest_Above_elevator;
        }
     }
    // This will only run when a person is going down( startFloor is greater than endFloor)
    else{
        printf("This person is goin down\n");
        for(const auto& elevator: elevatorsAbovePerson ){
            // If the elevator is on its way to the next_person's floor
            if(allElevatorsSet[elevator].lastPersonReceived.endFloor == next_person.startFloor){
                return elevator;
            }
            // If the elevator is ABOVE next_person and its supposed to pass by their floor, return it
            // could be improved by further finding the closest current floor among these elevators
            else  if(allElevatorsSet[elevator].lastPersonReceived.endFloor < next_person.startFloor){
                return elevator;
            }
            //If the last person added is not going to pass by next_person
            //Then check which of the lastPersonReceived in elevatorsAbovePerson is closest to next_person's startFloor
            else {
                int new_A_distance = abs(allElevatorsSet[elevator].lastPersonReceived.endFloor  - next_person.startFloor);
                if( new_A_distance < A_distance ){
                    A_distance = new_A_distance;
                    closest_Above_elevator = elevator;
                }
            }

        }
        // If an elevator UNDER the next_person is not chosen, see if there's a better choice ABOVE
        for(const auto& elevator: elevatorsUnderPerson) {
             if(allElevatorsSet[elevator].lastPersonReceived.endFloor == next_person.startFloor){
                return elevator;
             }
            //If the last person added is not going to stop at next_person start floor
            //Then check which of the lastPersonReceived in elevatorsUnderPerson is closest to next_person's startFloor
             else {
                int new_U_distance = abs(allElevatorsSet[elevator].lastPersonReceived.endFloor  - next_person.startFloor);
                if( new_U_distance < U_distance ){
                    U_distance = new_U_distance;
                    closest_Under_elevator = elevator;
                }
             }
        }
        //If no choice was made, find the closest elevator between the ABOVE and UNDER elevators
        if(A_distance < U_distance){
            return closest_Above_elevator;
        }
        else{
             return closest_Under_elevator;
        }
    }

}
//Simple FCFS scheduling algorithm
void firstComefirstServe(){
    while(simulationIsRunning.load()){
        std::unique_lock<std::mutex> responsesLock(responsesMutex);
        if (responses.empty()) {
            // If there are no people to add, release the lock and skip this iteration
            responsesLock.unlock();
            startSimulation("check");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // add a small wait
            continue;
        }
        //First Come First Serve, so the person at the front get to go each time
        Person  nextPerson = responses.front();
        printf("%s is starting at floor: %d and is going to floor: %d \n",nextPerson.personID.c_str(), nextPerson.startFloor, nextPerson.endFloor);
        string chosenElevator = chooseAnElevator(nextPerson);
        //If an elevator wasn't chosen, we'll repeat the call on the same person
        if(chosenElevator == "None"){
            printf("Elevator wasn't chosen so return the choosing function\n");
            //release the responses lock
            responsesLock.unlock();
            //add a small wait
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        allElevatorsSet[chosenElevator].lastPersonReceived = nextPerson;
        //If an elevator was chosen for the person, then they're removed from queue
        responses.erase(responses.begin());
        //release the responses lock
        responsesLock.unlock();
        //Store the person's ID and their elevator in a vector to be sent to an elevator
        vector<string> person_to_elevator = {nextPerson.personID, allElevatorsSet[chosenElevator].elevatorID};
        std::unique_lock<std::mutex> elevatorLock(elevatorMutex);
        peopleToAddToElevator.push(person_to_elevator);
        elevatorLock.unlock(); //release the elevator lock
       
        // Find any people in queue who are on the same floor and going in the same direction
        // If any are found they will be added to the same elevator
        sameFloorsameDirection(nextPerson, chosenElevator);


    }
    printf("FCFS Scheduler Thread is done\n");
}

int main(int argc, char* argv[])
{
    std::ifstream input_file(argv[1]); // Open the file

    if (!input_file) {
        std::cerr << "Error opening file.\n";
        return 1;
    }

    readBuildingLayout(input_file);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); //wait for the simulation to load
    startSimulation("start");

    thread Input_Communication_Thread(getNextInput);   //Thread to get next input
    thread Output_Communication_Thread(addPersonToElevator);    //Thread to add person to elevator
    thread Scheduler_Computation_Thread(firstComefirstServe); //Thread to Schedule the elevators

// Join the threads
    Input_Communication_Thread.join();
    Output_Communication_Thread.join();
    Scheduler_Computation_Thread.join();

    return 0;
}
