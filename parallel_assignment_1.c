/*
* ASSUMPTIONS: Only one train at a time can load at a station
*/
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

// Train Status
#define IN_TRANSIT 1
#define IN_STATION 0
#define NOT_IN_NETWORK -1

// Links
#define LINK_IS_EMPTY -1
#define LINK_IS_USED 1

// Direction
#define LEFT 0
#define RIGHT 1 

// Station status
#define READY_TO_LOAD -1
#define UNVISITED -2
#define LOADING 0

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
};

typedef struct station_names
{
    char name[20];
} station_name;


int main(int argc, char *argv[]) 
{
    // NOTE: Hardcoded values from sample input
    int S = 8;                 // Number of train stations in the network
    const char *all_stations_list[] = {       // List of stations
        "changi",
        "tampines",
        "clementi",
        "downtown",
        "chinatown",
        "harborfront",
        "bedok",
        "tuas"
    };                          
    int link_transit_time[S][S] = {             // Represents the grap - 1;
        {0, 3, 0, 0, 0, 0, 0, 0},
        {3, 0, 8, 6, 0, 2, 0, 0},
        {0, 8, 0, 0, 4, 0, 0, 5},
        {0, 6, 0, 0, 0, 9, 0, 0},
        {0, 0, 4, 0, 0, 0, 10, 0},
        {0, 2, 0, 9, 0, 0, 0, 0},
        {0, 0, 0, 0, 10, 0, 0, 0},
        {0, 0, 5, 0, 0, 0, 0, 0},
    };                          
    double all_stations_popularity_list[S] = {
        0.9, 0.5, 0.2, 0.3, 0.7, 0.8, 0.4, 0.1
    };
    const char *G[] = {       // Stations in the green line
        "tuas",
        "clementi",
        "tampines",
        "changi"
    };
    const char *Y[] = {       // Stations in the yellow line
        "bedok",
        "chinatown",
        "clementi",
        "tampines",
        "harborfront"
    };
    const char *B[] = {       // Stations in the blue line
        "changi",
        "tampines",
        "downtown",
        "harborfront"
    };
    int N = 10;                  // Number of time ticks in the simulation (Iterations)
    int g = 4;                  // Number of trains in green line
    int y = 10;                  // Number of trains in yellow line
    int b = 10;                  // Number of trains in blue line


    // TODO: Initialization step. Have to find some way to initialise this programmatically.
    // 2D matrix to store the status of each link.
    // value -1 : link is empty | 1 A train is on the link
    int links_status[S][S];
    int i;
    int j;
    for (i = 0; i < S; i++) {
        for (j = 0; j < S; j++) {
            links_status[i][j] = LINK_IS_EMPTY;
        }
    }

    struct train_type green_trains[g] = {
        {0, -1, -1, -1, -1},
        {0, -1, -1, -1, -1},
        {0, -1, -1, -1, -1},
        {0, -1, -1, -1, -1}
    };

    // TODO: Programmatically find the number of stations in the green line.
    // -2 : unvisited
    // -1 : not loading
    // >= 0: index of loading train
    int num_green_stations = 4;
    int green_stations[num_green_stations] = {UNVISITED, UNVISITED, UNVISITED, UNVISITED};
    int green_station_waiting_times[num_green_stations];
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

        #pragma omp parallel shared(introduced_train, green_stations) private(i)
        {
            // This iteration is going through all the trains in green line.
            // As i is private, each train is handled by a single thread.
            for (i = 0; i < g; i++) {
                if (green_trains[i].status == NOT_IN_NETWORK) {
                    #pragma omp critical
                    {
                        if (introduced_train == 0) {
                            // TODO: starting station should alternate between ends of a line
                            int starting_station = 0;
                            // Introduce the train to the network and update.
                            introduced_train = 1;
                            green_trains[i].status = IN_STATION;
                            green_trains[i].station = starting_station;
                            // If no trains are loading. We will start loading the introduced train immediately.
                            if (green_stations[starting_station] == READY_TO_LOAD) {
                                green_stations[starting_station] = LOADING;
                                int current_all_station_index = get_all_station_index(starting_station, G, all_stations_list);
                                green_trains[i].loading_time = calculate_loadtime(all_stations_popularity_list[current_all_station_index]) - 1;
                            }
                        }
                    }
                    else {
                        // A train as already entered the line. As only one train is 
                        // allowed into the line in any time tick, we will skip this.
                        continue;
                    }
                } else if (green_trains[i].status == IN_STATION) {
                    if (green_trains[i].loading_time > 0) {
                        green_trains[i].loading_time--;
                    } else if (green_trains[i].loading_time == FINISHED_LOADING) {
                        int current_station = green_trains[i].station;
                        int current_all_station_index = get_all_station_index(current_station, G, all_stations_list);
                        int next_station = get_next_station(current_station, green_trains[i].direction, num_green_stations);
                        int next_all_station_index = get_all_station_index(next_station, G, all_stations_list);
                        #pragma omp critical
                        {
                            // The link is currently occupied, wait in the station.
                            if (links_status[current_all_station_index][next_all_station_index] == LINK_IS_USED ) {
                                continue;
                            } 
                            // Link unoccupied, move train to the link
                            green_trains[i].transit_time = link_transit_time[current_all_station_index][next_all_station_index] - 1;
                            green_trains[i].status = IN_TRANSIT;
                            green_trains[i].loading_time = WAITING_TO_LOAD;
                            // Empty up station
                            green_stations[current_station] = READY_TO_LOAD;
                            #pragma omp atomic
                            {
                                links_status[current_all_station_index][next_all_station_index] = LINK_IS_USED;
                            }
                        }
                    } else if (green_trains[i].loading_time == WAITING_TO_LOAD) {
                        // This train can start loading.
                        if (green_stations[green_trains[i].station] == READY_TO_LOAD) {
                            green_trains[i].loading_time = calculate_loadtime - 1;
                            green_stations[green_trains[i].station] = i;
                        }
                    }
                } else if (green_trains[i].status == IN_TRANSIT) {
                    if (green_trains[i].transit_time == 0) {
                        // Move the train to the station
                        int current_station = green_trains[i].station;
                        int next_station = get_next_station(current_station, green_trains[i].direction, num_green_stations);
                        // Get next direction based on where the train is going.
                        int next_direction;
                        if (next_station > current_station) {
                            next_direction = RIGHT;
                        } else {
                            next_direction = LEFT;
                        }
                        // Free up the link the train was on.
                        free_link(current_station, next_station, G, all_stations_list, links_status)
                        // Update Train
                        green_trains[i].station = next_station;
                        green_trains[i].direction = next_direction;
                        green_trains[i].status = IN_STATION;
                    } else {
                        green_trains[i].transit_time--;
                    }
                }
            }
        }
        // Master thread consolidation:
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

// Returns the index of the next station with the direction and previous station.
// index: line_station index 
int get_next_station(int prev_station, int direction, int num_stations)  {
    if (direction == RIGHT) {
        // Reached the end of the station
        if (prev_station == num_stations - 1) {
            return prev_station - 1;
        }
        return prev_station + 1;
    }
    // Reached the start of the station
    if (prev_station == 0) {
        return 1;
    }
    return prev_station -1;
}

// Frees up the link
void free_link(int current_station, int next_station, station_name line_stations[], station_name all_stations_list[], int links_status[][]) {
    int current_all_station_index = get_all_station_index(current_station, line_stations, all_stations_list);
    int next_all_station_index = get_all_station_index(next_station, line_stations, all_stations_list);
    #pragma omp atomic 
    {
        links_status[current_all_station_index][next_all_station_index] = LINK_IS_EMPTY;
    }
}

// Returns the index of a station in the "all_station_list"
// index: all_station_list index
int get_all_station_index(int line_station_index, station_name line_stations[], station_name all_stations_list[]) {
    char name = line_stations[line_station_index];
    for (int i = 0; i < sizeof(all_stations_list); i ++) {
        if (strcmp(name, all_stations_list[i]) == 1) { // returns 1 if there is a match.
            return i;
        }
    }
}

int calculate_loadtime(double popularity) {
    double randomNumber;
    randomNumber = rand() % 10;
    return round(rand() * popularity);
}