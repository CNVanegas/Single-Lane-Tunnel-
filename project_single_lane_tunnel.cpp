//Name: Cristian Vanegas
//Course: CSCI - 144 FALL 2020
//Project: Single Lane Tunnel Using Threads and Condition Variables

#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
using namespace std;

const int MAXCARS = 200; //Array max for total cars 

//Global Variables to Hold M and N values from command line
int max_can_move = 0; //Holds N for total cars that can move in tunnel at a time
int total_cars = 0;   //Holds M for total amount of cars to pass tunnel
int duration = 0;     //Total time traffic has ran
mutex mtx;            //Single mutex variable for locking critical sections
condition_variable NorthSouth[2];       //Condition variable array to manage N and S traffic [N,S]

static int tunnelCount = 0;             //Current number of cars in the tunnel
static int carsWaiting[2] = {0,0};      //Represents number of cars waiting in each direction
static int currentDirection;            //Determines directions cars travelling: 0 for North, 1 for South
int carTimes[MAXCARS];                  //Array that holds the total travel time for each car
int carStartTime[MAXCARS];              //Array that holds start time for each car
int carArrivalTime[MAXCARS];            //Array that holds arrival time of every car

//Function Prototypes
void carInfo(int, int);                 //Main thread function that handels car arrival and departure
void arrive(int, int);                  //Arrival function handling car arrival and car waiting for crossing
void leave(int, int);                   //Leave function handling car departure notifying other threads to run
void averages(void);                    //Function that calculates the average time cars spent travelling

int main(int argc, char ** argv)
{
    //Input validation to have 3 Arguments given for proper execution
    if (argc != 3)
    {
        cout << "Input: " << argv[0] << " #-of-Cars  #-of-cars-that-can-move" << endl;
        return 0;
    }
    else 
    {
        total_cars = atoi(argv[1]);         //Saving total cars the will move from second argument
        max_can_move = atoi(argv[2]);       //Saving max cars that can from from third argument
        srand(time(NULL));                  //Initializing srand
        bool flip = true;                   //bool value to alternate direction of each car thread of N or S
        thread cars[total_cars];            //Array of threads meant to hold each car thread

        cout << "---- Main Process Beginning ----" << endl << endl;
        int i = 0;

        //Loop creating M car threads
        for (i = 0; i < total_cars; i++)
        {
            //if else to alternate south and north bound car creation
            if (flip)
                cars[i] = thread(carInfo,i,0);
            else
                cars[i] = thread(carInfo,i,1);
            flip = !flip;
            duration += rand() % 10;
        }
        usleep(500); 

        //Wait for M threads to finish
        for (i = 0; i < total_cars;i++)
        {
            cars[i].join();
        }
        
    }
    cout << endl;
    cout << "Traffic Finished after " << duration << " Seconds" << endl << endl;
    averages();
    return 0;
}

//Main car thread function which handles arrival and departure
void carInfo(int id, int dir)
{
    arrive(dir, id);
    
    sleep(1);

    leave(dir, id);
}

//Arrive function handles car arrival and waiting and finally allows car to cross once safe
void arrive(int dir, int id)
{   
    //Creating unique lock for critical section
    unique_lock<mutex> lck(mtx);
    carTimes[id] = rand() % 121;    //Creating random car arrival time
    carArrivalTime[id] = carTimes[id];
    cout << "Car " << id+1 << "    -- Arrives " << (dir == 0 ? "N" : "S") << " -- " << carTimes[id] << "s" << endl;

    //Checks if it is safe for car to begin crossing, otherwise adds to waiting list and halts execution for other threads
    while((tunnelCount == max_can_move) || (tunnelCount > 0 && currentDirection != dir))
    {
        carsWaiting[dir]++;
        NorthSouth[dir].wait(lck);
        carsWaiting[dir]--;
    }

    //By this point it is safe the car to begin crossing, thus increase tunnel count and set green light for cars direction
    tunnelCount++;
    int start = rand() % 6;     //Generate random starting time for car
    start += 1;
    carTimes[id] += start;          //Add start time to total this cars total travel time
    carStartTime[id] = start;       //Save time it took car to start moving 
    currentDirection = dir;         //Set current direction of traffic
    cout << (dir == 0 ? "N " : "S ") << "has Green" << endl;  //Display who has green light
    cout << "Car " << id+1 << "(" << (dir == 0 ? "N" : "S") << ")    -- STARTS --    " << start << "s -> " << (dir == 0 ? "S" : "N") << endl;
    cout << "Elapsed time: " << duration << endl;    //Output current run time
}

//Leave function handles car departure and signals to other waiting cars if they can move
void leave(int dir, int id)
{
    //Create unique lock for critical section
    unique_lock<mutex> lck(mtx);
    carTimes[id] += rand() % 61;    //Generate random time for duration it took car to cross
    tunnelCount--;                  //Decrement tunnel car count
    cout << "Car " << id+1 << "(" << (dir == 0 ? "N" : "S") <<")    -- EXITS --     " << carTimes[id] << "s" << endl;

    //if tunnel still has cars on it notify other cars waiting to go same direction as exiting car
    if (tunnelCount > 0)
        NorthSouth[dir].notify_one();

    //Else if tunnel is empty 
    else
    {
        //If opposite direction has cars waiting, let them go
        if (carsWaiting[1-dir] != 0)
            NorthSouth[1-dir].notify_one();
        //Else let current direction continue going
        else
            NorthSouth[dir].notify_one();
    }
    duration += carTimes[id];   //Add cars total run time to total traffic duration 
}

//Calculates average travel time and start time of all cars 
void averages(void)
{
    int sum = 0;
    int sum2 = 0;
    int sum3 = 0;
    int average = 0;
    for (int i = 0; i < total_cars; i++)
    {
        sum += carTimes[i];
        sum2 += carStartTime[i];
        sum3 += carArrivalTime[i];
    }
        
    average = sum / total_cars;
    cout << "Average Car Travel Time: " << average << endl;
    average = sum2 / total_cars;
    cout << "Average Car Start Time: " << average << endl;
    average = sum3 / total_cars;
    cout << "Average Car Arrival Time: " << average << endl << endl;
}