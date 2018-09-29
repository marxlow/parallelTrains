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
#define GREEN 0
#define BLUE 1
#define YELLOW 2

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


// Function declaration: Updating network
void introduce_train_into_network(struct train_type *train, double all_stations_popularity_list[], int **line_stations, char *line_stations_name_list[], char *all_stations_list[], int num_stations, int num_network_train_stations, int train_number, int *introduced_train_left, int *introduced_train_right);
void in_station_action(struct train_type *train, int train_number, int S, char *line_stations_name_list[], int **line_stations, char *all_stations_list[], int num_stations, double all_stations_popularity_list[], int **link_transit_time, int **links_status);
void in_transit_action(struct train_type *train, int num_stations, int S, int **line_stations, char* line_stations_name_list[], char *all_stations_list[], int **links_status_update);
void update_train_stations(int direction_index, int num_stations, int **train_stations, struct train_type trains[]);
void update_links_status(int **links_status_update, int **links_status, int S;

// Function declaration: Calculating waiting time
double get_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N);
void get_longest_shortest_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N, double *longest_average_waiting_time, double *shortest_average_waiting_time);

// Function declaration: Helper functions
void print_status(struct train_type trains[], int num_trains, char *G[], int num_stations, int line);
int get_next_station(int prev_station, int direction, int num_stations);
int get_all_station_index(int num_all_stations, int line_station_index, char *line_stations[], char *all_stations_list[]);
int calculate_loadtime(double popularity);
int change_train_direction(int direction);


// Functions: Updating network
void introduce_train_into_network(struct train_type *train, double all_stations_popularity_list[], int **line_stations, char *line_stations_name_list[], char *all_stations_list[], int num_stations, int num_network_train_stations, int train_number, int *introduced_train_left, int *introduced_train_right) {
    int starting_station = -1;
    if (*introduced_train_right == NOT_INTRODUCED) {
        starting_station = 0;
    } else if (*introduced_train_left == NOT_INTRODUCED) {
        starting_station = num_stations - 1;
    }
    // Introducing a train into the network.
    if (starting_station != -1) {
        if (starting_station == 0) {
            *introduced_train_right = INTRODUCED;
            train->direction = RIGHT;        
        } else {
            *introduced_train_left = INTRODUCED;
            train->direction = LEFT;
        }
        train->status = IN_STATION;
        train->station = starting_station;
        if (line_stations[train->direction][starting_station] == UNVISITED) {
            line_stations[train->direction][starting_station] = READY_TO_LOAD;
        }        
        // If no trains are loading. We will start loading the introduced train immediately.
        if (line_stations[train->direction][starting_station] == READY_TO_LOAD) {
            line_stations[train->direction][starting_station] = train_number;                // The train number is the global train index. 
            int global_station_index = get_all_station_index(num_network_train_stations, train->station, line_stations_name_list, all_stations_list);
            train->loading_time = calculate_loadtime(all_stations_popularity_list[global_station_index]) - 1; 
            //printf("Setting train %d as loading time %d.\n", train_number, train->loading_time);
        }
    }
}

void in_station_action(struct train_type *train, int train_number, int S, char *line_stations_name_list[], int **line_stations, char *all_stations_list[], int num_stations, double all_stations_popularity_list[], int **link_transit_time, int **links_status) {
    // This train is currently loading at a station.
    if (train->loading_time > 0) {
        train->loading_time--;
    } else if (train->loading_time == FINISHED_LOADING) {
        int current_station = train->station;
        int current_all_station_index = get_all_station_index(S, current_station, line_stations_name_list, all_stations_list);
        int next_station = get_next_station(current_station, train->direction, num_stations);
        int next_all_station_index = get_all_station_index(S, next_station, line_stations_name_list, all_stations_list);
        // TODO: CRITICAL SECTION?
        // Link is not occupied, move train into link.
        //printf("The link here is: links_status[%d][%d] = %d", current_all_station_index, next_all_station_index, links_status[current_all_station_index][next_all_station_index]);
        
        if (links_status[current_all_station_index][next_all_station_index] == LINK_IS_EMPTY) {
            train->transit_time = link_transit_time[current_all_station_index][next_all_station_index] - 1;
            train->status = IN_TRANSIT;
            train->loading_time = WAITING_TO_LOAD;
            links_status[current_all_station_index][next_all_station_index] = LINK_IS_USED;
        }
    }
    // Load a waiting train
    if (train->status == IN_STATION && train->loading_time == WAITING_TO_LOAD && line_stations[train->direction][train->station] == READY_TO_LOAD) {
        int global_station_index = get_all_station_index(S, train->station, line_stations_name_list, all_stations_list);
        train->loading_time = calculate_loadtime(all_stations_popularity_list[global_station_index]) - 1;
        line_stations[train->direction][train->station] = train_number; // The train number is the global train index
    }
}

