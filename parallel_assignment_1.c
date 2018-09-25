#include <omp.h>
#include <stdio.h>
#include <stdlib.h>


struct train_type
{
    int loading_time;   // -1 for has not started loading yet | 0 for not loading| > 0 for currently loading
    int in_transit;     // 1 for in transit | 0 for in station | -1 for not in line
    int direction;      // 1 for up  | 0 for down
    int station;        // Integer denotes the index in the station array
    int transit_time;   // -1 for NA | > 0 for in transit
}

typedef struct station_name
{
    char name[20];
} name;


int main(int argc, char *argv[]) 
{
    // NOTE: Hardcoded values from sample input
    int S = 10;                 // Number of train stations in the network
    station_name L[S] = {       // List of stations
        "changi",
        "tampines",
        "clementi",
        "downtown",
        "chinatown",
        "harborfront",
        "bedok",
        "tuas"
    };                          
    int m[S][S] = {             // Represents the graph
        {0, 3, 0, 0, 0, 0, 0, 0},
        {3, 0, 8, 6, 0, 2, 0, 0},
        {0, 8, 0, 0, 4, 0, 0, 5},
        {0, 6, 0, 0, 0, 9, 0, 0},
        {0, 0, 4, 0, 0, 0, 10, 0},
        {0, 2, 0, 9, 0, 0, 0, 0},
        {0, 0, 0, 0, 10, 0, 0, 0},
        {0, 0, 5, 0, 0, 0, 0, 0},
    };                          
    double P[S] = {
        0.9, 0.5, 0.2, 0.3, 0.7, 0.8, 0.4, 0.1
    }
    station_name G[4] = {       // Stations in the green line
        "tuas",
        "clementi",
        "tampines",
        "changi"
    };
    station_name Y[5] = {       // Stations in the yellow line
        "bedok",
        "chinatown",
        "clementi",
        "tampines",
        "harborfront"
    };
    station_name B[4] = {       // Stations in the blue line
        "changi",
        "tampines",
        "downtown",
        "harborfront"
    };
    int N = 10                  // Number of time ticks in the simulation (Iterations)
    int g = 10                  // Number of trains in green line
    int y = 10                  // Number of trains in yellow line
    int b = 10                  // Number of trains in blue line


    // Creating global variables that are shared.
    int links[4];
    
    // TODO: Have to find some way to initialise this programmatically.
    train_type green_trains[g] = {
        {0, -1, 0, -1},
        {0, -1, 0, -1},
        {0, -1, 0, -1},
        {0, -1, 0, -1}
    };
    int green_stations[4]; 

    // Set the number of threads to be = number of trains
    omp_set_num_threads(g);

    int time_tick;
    for (time_tick = 0 ; time_tick < N; time_tick++) {
        // Entering the stations 1 time tick at a time.
        int i;
        // Boolean value to make sure that only 1 train enters the line at any time tick.
        int introduced_train = 0;
        #pragma omp parallel shared(introduced_train) private(i)
        {
            // This iteration is going through all the trains in green line.
            // As i is private, each train is handled by a single thread.
            for (i = 0; i < g; i++) {
                if (green_trains[i].in_transit == -1) {
                    #pragma omp critical
                    if (introduced_train == 0) {
                        introduced_train = 1;
                        green_trains[i].in_transit = 0;
                        // TODO: Currently starting off only from the first station. Need to start
                        // off from last station too.
                        green_trains[i].station = 0;
                    }
                    else {
                        // A train as already entered the line. As only one train is 
                        // allowed into the line in any time tick, we will skip this.
                        continue
                    }
                } else if (green_trains[i].in_transit == 0) {
                    if (green_trains[i].loading_time > 0) {
                        green_trains[i].loading_time--;
                    } else if (green_trains[i].loading_time == 0) {
                        // TODO: Check if train can move into link.
                    } else {
                        // TODO: Check if train can start loading.
                    }
                } else {
                    if (green_trains[i].transit_time == 0) {
                        // TODO: Move train into station
                    } else {
                        green_trains[i].transit_time--;
                    }
                }
            }
        }
    }
}



// TODO: Finish calculate_loadtime
int calculate_loadtime(){


    return 0
}