#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_GREEN_TRAIN 2
#define NUM_GREEN_STATION 3
#define NUM_ITERATIONS 10

struct train_type
{
    int loading_time;
    int in_transit; // 1 for in transit | 0 for in station
    int direction; // 1 for up  | 0 for down
    char station[2]; // station the train is currently in/from
    char *name;
}

struct station 
{
    int loading_time; // -1: Initial state unvisited by any trains. 0: Waiting state. > 0: Loading state.
    char *loading_train_name; // e.g g0... g11
}

void work()
{
    // Initialize thread as a train
    struct 
}

int main(int argc, char *argv[]) 
{
    // TODO: These variables should get their values from stdin.
    int num_green_station = NUM_GREEN_STATION;
    int num_green_train = NUM_GREEN_TRAIN;
    int num_iterations = NUM_ITERATIONS;

    train_type trains[num_green_train];
    station greenStations[num_green_station];

    // Initialize stations
    for (i = 0; i < num_green_station; i++) {
        greenStations[i].loading_time = -1;
    }

    // Set the number of threads to be = number of trains
    omp_set_num_threads(num_green_train);

    // Start parallel code
    #pragma omp parallel private(i)
    {
        for (i = 0; i < num_green_train; i ++) {

        }
    }


}