void in_transit_action(struct train_type *train, int num_stations, int S, int **line_stations, char* line_stations_name_list[], char *all_stations_list[], int **links_status_update) {
    train->transit_time--;
    if (train->transit_time == 0) {
        // Move the train to the next station
        int prev_station;
        prev_station = train->station;
        train->station = get_next_station(train->station, train->direction, num_stations);
        train->status = IN_STATION;
        // Update the direction of the train (For trains reaching a terminal station)
        if (prev_station < train->station) {
            train->direction = RIGHT;
        } else {
            train->direction = LEFT;
        }
        // Update the station if this is the first time it is being visited
        // But will cause counting problem?
        if (line_stations[train->direction][train->station] == UNVISITED) 
        {
            line_stations[train->direction][train->station] = READY_TO_LOAD;
        }
        // Mark the link as free to be updated at the master thread.
        int current_all_station_index = get_all_station_index(S, prev_station, line_stations_name_list, all_stations_list);
        int next_all_station_index = get_all_station_index(S, train->station, line_stations_name_list, all_stations_list);
        links_status_update[current_all_station_index][next_all_station_index] = FREE_THIS_LINK;
    }
}

/**
 *  This function goes through the status of all the train stations and checks if any loading trains at the station
 *  has finished loading (loading_time == 0). If it is, then change the status to READY_TO_LOAD.
 */
void update_train_stations(int direction_index, int num_stations, int **train_stations, struct train_type trains[]) {
    int i;
    int train_index;
    for (i = 0 ; i < num_stations; i++) {
        train_index = train_stations[direction_index][i];
        if (train_index >= 0 && trains[train_index].loading_time == FINISHED_LOADING) {
            // printf("Train %d Has finished loading at station [%d][%d]", train_index, direction_index, i);
            train_stations[direction_index][i] = READY_TO_LOAD; 
        }
    }
}

