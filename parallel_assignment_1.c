/*
 * ASSUMPTIONS: 
 * 1. Only one train in a given direction and line at a time can load at a station. This means that if two trains from different lines are at the same station,
 * they can both load at the same time.
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
#define GREEN 1
#define BLUE 2
#define YELLOW 3

// Links
#define LINK_IS_EMPTY -1
#define LINK_IS_USED 1
#define LINK_DEFAULT_STATUS 0
#define FREE_THIS_LINK 2


// Direction
#define LEFT 0      // FROM END OF ARRAY TO START 
#define RIGHT 1     // FROM START OF ARRAY TO END

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
    int line;
};

// NOTE: THIS IS A CHEAT. Putting S as a global variable so that I can pass 2D arrays around.
const int S = 8;

// Function declaration: Updating network
void introduce_train_into_network(struct train_type *train, double all_stations_popularity_list[], int **line_stations, char *line_stations_name_list[], char *all_stations_list[], int num_stations, int num_network_train_stations, int train_number, int *introduced_train_left, int *introduced_train_right);
void in_station_action(struct train_type *train, int index_of_train, int S, char *line_train_station_names[], int **line_stations, char *all_stations_list[], int num_stations, double all_stations_popularity_list[], int link_transit_time[][S], int links_status[][S]);
void in_transit_action(struct train_type *train, int num_stations, int **line_stations, char* line_stations_name_list, char *all_stations_list, int **links_to_free) 
void clean_up_train_stations(int direction_index, int num_stations, int **train_stations, struct train_type trains[]);
void free_links(int **links_to_free, int **links_status);

// Function declaration: Calculating waiting time
double get_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N);
void get_longest_shortest_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N, double *longest_average_waiting_time, double *shortest_average_waiting_time);

// Function declaration: Helper functions
void print_status(struct train_type trains[], int num_trains, char *G[], int num_stations);
int get_next_station(int prev_station, int direction, int num_stations);
int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[]);
int calculate_loadtime(double popularity);
int change_train_direction(int direction);


// Functions: Updating network
void introduce_train_into_network(struct train_type *train, double all_stations_popularity_list[], int **line_stations, char *line_stations_name_list[], char *all_stations_list[], int num_stations, int num_network_train_stations, int train_number, int *introduced_train_left, int *introduced_train_right) {
    int starting_station = -1;
    if (*introduced_train_right == NOT_INTRODUCED) {
        starting_station = 0;
    } else if (*introduced_train_left == NOT_INTRODUCED) {
        starting_station = num_stations - 1;
        printf("Starting station: %d\n", starting_station);
    }

    // Introducing a train into the network.
    if (starting_station != -1) {
        if (starting_station == 0) {
            *introduced_train_right = INTRODUCED;
            trains[train_number].direction = RIGHT;        
        } else {
            *introduced_train_left = INTRODUCED;
            trains[train_number].direction = LEFT;
        }
        train->status = IN_STATION;
        train->station = starting_station;
        printf("I'M HERE SHOULD BE TWICE. TRAIN DIRECTION : %d, STARTING STATION: %d \n", train->.direction, starting_station);
        printf("CURRENT LINE STATUS = %d\n", line_stations[train->direction][starting_station]);
        if (line_stations[train->direction][starting_station] == UNVISITED) {
            line_stations[train->direction][starting_station] = READY_TO_LOAD;
        }        
        printf("After LINE STATUS = %d\n", line_stations[trains[train_number].direction][starting_station]);
        // If no trains are loading. We will start loading the introduced train immediately.
        if (line_stations[train->direction][starting_station] == READY_TO_LOAD) {
            line_stations[train->direction][starting_station] = train_number;                // The train number is the global train index. 
            int global_station_index = get_all_station_index(num_network_train_stations, train->station, line_stations_name_list, all_stations_list);
            train->loading_time = calculate_loadtime(all_stations_popularity_list[global_station_index]) - 1; 
            printf("Setting train %d as loading time %d.\n", train_number, train->loading_time);
        }
    }
}

void in_station_action(struct train_type *train, int train_number, int S, char *line_stations_name_list[], int **line_stations, char *all_stations_list[], int num_stations, double all_stations_popularity_list[], int link_transit_time[][S], int **links_status) {
    // This train is currently loading at a station.
    if (train->loading_time > 0) {
        train->loading_time--;
    } else if (train->loading_time == FINISHED_LOADING) {
        int current_station = train->station;
        int current_all_station_index = get_all_station_index(S, current_station, line_train_station_names, all_stations_list);
        int next_station = get_next_station(current_station, train->direction, num_stations);
        int next_all_station_index = get_all_station_index(S, next_station, line_train_station_names, all_stations_list);
        // TODO: CRITICAL SECTION?
        // Link is not occupied, move train into link.
        if (links_status[current_all_station_index][next_all_station_index] == LINK_IS_EMPTY) {
            train->transit_time = link_transit_time[current_all_station_index][next_all_station_index] - 1;
            train->status = IN_TRANSIT;
            train->loading_time = WAITING_TO_LOAD;
            links_status[current_all_station_index][next_all_station_index] = LINK_IS_USED;
        }
    }
    if (train->status == IN_STATION && train->loading_time == WAITING_TO_LOAD && line_stations[train->direction][train->station] == READY_TO_LOAD) {
        int global_station_index = get_all_station_index(S, train->station, line_train_station_names, all_stations_list);
        train->loading_time = calculate_loadtime(all_stations_popularity_list[global_station_index]) - 1;
        line_stations[train->direction][train->station] = train_number;                    // The train number is the global train index
    }
}

void in_transit_action(struct train_type *train, int num_stations, int **line_stations, char* line_stations_name_list, char *all_stations_list, int **links_to_free) {
    train->transit_time--;
    if (train->transit_time == 0) {
        // Move the train to the next station
        int prev_station = train->station;
        train->station = get_next_station(train->station, train->direction, num_stations);
        train->status = IN_STATION;
        // Update the direction of the train (For trains reaching a terminal station)
        if (next_station < train->station) {
            train->direction = LEFT;
        } else {
            train->direction = RIGHT;
        }
        // Update the station if this is the first time it is being visited
        if (line_stations[train->direction][train->station] == UNVISITED) 
        {
            line_stations[train->direction][train->station] = READY_TO_LOAD;
        }
        // Mark the link as free to be updated at the master thread.
        int current_all_station_index = get_all_station_index(num_stations, prev_station, line_stations_name_list, all_stations_list);
        int next_all_station_index = get_all_station_index(num_stations, train->station, line_stations_name_list, all_stations_list);
        links_to_free[current_all_station_index][next_all_station_index] = FREE_THIS_LINK;
    }
}

/**
 *  This function goes through the status of all the train stations and checks if any loading trains at the station
 *  has finished loading (loading_time == 0). If it is, then change the status to READY_TO_LOAD.
 */
