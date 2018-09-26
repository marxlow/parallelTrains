#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define IN_TRANSIT 1
#define IN_STATION 0
#define NOT_IN_NETWORK -1

#define NO_TRAINS -1
#define LINK_IS_EMPTY -1

#define LEFT 0
#define RIGHT 1 

// Station status
#define READY_TO_LOAD -1
#define UNVISITED -2

// Loading status
#define WAITING_TO_LOAD -1
#define FINISHED_LOADING 0

struct train_type
{
    int loading_time;   // -1 waiting to load | 0 has loaded finish at the station| > 0 for currently loading
    int status;     // 1 for in transit | 0 for in station | -1 for not in network
    int direction;      // 1 for up  | 0 for down
    int station;        // -1 for not in any station | > 0 for index of station it is in
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
    int g = 4                  // Number of trains in green line
    int y = 10                  // Number of trains in yellow line
    int b = 10                  // Number of trains in blue line


    // TODO: Initialization step. Have to find some way to initialise this programmatically.
    int links[4] = {LINK_IS_EMPTY, LINK_IS_EMPTY, LINK_IS_EMPTY, LINK_IS_EMPTY}; // -1 empty link

    train_type green_trains[g] = {
        {0, -1, -1, -1, -1},
        {0, -1, -1, -1, -1},
        {0, -1, -1, -1, -1},
        {0, -1, -1, -1, -1}
    };

    // TODO: Programmatically find the number of stations in the green line.
    // -2 : unvisited
    // -1 : not loading
    // >= 0: index of loading train
    int num_green_stations = 4
    int green_stations[num_green_stations] = { -2, -2, -2, -2 };
    int green_station_waiting_times[4];
    int i;
    for (i = 0 ; i < num_green_stations; i++) {
        green_station_waiting_times[i] = 0;
    }

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
                if (green_trains[i].status == NOT_IN_NETWORK) {
                    #pragma omp critical
                    if (introduced_train == 0)
                        int starting_station = 0;
                        // Introduce the train to the network and update.
                        introduced_train = 1;
                        green_trains[i].status = IN_STATION;
                        green_trains[i].station = starting_station;
                        if (green_stations[starting_station] == NO_TRAINS) {
                            green_stations[starting_station] = i;
                            // TODO: Get loading time of a train with a random function.
                            green_trains[i].loading_time = calculate_loadtime() - 1;
                        }
                    }
                    else {
                        // A train as already entered the line. As only one train is 
                        // allowed into the line in any time tick, we will skip this.
                        continue;
                    }
                } else if (green_trains[i].status == IN_STATION) 
                    if (green_trains[i].loading_time > 0) {
                        green_trains[i].loading_time--;
                    } else if (green_trains[i].loading_time == FINISHED_LOADING) {
                        // Check if train can move into link.
                        int link_index = get_next_link(station, direction, m, green_stations, L);
                        #pragma omp critical 
                        {
                            // The link is currently occupied, wait in the station.
                            if (link[link_index] >= 0) {
                                continue;
                            }
                            // Link unoccupied, move train onto link
                            #pragma omp atomic 
                            {
                                link[link_index] = i;
                            }
                        }
                        green_trains[i].status = IN_TRANSIT;
                        green_trains[i].transit_time = get_transit_time();
                        green_trains[i].loading_time = WAITING_TO_LOAD;
                    } else if (green_trains[i].loading_time == WAITING_TO_LOAD) {
                        // This train can start loading.
                        if (green_stations[green_trains[i].station] == READY_TO_LOAD) {
                            green_trains[i].loading_time = calculate_loadtime - 1;
                            green_stations[green_trains[i].station] = i;
                        }
                    }
                } else if (green_trains[i].status == IN_TRANSIT) {
                    if (green_trains[i].transit_time == 0) {
                        // Move the train from the link to the station
                        int link_index = get_next_link(station, direction, m, green_stations);
                        #pragma omp atomic 
                        {
                            link[link_index] = i;
                        }
                        int next_station = get_next_station(green_trains[i].station, green_trains[i].direction);
                        green_trains[i].station = nextStation;
                        green_stations[next_station] = 1;
                    } else {
                        green_trains[i].transit_time--;
                    }
                }
            }
        }
        // Master thread consolidation:
        int i;
        // Count the waiting times at each station.
        for (i = 0 ; i < num_green_stations; i++) {
            if (green_stations[i] == READY_TO_LOAD) {
                green_station_waiting_times[i]++;
            } 
        }
        // Clean up stations where the loading train has just finished loading up passengers.
        for (i = 0 ; i < num_green_stations; i++) {
            if (green_stations[i] >= 0) {
                if (green_trains[green_stations[i]].loading_time == 0) {
                    green_stations[i] == WAITING_TO_LOAD;
                }
            }
        }
    }
    double average_waiting_time = get_average_waiting_time(green_station_waiting_times, num_green_stations, N);
    double longest_average_waiting_time = 0;
    double shortest_average_waiting_time = INT_MAX;
    get_longest_shortest_average_waiting_time(green_station_waiting_times, num_green_stations, N, &longest_average_waiting_time, &shortest_average_waiting_time);
}

void get_longest_shortest_average_waiting_time(int green_station_waiting_times[], int num_green_stations, int N, 
                                        int *longest_average_waiting_time, int *shortest_average_waiting_time) {
    int i;
    for (i = 0; i < num_green_stations; i++) {
        if (*longest_average_waiting_time < double(green_station_waiting_times[i]) / double(N)) {
            *longest_average_waiting_time = double(green_station_waiting_times[i]) / double(N);
        }
        if (*shortest_average_waiting_time > double(green_station_waiting_times[i] / double(N))) {
            *shortest_average_waiting_time = double(green_station_waiting_times[i] / double(N));
        }
    }
}

double get_average_waiting_time(int green_station_waiting_times[], int num_green_stations, int N) {
    int i;
    int total_waiting_time = 0;
    for (i = 0; i < num_green_stations; i++) {
        total_waiting_time += green_station_waiting_times[i];
    }
    double average_waiting_time;
    average_waiting_time = double(total_waiting_time) / double(num_green_stations);
    average_waiting_time = average_waiting_time / double(N);
    return average_waiting_time;
}

int get_next_station(int prev_station, int direction) {
    return 0;
}
// TODO: Get the next link of the station
// The parameters takes in the line that the train is in and the direction it is going.
// RETURN: Index of the link in the link array. 
// Also return the transit time? Or separate it into another function?
int get_next_link(int station, int direction, int m[][], station_name line[], station_name L[]) {
    return 0
}

int get_transit_time() {
    return 0
}

// TODO: Finish calculate_loadtime
int calculate_loadtime(){
    
    return 0
}