void update_links_status(int **links_status_update, int **links_status, int S) {
    int i;
    int j;
    for (i = 0; i < S; i++) {
        for (j = 0; j < S; j ++) {
            if (links_status_update[i][j] == FREE_THIS_LINK) {
                links_status[i][j] = LINK_IS_EMPTY;
                links_status_update[i][j] = LINK_DEFAULT_STATUS;
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

void print_status(struct train_type trains[], int num_all_trains, char *line_station_names[], int num_stations, int line) {
    int i;
    printf("\n~~~~~~~~~~ TRAIN STATUS ~~~~~~~~~~~\n");
    for (i = 0; i < num_all_trains; i++) {
        if (trains[i].line != line){
            continue;
        }
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
            printf("Train %d is currently in transit %s->%s | With %d ticks left to transit.\n", i, line_station_names[current_station], line_station_names[next_station], trains[i].transit_time);
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
    for (int i = 0; i < num_stations; i++) {   
        if (strcmp(line_stations[line_station_index], all_stations_list[i]) == 0) {
            return i;
        }
    }
    printf("WEIRDDDDD...\n");
}

int calculate_loadtime(double popularity) {
    double random_number;
    random_number = (rand() % 10) + 1;
    return ceil(random_number * popularity);
}


int main(int argc, char *argv[]) {
    int i;
    int j;
    int k;

    //---------------------------- PARSING INPUT FROM THE INPUT FILE. -------------------------------//
    char c[1000];
    FILE *fptr;
    if ((fptr = fopen("input.txt", "r")) == NULL)
    {
        printf("Error! opening file");
        // Program exits if file pointer returns NULL.
        exit(1);         
    }
    fgets(c, 1000, fptr);
    int S = atoi(c);

    fgets(c, 1000, fptr);
    char *all_stations_list[S];
    const char delimiter[2] = ",";
    char *station;             // pointer to the first character of station token.
    station = strtok(c, delimiter);
    for (i = 0 ; i < S; i++) {
        all_stations_list[i] = station;
        station = strtok(NULL, delimiter);
    }
    // REALLOCATE THE VALUES OF ALL STATIONS LISt.
    for (i = 0 ; i < S; i++ ){
        char *station = all_stations_list[i];
        int index = 0;
        char *station_value = malloc(100*sizeof(char));
        while (station[index] != '\0') {
            if (station[index] == '\n'){
                break;
            }
            station_value[index] = station[index];
            index++;
        }
        station_value[index] = '\0';
        all_stations_list[i] = station_value;  
    }
    // S x S matrix denoting the link transit time.
    int *link_transit_time[S];
    for (i = 0 ; i < S; i++) {
        link_transit_time[i] = (int*)malloc(S * sizeof(int));
    }
    const char space_delimiter[2] = " ";
    char *value;
    int int_value;
    // fgets S number of times.
    for (i = 0 ; i < S; i++) {
        fgets(c, 1000, fptr);
        value = strtok(c, space_delimiter);
        int_value = atoi(value);
        for (j = 0 ; j < S; j++) {
            int_value = atoi(value);
            link_transit_time[i][j] = int_value;
            value = strtok(NULL, space_delimiter);
        }
    }

    // POPULARITY LIST.
    double *all_stations_popularity_list = malloc(S * sizeof(double));
    double double_value;
    fgets(c, 1000, fptr);
    value = strtok(c, space_delimiter);
    sscanf(value, "%lf", &double_value);
    for (i = 0 ; i < S; i++) {
        sscanf(value, "%lf", &double_value);
        all_stations_popularity_list[i] = double_value;
        value = strtok(NULL, space_delimiter);
    }
    
    // GREEN TRAIN STATION LIST.
    char *temp_G[S];
    fgets(c, 1000, fptr);
    station = strtok(c, delimiter);
    i = 0;
    int num_green_stations = 0;
    while (station != NULL){
        temp_G[num_green_stations] = station;
        num_green_stations++;
        station = strtok(NULL, delimiter);
    }
    char *G[num_green_stations];
    for (i = 0 ; i < num_green_stations; i++) {
        G[i] = temp_G[i];
    }
    for (i = 0 ; i < S; i++ ){
        char *station = G[i];
        int index = 0;
        char *station_value = malloc(100*sizeof(char));
        while (station[index] != '\0') {
            if (station[index] == '\n'){
                break;
            }
            station_value[index] = station[index];
            index++;
        }
        station_value[index] = '\0';
        G[i] = station_value;  
    }


    // BLUE TRAIN STATION LIST
    char *temp_B[S];
    fgets(c, 1000, fptr);
    station = strtok(c, delimiter);
    int num_blue_stations = 0;
    while (station != NULL){
        temp_B[num_blue_stations] = station;
        num_blue_stations++;
        station = strtok(NULL, delimiter);
    }
    char *B[num_blue_stations];
    for (i = 0 ; i < num_blue_stations; i++) {
        B[i] = temp_B[i];
    }
    for (i = 0 ; i < S; i++ ){
        char *station = B[i];
        int index = 0;
        char *station_value = malloc(100*sizeof(char));
        while (station[index] != '\0') {
            if (station[index] == '\n'){
                break;
            }
            station_value[index] = station[index];
            index++;
        }
        station_value[index] = '\0';
        B[i] = station_value;  
    }

    // YELLOW TRAIN STATION LIST
    char *temp_Y[S];
    fgets(c, 1000, fptr);
    station = strtok(c, delimiter);
    int num_yellow_stations = 0;
    while (station != NULL){
        temp_Y[num_yellow_stations] = station;
        num_yellow_stations++;
        station = strtok(NULL, delimiter);
    }
    char *Y[num_yellow_stations];
    for (i = 0 ; i < num_yellow_stations; i++) {
        Y[i] = temp_Y[i];
    }
    for (i = 0 ; i < S; i++ ){
        char *station = Y[i];
        int index = 0;
        char *station_value = malloc(100*sizeof(char));
        while (station[index] != '\0') {
            if (station[index] == '\n'){
                break;
            }
            station_value[index] = station[index];
            index++;
        }
        station_value[index] = '\0';
        Y[i] = station_value;  
    }  

    fgets(c, 1000, fptr);
    int N = atoi(c); 
    fgets(c, 1000, fptr);
    strtok(c, delimiter);
    int g = atoi(c);
    strtok(NULL, delimiter);
    int y = atoi(c);
    strtok(NULL, delimiter);
    int b = atoi(c);

    fclose(fptr);

    printf("~~ALL STATIONS~~\n");
    for (i = 0 ; i < S; i ++ ) {
        printf("%s, ", all_stations_list[i]);
    }
    printf("\n");
    
    printf("\n");
    printf("~~GREEN~~\n");
    for (i = 0; i < num_green_stations; i++) {
        printf("%s, ", G[i]);
    }
    
    printf("\n");
    printf("~~BLUE~~\n");
    for (i = 0 ; i < num_blue_stations; i++) {
        printf("%s, ", B[i]);
    }
    printf("\n");
    printf("~~YELLOW~~\n");
    for (i = 0 ; i < num_yellow_stations; i++) {
        printf("%s, ", Y[i]);
    }
    printf("\n");
    //---------------------------- PARSING INPUT FROM THE INPUT FILE. -------------------------------//


    // Initialize Link status. -1: Link is empty | 1: Link is used
    int *links_status[S];
    for (i = 0; i < S; i++) {
        links_status[i] = (int*)malloc(S * sizeof(int));
    }
    for (i = 0; i < S; i++) {
        for (j = 0; j < S; j++) {
            links_status[i][j] = LINK_IS_EMPTY;
        }
    }
    for (i = 0 ; i < S; i++ ) {
        for (j = 0 ; j < S; j++) {
            printf("%d ", links_status[i][j]);
        }
        printf("\n");
    }

    // Initialize all trains,
    int num_all_trains = g + y + b;
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

    //int num_green_stations = sizeof(G) / sizeof(G[0]);
    //int num_yellow_stations = sizeof(Y) / sizeof(Y[0]);
    //int num_blue_stations = sizeof(B) / sizeof(B[0]);
    // INITIALISATION of arrays that keep track of the status of EACH station on EACH line in EACH direction.
    // If a station is occupied, it will store the GLOBAL INDEX of the train from the trains array.
    int *green_stations[2];
    int *yellow_stations[2];
    int *blue_stations[2];
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
    int *links_status_update[S];
    for (i = 0; i < S; i ++) {
        links_status_update[i] = (int*)malloc(S * sizeof(int));
    }
    for (i = 0; i < S; i ++) {
        for (j = 0; j < S; j++) {
            links_status_update[i][j] = LINK_DEFAULT_STATUS;
        }
    }
    // INTIALISATION of 2D array to keep track of station visits

    // INITIALISATION of thread
    omp_set_num_threads(num_all_trains);

    int time_tick;
    for (time_tick = 0; time_tick < N; time_tick++) {
        // Entering the stations 1 time tick at a time.
        int i;
        int j;
        // Boolean value to make sure that only 1 train enters the line at any time tick.
        // Introduced train keeps track of at every iteration if a train has been introduced into the line.
        int introduced_train[2][3];
        for (i = 0 ; i < 2; i++) {
            for (j = 0; j < 3; j++) {
                introduced_train[i][j] = NOT_INTRODUCED;
            }
        }
    #pragma omp parallel for shared(introduced_train, green_stations, yellow_stations, blue_stations, trains) private(i)
        // Each Parallel thread will take up a train
        for (i = 0; i < num_all_trains; i++) {
            // Initialization of each train(thread)
            // struct train_type trains[i] = trains[i];
            int **line_stations;
            char **line_stations_name_list;
            int num_stations;
            int *introduced_train_left;
            int *introduced_train_right;
            if (trains[i].line == GREEN) {
                introduced_train_left = &introduced_train[LEFT][GREEN];
                introduced_train_right = &introduced_train[RIGHT][GREEN];
                line_stations = green_stations;
                line_stations_name_list = G; 
                num_stations = num_green_stations;
            } else if (trains[i].line == BLUE) {
                introduced_train_left = &introduced_train[LEFT][BLUE];
                introduced_train_right = &introduced_train[RIGHT][BLUE];
                line_stations = blue_stations;
                line_stations_name_list = B;
                num_stations = num_blue_stations;
            } else {
                introduced_train_left = &introduced_train[LEFT][YELLOW];
                introduced_train_right = &introduced_train[RIGHT][YELLOW];
                line_stations = yellow_stations;
                line_stations_name_list = Y;
                num_stations = num_yellow_stations;
            }
            // printf("~~~~ DEBUG: Train %d | Status: %d", i, trains[i].status);
            // Move the train by a "tick" and update the status of the network
            if (trains[i].status == NOT_IN_NETWORK) {
                #pragma omp critical
                {   
                    introduce_train_into_network(&trains[i], all_stations_popularity_list, line_stations, line_stations_name_list, all_stations_list, num_stations, S, i, introduced_train_left, introduced_train_right);
                }
            }
            else if (trains[i].status == IN_STATION) {
                #pragma omp critical 
                {
                    in_station_action(&trains[i], i, S, line_stations_name_list, line_stations, all_stations_list, num_stations, all_stations_popularity_list, link_transit_time, links_status);
                }
            }
            else if (trains[i].status == IN_TRANSIT) {
                in_transit_action(&trains[i], num_stations, S, line_stations, line_stations_name_list, all_stations_list, links_status_update);
            }
        }

        // Master thread
        printf("~~~~~ END OF ITERATION %d ~~~~\n", time_tick);
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
                    green_station_waiting_times[i][j] += 1;
                }
            }
            for (j = 0; j < num_yellow_stations; j++) {
                if (yellow_stations[i][j] == READY_TO_LOAD) {
                    yellow_station_waiting_times[i][j] += 1;
                }
            }
            for (j = 0; j < num_blue_stations; j++) {
                if (blue_stations[i][j] == READY_TO_LOAD) {
                    blue_station_waiting_times[i][j] += 1;
                }
            }
        }
        // Free up stations where the loading train has just finished loading up passengers.
        for (i = 0 ; i < 2; i++) {
            update_train_stations(i, num_green_stations, green_stations, trains);
            update_train_stations(i, num_blue_stations, blue_stations, trains);
            update_train_stations(i, num_yellow_stations, yellow_stations, trains);
        }
        // Free up the links which were just used by trains if any.
        update_links_status(links_status_update, links_status, S);
        printf("~~~~ GREEN TRAINS ~~~~~");
        print_status(trains, num_all_trains, G, num_green_stations, GREEN);
        printf("~~~~ YELLOW TRAINS ~~~~~");
        print_status(trains, num_all_trains, Y, num_yellow_stations, YELLOW);
        printf("~~~~ BLUE TRAINS ~~~~~");
        print_status(trains, num_all_trains, B, num_blue_stations, BLUE);
        printf("\n\n");
    }

    printf("~~~~~ END OF NETWORK!!! ~~~~~\n");
    printf("~~~~~ GREEN WAITING TIMES ~~~~~\n");
    for (i = 0; i < 2; i ++) {
        for (j = 0; j < num_green_stations; j++) {
            printf("(at %d) Station %d waiting time: %d\n", i, j, green_station_waiting_times[i][j]);
        }
    }

    printf("~~~~~ YELLOW WAITING TIMES ~~~~~\n");
    for (i = 0; i < 2; i ++) {
        for (j = 0; j < num_yellow_stations; j++) {
            printf("(at %d) Station %d waiting time: %d\n", i, j, yellow_station_waiting_times[i][j]);
        }
    }

    printf("~~~~~ BLUE WAITING TIMES ~~~~~\n");
    for (i = 0; i < 2; i ++) {
        for (j = 0; j < num_blue_stations; j++) {
            printf("(at %d) Station %d waiting time: %d\n", i, j, blue_station_waiting_times[i][j]);
        }
    }

    double green_average_waiting_time = get_average_waiting_time(num_green_stations, green_station_waiting_times, N);
    double yellow_average_waiting_time = get_average_waiting_time(num_yellow_stations, yellow_station_waiting_times, N);
    double blue_average_waiting_time = get_average_waiting_time(num_blue_stations, blue_station_waiting_times, N);
    printf("Green waiting time: %G \n Yellow waiting time: %G \n Blue waiting time: %G \n", green_average_waiting_time, yellow_average_waiting_time, blue_average_waiting_time);
    // double longest_average_waiting_time = 0;
    // double shortest_average_waiting_time = INT_MAX;
    // get_longest_shortest_average_waiting_time(num_green_stations, green_station_waiting_times, N, &longest_average_waiting_time, &shortest_average_waiting_time);
    // printf("Average waiting time: %G | longest_average_waiting_time: %G | shortest_average_waiting_time: %G\n", average_waiting_time, longest_average_waiting_time, shortest_average_waiting_time);
}