void clean_up_train_stations(int direction_index, int num_stations, int **train_stations, struct train_type trains[]) {
    int i;
    int train_index;
    for (i = 0 ; i < num_stations; i++) {
        train_index = train_stations[direction_index][i];
        if (train_index >= 0) {
            if (trains[train_index].loading_time == FINISHED_LOADING) {
                train_stations[direction_index][i] = READY_TO_LOAD; 
            }
        }
    }
}

void free_links(int **links_to_free, int **links_status) {


    int i;
    int j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j ++) {
            if (links_to_free[i][j] == FREE_THIS_LINK) {
                links_status[i][j] = LINK_IS_EMPTY;
                links_to_free[i][j] = LINK_DEFAULT_STATUS;
            }
        }
    }
}

// Functions: Calculating waiting time
double get_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N) {
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

void get_longest_shortest_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N, double *longest_average_waiting_time, double *shortest_average_waiting_time) {
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

// Functions: Helper functions
int change_train_direction(int direction)  {
    direction += 1;
    return direction % 2;
}

void print_status(struct train_type trains[], int num_trains, char *G[], int num_stations) {
    int i;
    printf("\n~~~~~~~~~~ TRAIN STATUS ~~~~~~~~~~~\n");
    for (i = 0; i < num_trains; i++)
    {
        char *direction;
        if (trains[i].direction == LEFT) {
            direction = "Left";
        } else {
            direction = "Right";
        }

        if (trains[i].status == IN_STATION && trains[i].loading_time == WAITING_TO_LOAD)
        {
            printf("Train %d is currently in (%s) station %d | Waiting to load...\n", i, direction, trains[i].station);
        }
        else if (trains[i].status == IN_STATION) {
            printf("Train %d is currently in (%s) station %d | With %d ticks left to load\n", i, direction, trains[i].station, trains[i].loading_time);
        }
        else if (trains[i].status == IN_TRANSIT)
        {
            int current_station = trains[i].station;
            int next_station = get_next_station(current_station, trains[i].direction, num_stations);
            printf("Train %d is currently in transit %s->%s | With %d ticks left to transit.\n", i, G[current_station], G[next_station], trains[i].transit_time);
        }
    }
}

int get_next_station(int prev_station, int direction, int num_stations) {
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

int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[]) {
    const char *name = line_stations[line_station_index];
    for (int i = 0; i < num_stations; i++)
    {
        if (strcmp(name, all_stations_list[i]) == 0)
        {
            return i;
        }
    }
}

int calculate_loadtime(double popularity) {
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
    int N = 100;  // Number of time ticks in the simulation (Iterations)
    int total_lines = 1; // Number of lines operating innetwork
    int g = 4;  // Number of trains in green line
    int y = 10; // Number of trains in yellow line
    int b = 10; // Number of trains in blue line
    int i;
    int j;
    int k;

    // Initialize Link status. -1: Link is empty | 1: Link is used
    int links_status[S][S];
    for (i = 0; i < S; i++) {
        for (j = 0; j < S; j++) {
            links_status[i][j] = LINK_IS_EMPTY;
        }
    }
    // Set the number of threads to be = number of trains
    int num_all_trains = g + y + b;

    // Initialize all trains,
    struct train_type trains[num_all_trains];
    struct train_type initial_green_train = {WAITING_TO_LOAD, NOT_IN_NETWORK, RIGHT, -1, -1, GREEN};
    struct train_type initial_blue_train = {WAITING_TO_LOAD, NOT_IN_NETWORK, RIGHT, -1 , -1, BLUE};
    struct train_type initial_yellow_train = {WAITING_TO_LOAD, NOT_IN_NETWORK, RIGHT, -1, -1, YELLOW};
    for (i = 0; i < g; i++) {
        trains[i] = initial_green_train;
    }
    for (i = g; i < g + y; i ++) {
        trains[i] = initial_yellow_train;
    }
    for (i = g + y; i < g + y + b; i++) {
        trains[i] = initial_blue_train;
    }

    int num_green_stations = sizeof(G) / sizeof(G[0]);
    int num_yellow_stations = sizeof(Y) / sizeof(G[0]);
    int num_blue_stations = sizeof(B) / sizeof(B[0]);
    // INITIALISATION of arrays that keep track of the status of EACH station on EACH line in EACH direction.
    // If a station is occupied, it will store the GLOBAL INDEX of the train from the trains array.
    int *green_stations[2];
    int *yellow_stations[2];
    int *blue_stations [2];
    for (i = 0 ; i < 2; i++) {
        green_stations[i] = (int*)malloc(num_green_stations * sizeof(int));
        yellow_stations[i] = (int*)malloc(num_yellow_stations * sizeof(int));
        blue_stations[i] = (int*)malloc(num_blue_stations * sizeof(int));
    }
    for (i = 0; i < 2; i ++) {
        for (j = 0; j < num_green_stations; j++){
            green_stations[i][j] = UNVISITED;
        }
        for (j = 0; j < num_yellow_stations; j++) {
            yellow_stations[i][j] = UNVISITED;
        }
        for (j = 0; j < num_blue_stations; j++) {
            blue_stations[i][j] = UNVISITED;
        }
    }

    // INITIALISATION of arrays that keep track of waiting time.
    int *green_station_waiting_times[2];
    int *yellow_station_waiting_times[2];
    int *blue_station_waiting_times[2];
    for (i = 0; i < 2; i++){
        green_station_waiting_times[i] = (int*)malloc(num_green_stations * sizeof(int));
        yellow_station_waiting_times[i] = (int*)malloc(num_yellow_stations * sizeof(int));
        blue_station_waiting_times[i] = (int*)malloc(num_blue_stations * sizeof(int));
    }
    for (i = 0; i < 2; i++) {
        for (j = 0 ; j < num_green_stations; j++ ) {
            green_station_waiting_times[i][j] = 0;
        }
        for (j = 0 ; j < num_yellow_stations; j++) {
            yellow_station_waiting_times[i][j] = 0;
        }
        for (j = 0 ; j < num_blue_stations; j++) {
            blue_station_waiting_times[i][j] = 0;
        }
    }

    // INITALISATION of 2D array to keep track of which link to free up. If an entry is 1 it means that a train just finished transitting in the link. 0 otherwise.
    int links_to_free[8][8];
    for (i = 0; i < 8; i ++) {
        for (j = 0; j < 8; j++) {
            links_to_free = LINK_DEFAULT_STATUS;
        }
    }

    // INITIALISATION of thread
    omp_set_num_threads(num_all_trains);

    int time_tick;
    for (time_tick = 0; time_tick < N; time_tick++) {
        // Entering the stations 1 time tick at a time.
        int i;
        // Boolean value to make sure that only 1 train enters the line at any time tick.
        // TODO(LOWJIANSHENG): Have an introduced boolean for every single line.
        int introduced_train_left = NOT_INTRODUCED;
        int introduced_train_right = NOT_INTRODUCED;
        // Count the number of idle trains at the start of each iteration. Since READY_TO_LOAD will only be accurately updated after each iteration
        for (i = 0; i < 2; i++) {
            char *c;
            if (i == 0) {
                c = "left";
            } else {
                c = "right";
            }
            for (j = 0; j < num_green_stations; j++) {
                if (green_stations[i][j] == READY_TO_LOAD) {
                    printf("(%s) Station %d is waiting\n", c, j);
                    green_station_waiting_times[i][j] += 1;
                }
            }
        }
    
    #pragma omp parallel for shared(introduced_train_left, introduced_train_right, green_stations, green_trains) private(i)
        // Each Parallel thread will take up a train
        for (i = 0; i < num_all_trains; i++) {
            // Initialization of each train(thread)
            struct train_type current_train = trains[i];
            int *line_stations[];
            int num_stations;
            char *line_stations_name_list[];
            if (current_train.line == GREEN) {
                line_stations = green_stations;
                num_stations = num_green_stations;
                line_stations_name_list = G; 
            } else if (current_train.line == BLUE) {
                line_stations = blue_stations;
                num_stations = num_blue_stations;
                line_stations_name_list = B;
            } else {
                line_stations = yellow_stations;
                num_stations = num_yellow_stations;
                line_stations_name_list = Y;
            }
            // Move the train by a "tick" and update the status of the network
            if (current_train.status == NOT_IN_NETWORK) {
                #pragma omp critical
                {   
                    introduce_train_into_network(current_train, all_stations_popularity_list, line_stations, line_stations_name_list, all_stations_list, num_stations, S, i, &introduced_train_left, &introduced_train_right);
                }
            }
            else if (current_train.status == IN_STATION) {
                in_station_action(&current_train, i, S, line_stations_name_list, line_stations, all_stations_list, num_stations, all_stations_popularity_list, link_transit_time, links_status);
            }
            else if (current_train.status == IN_TRANSIT) {
                in_transit_action(&current_train, num_stations, line_stations, line_stations_name_list, all_stations_list, links_to_free);
            }
        }

        // Master thread
        printf("~~~~~ END OF ITERATION %d ~~~~\n", time_tick);
        // Free up stations where the loading train has just finished loading up passengers.
        for (i = 0 ; i < 2; i++) {
            clean_up_train_stations(i, num_green_stations, green_stations, trains);
            clean_up_train_stations(i, num_blue_stations, blue_stations, trains);
            clean_up_train_stations(i, num_yellow_stations, yellow_stations, trains);
        }
        // Free up the links which were just used by trains if any.
        free_links(links_to_free, links_status);
        print_status(green_trains, g, G, num_green_stations);
        printf("\n\n");
    }

    printf("~~~~~ END OF NETWORK!!! ~~~~~\n");
    for (i = 0; i < 2; i ++) {
        for (j = 0; j < num_green_stations; j++) {
            printf("(at %d) Station %d waiting time: %d\n", i, j, green_station_waiting_times[i][j]);
        }
    }

    double average_waiting_time = get_average_waiting_time(num_green_stations, green_station_waiting_times, N);
    double longest_average_waiting_time = 0;
    double shortest_average_waiting_time = INT_MAX;
    get_longest_shortest_average_waiting_time(num_green_stations, green_station_waiting_times, N, &longest_average_waiting_time, &shortest_average_waiting_time);
    printf("Average waiting time: %G | longest_average_waiting_time: %G | shortest_average_waiting_time: %G\n", average_waiting_time, longest_average_waiting_time, shortest_average_waiting_time);
}
