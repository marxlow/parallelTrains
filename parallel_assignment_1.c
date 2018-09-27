/*
 * ASSUMPTIONS: 
 * 1. Only one train at a time can load at a station
 * 2. Upon reaching a terminal station (Tamp -> Changi | direction: Right). Train will load for station[right][changi] rather than station[left][changi]. 
*/
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define VISITED 1
#define LOADING 0

// Loading status
#define WAITING_TO_LOAD -1
#define FINISHED_LOADING 0

// Introducing trains into the network
#define INTRODUCED 1
#define NOT_INTRODUCED 0

struct train_type
{
    int loading_time; // -1 waiting to load | 0 has loaded finish at the station| > 0 for currently loading
    int status;       // 1 for in transit | 0 for in station | -1 for not in network
    int direction;    // 1 for up  | 0 for down
    int station;      // -1 for not in any station | > 0 for index of station it is in
    int transit_time; // -1 for NA | > 0 for in transit
};

// FUNCTION DECLARATIONS $$ DO NOT REMOVE.
void print_status(struct train_type green_trains[], int num_green_trains, char *G[], int num_green_stations);
void get_longest_shortest_average_waiting_time(int num_green_stations, int green_station_waiting_times[][4], int N, double *longest_average_waiting_time, double *shortest_average_waiting_time);
double get_average_waiting_time(int num_green_stations, int green_station_waiting_times[][num_green_stations], int N);
int get_next_station(int prev_station, int direction, int num_stations);
void free_link(int num_stations, int current_station, int next_station, char *line_stations[], char *all_stations_list[], int links_status[][8]);
int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[]);
int calculate_loadtime(double popularity);
int change_train_direction(int direction);


// FUNCTIONS
int change_train_direction(int direction) 
{
    direction += 1;
    return direction % 2;
}

void print_status(struct train_type green_trains[], int num_green_trains, char *G[], int num_green_stations)
{
    int i;
    for (i = 0; i < num_green_trains; i++)
        {
        if (green_trains[i].status == IN_STATION)
        {
            printf("Train %d is currently in station %d | With loading time: %d \n", i, green_trains[i].station, green_trains[i].loading_time);
        }
        else if (green_trains[i].status == IN_TRANSIT)
        {
            int current_station = green_trains[i].station;
            int next_station = get_next_station(current_station, green_trains[i].direction, num_green_stations);
            printf("Train %d is currently in transit %s->%s| With transit time: %d \n", i, G[current_station], G[next_station], green_trains[i].transit_time);
            printf("Current train direction = %d\n", green_trains[i].direction);
        }
    }
}

void get_longest_shortest_average_waiting_time(int num_green_stations, int green_station_waiting_times[][num_green_stations], int N, double *longest_average_waiting_time, double *shortest_average_waiting_time)
{
    int i;
    int j;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < num_green_stations; j++) 
        {
            double waiting_time = (double)green_station_waiting_times[i][j];
            double station_average_waiting_time = waiting_time / (double)N;
            // Update waiting times
            if (*longest_average_waiting_time < station_average_waiting_time)
            {
                *longest_average_waiting_time = station_average_waiting_time;
            }
            if (*shortest_average_waiting_time > station_average_waiting_time)
            {
                *shortest_average_waiting_time = station_average_waiting_time;
            }
        }
    }
}

double get_average_waiting_time(int num_green_stations, int green_station_waiting_times[][num_green_stations], int N)
{
    int i;
    int j;
    int total_waiting_time = 0;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < num_green_stations; j++) 
        {
            total_waiting_time += *green_station_waiting_times[i];
        }
    }
    double average_waiting_time;
    average_waiting_time = (double)total_waiting_time / (double)num_green_stations;
    average_waiting_time = average_waiting_time / (double)N;
    return average_waiting_time;
}

// Returns the index of the next station with the direction and previous station.
// index: line_station index
int get_next_station(int prev_station, int direction, int num_stations)
{
    if (direction == RIGHT)
    {
        // Reached the end of the station
        if (prev_station == num_stations - 1)
        {
            return prev_station -1;
        }
        return prev_station + 1;
    }
    // Reached the start of the station
    if (prev_station == 0)
    {
        return 1;
    }
    return prev_station - 1;
}

// Frees up the link so that other trains can come onboard.
void free_link(int num_stations, int current_station, int next_station, char *line_stations[], char *all_stations_list[], int links_status[][8])
{
    int current_all_station_index = get_all_station_index(num_stations, current_station, line_stations, all_stations_list);
    int next_all_station_index = get_all_station_index(num_stations, next_station, line_stations, all_stations_list);
    links_status[current_all_station_index][next_all_station_index] = LINK_IS_EMPTY;
}

// Returns the index of a station in the "all_station_list"
// index: all_station_list index
int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[])
{
    const char *name = line_stations[line_station_index];
    for (int i = 0; i < num_stations; i++)
    {
        if (strcmp(name, all_stations_list[i]) == 0)
        {
            return i;
        }
    }
}

int calculate_loadtime(double popularity)
{
    double random_number;
    random_number = (rand() % 10) + 1;
    
    return ceil(random_number * popularity);
}


int main(int argc, char *argv[])
{
    // TODO: READ VALUES FROM INPUT INSTEAD.
    int S = 8;                   // Number of train stations in the network
    char *all_stations_list[] = {// List of stations
                                 "changi",
                                 "tampines",
                                 "clementi",
                                 "downtown",
                                 "chinatown",
                                 "harborfront",
                                 "bedok",
                                 "tuas"};
    int link_transit_time[8][8] = {
        // Represents the grap - 1;
        {0, 3, 0, 0, 0, 0, 0, 0},
        {3, 0, 8, 6, 0, 2, 0, 0},
        {0, 8, 0, 0, 4, 0, 0, 5},
        {0, 6, 0, 0, 0, 9, 0, 0},
        {0, 0, 4, 0, 0, 0, 10, 0},
        {0, 2, 0, 9, 0, 0, 0, 0},
        {0, 0, 0, 0, 10, 0, 0, 0},
        {0, 0, 5, 0, 0, 0, 0, 0},
    };
    double all_stations_popularity_list[8] = {
        0.9, 0.5, 0.2, 0.3, 0.7, 0.8, 0.4, 0.1};
    char *G[] = {// Stations in the green line
                 "tuas",
                 "clementi",
                 "tampines",
                 "changi"};
    char *Y[] = {// Stations in the yellow line
                 "bedok",
                 "chinatown",
                 "clementi",
                 "tampines",
                 "harborfront"};
    char *B[] = {// Stations in the blue line
                 "changi",
                 "tampines",
                 "downtown",
                 "harborfront"};
    int N = 40;  // Number of time ticks in the simulation (Iterations)
    int total_lines = 1; // Number of lines operating innetwork
    int g = 4;  // Number of trains in green line
    int y = 10; // Number of trains in yellow line
    int b = 10; // Number of trains in blue line
    int i;
    int j;
    int k;

    // Initialize Link status. -1: Link is empty | 1: Link is used
    int links_status[S][S];
    for (i = 0; i < S; i++)
    {
        for (j = 0; j < S; j++)
        {
            links_status[i][j] = LINK_IS_EMPTY;
        }
    }

    // Initialize all trains,
    struct train_type green_trains[g];
    struct train_type yellow_trains[y];
    struct train_type blue_trains[b];
    struct train_type initial_train = {WAITING_TO_LOAD, NOT_IN_NETWORK, RIGHT, -1, -1};
    for (i = 0; i < g; i ++) 
    {
        green_trains[i] = initial_train;
    }
    for (i = 0; i < y; i ++) 
    {
        yellow_trains[i] = initial_train;
    }
    for (i = 0; i < b; i ++) 
    {
        blue_trains[i] = initial_train;
    }

    // Initialize all stations. -2 : unvisited | -1 : not loading | >= 0: index of loading train
    int num_green_stations = 4;
    int num_yellow_stations = 5;
    int num_blue_stations = 4;
    int green_stations[2][num_green_stations];
    int yellow_stations[2][num_yellow_stations];
    int blue_stations[2][num_blue_stations];
    for (i = 0; i < 2; i ++) 
    {
        for (j = 0; j < num_green_stations; j ++)
        {
            green_stations[i][j] = UNVISITED;
        }
    }
    for (i = 0; i < 2; i ++) 
    {
        for (j = 0; j < num_yellow_stations; j ++)
        {
            yellow_stations[i][j] = UNVISITED;
        }
    }
    for (i = 0; i < 2; i ++) 
    {
        for (j = 0; j < num_blue_stations; j ++)
        {
            blue_stations[i][j] = UNVISITED;
        }
    }

    // Initialize all arrays to track waiting time at each station of each line
    int green_station_waiting_times[2][num_green_stations];
    int yellow_station_waiting_times[2][num_yellow_stations];
    int blue_station_waiting_times[2][num_blue_stations];

    // Set the number of threads to be = number of trains
    int num_all_trains = g + y + b;
    omp_set_num_threads(num_all_trains);

    int time_tick;
    for (time_tick = 0; time_tick < N; time_tick++)
    {
        printf("Current time tick = %d\n", time_tick);
        // Entering the stations 1 time tick at a time.
        int i;
        // Boolean value to make sure that only 1 train enters the line at any time tick.
        int introduced_train_left = NOT_INTRODUCED;
        int introduced_train_right = NOT_INTRODUCED;

#pragma omp parallel for shared(introduced_train_left, introduced_train_right, green_stations, green_trains) private(i)
        // This iteration is going through all the trains in green line.
        for (i = 0; i < g; i++)
        {   
            if (green_trains[i].status == NOT_IN_NETWORK)
            {
                #pragma omp critical
                {
                    int starting_station = -1;
                    if (introduced_train_right == NOT_INTRODUCED) {
                        starting_station = 0;
                    } else if (introduced_train_left == NOT_INTRODUCED) {
                        starting_station = num_green_stations - 1;
                    }  

                    // Introducing a train into the network.
                    if (starting_station != -1)
                    {
                        if (starting_station == 0) {
                            introduced_train_right = INTRODUCED;
                            green_trains[i].direction = RIGHT;        
                        } else {
                            introduced_train_left = INTRODUCED;
                            green_trains[i].direction = LEFT;
                        }
                        green_trains[i].status = IN_STATION;
                        green_trains[i].station = starting_station;
                        if (green_stations[green_trains[i].direction][starting_station] == UNVISITED) {
                            green_stations[green_trains[i].direction][starting_station] = READY_TO_LOAD;
                        }
                        // If no trains are loading. We will start loading the introduced train immediately.
                        if (green_stations[green_trains[i].direction][starting_station] == READY_TO_LOAD)
                        {
                            green_stations[green_trains[i].direction][starting_station] = i;
                            int global_station_index = get_all_station_index(S, green_trains[i].station, G, all_stations_list);
                            green_trains[i].loading_time = calculate_loadtime(all_stations_popularity_list[global_station_index]) - 1; 
                            printf("Setting train %d as loading time %d.\n", i, green_trains[i].loading_time);
                        }
                    }
                }
            }
            else if (green_trains[i].status == IN_STATION)
            {
                // Decrement loading time and free up station if it is done loading
                if (green_trains[i].loading_time > 0)
                {
                    green_trains[i].loading_time--;
                }
                else if (green_trains[i].loading_time == FINISHED_LOADING)
                {
                    int current_station = green_trains[i].station;
                    int current_all_station_index = get_all_station_index(S, current_station, G, all_stations_list);
                    int next_station = get_next_station(current_station, green_trains[i].direction, num_green_stations);
                    int next_all_station_index = get_all_station_index(S, next_station, G, all_stations_list);
                    #pragma omp critical
                    {
                        // The link is currently occupied, wait in the station.
                        if (links_status[current_all_station_index][next_all_station_index] == LINK_IS_EMPTY)
                        {
                            // Link unoccupied, move train to the link
                            green_trains[i].transit_time = link_transit_time[current_all_station_index][next_all_station_index] - 1;
                            // printf("Transiting green train %d with transit time %d\n", i, green_trains[i].transit_time);
                            green_trains[i].status = IN_TRANSIT;
                            green_trains[i].loading_time = WAITING_TO_LOAD;
                            links_status[current_all_station_index][next_all_station_index] = LINK_IS_USED;
                        }
                    }
                }
                #pragma omp critical
                {
                    // Train is waiting on an empty station, we can start loading
                    if (green_trains[i].status == IN_STATION && green_trains[i].loading_time == WAITING_TO_LOAD && green_stations[green_trains[i].direction][green_trains[i].station] == READY_TO_LOAD)
                    {
                        int global_station_index = get_all_station_index(S, green_trains[i].station, G, all_stations_list);
                        green_trains[i].loading_time = calculate_loadtime(all_stations_popularity_list[global_station_index]) - 1;
                        green_stations[green_trains[i].direction][green_trains[i].station] = i;
                    }
                }
            }
            else if (green_trains[i].status == IN_TRANSIT)
            {
                green_trains[i].transit_time--;
            }
        }

        // Master thread consolidation:
        // Count the waiting times at each station.
        printf("~~~~~ END OF ITERATION ~~~~\n");
        for (i = 0; i < 2; i++) 
        {
            for (j = 0; j < num_green_stations; j++)
            {
                if (green_stations[i][j] == READY_TO_LOAD)
                {
                    printf("Station index %d (at %d) is waiting\n", i, j);
                    green_station_waiting_times[i][j] += 1;
                }
            }
        }

        // Free up stations where the loading train has just finished loading up passengers.
        for (i = 0; i < 2; i ++)
        {
            for (j = 0; j < num_green_stations; j++)
            {
                // printf("Station %d :145
                // With train index: %d, with train loading time: %d\n", i, green_stations[i], green_trains[green_stations[i]].loading_time);
                int green_train_index = green_stations[i][j];
                if (green_train_index >= 0)
                {
                    if (green_trains[green_train_index].loading_time == FINISHED_LOADING)
                    {
                        green_stations[i][j] = READY_TO_LOAD;
                    }
                }
            }
        }

        // Free up links where the loading train was on
        for (i = 0; i < g; i ++) {
            if (green_trains[i].status == IN_TRANSIT && green_trains[i].transit_time == 0)
            {
                // Move the train to the station
                int current_station = green_trains[i].station;
                int next_station = get_next_station(current_station, green_trains[i].direction, num_green_stations);
                // Get next direction based on where the train is going.
                int next_direction = green_trains[i].direction;
                if (next_station < current_station)
                {
                    next_direction = LEFT;
                } else 
                {
                    next_direction = RIGHT;
                }
                // Free up the link the train was on.
                free_link(S, current_station, next_station, G, all_stations_list, links_status);
                // Update Train
                green_trains[i].station = next_station;
                green_trains[i].direction = next_direction;
                green_trains[i].status = IN_STATION;
                printf("Train %d has landed in station index %d (at %d).", i, next_direction, next_station);
                // Update Stations
                if (green_stations[next_direction][next_station] == UNVISITED) 
                {
                    green_stations[next_direction][next_station] = READY_TO_LOAD;
                }
            }
        }
        print_status(green_trains, g, G, num_green_stations);
        printf("\n\n");
    }

    printf("~~~~~ END OF NETWORK!!! ~~~~~\n");
    for (i = 0; i < 2; i ++) 
    {
        for (j = 0; j < num_green_stations; j++) 
        {
            printf("(at %d) Station %d waiting time: %d\n", i, j, green_station_waiting_times[i][j]);
        }
    }

    double average_waiting_time = get_average_waiting_time(num_green_stations, green_station_waiting_times, N);
    double longest_average_waiting_time = 0;
    double shortest_average_waiting_time = INT_MAX;
    get_longest_shortest_average_waiting_time(num_green_stations, green_station_waiting_times, N, &longest_average_waiting_time, &shortest_average_waiting_time);
    printf("Average waiting time: %G | longest_average_waiting_time: %G | shortest_average_waiting_time: %G\n", average_waiting_time, longest_average_waiting_time, shortest_average_waiting_time);
}
