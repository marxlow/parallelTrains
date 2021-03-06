#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

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
#define LINK_DEFAULT_STATUS -2

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

// Parallel variables
#define MASTER_ID slaves
// (For links)
#define LINK_ROW_INDEX 0
#define LINK_COL_INDEX 1
#define LINK_STATUS_INDEX 2
#define LINK_TRANSIT_TIME_INDEX 3
// (For trains)
#define TRAIN_DIRECTION_INDEX 0
#define TRAIN_LINE_INDEX 1
#define TRAIN_LOADING_TIME_INDEX 2
#define TRAIN_STATION_INDEX 3
#define TRAIN_STATUS_INDEX 4
#define TRAIN_TRANSIT_TIME_INDEX 5

int slaves;

struct train_type
{
    int loading_time; // -1 waiting to load | 0 has loaded finish at the station| > 0 for currently loading
    int status;       // 1 for in transit | 0 for in station | -1 for not in network
    int direction;    // 1 for up  | 0 for down
    int station;      // -1 for not in any station | > 0 for index of station it is in
    int transit_time; // -1 for NA | > 0 for in transit
    int line;
};

// Parallel variables
int myid;

// Function declaration: Updating network
void introduce_train_into_network(struct train_type *train, double all_stations_popularity_list[], int **line_stations, char *line_stations_name_list[], char *all_stations_list[], int num_stations, int num_network_train_stations, int train_number, int *introduced_train_left, int *introduced_train_right);

// Function declaration: Calculating waiting time
double get_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N);
void get_longest_shortest_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N, double *longest_average_waiting_time, double *shortest_average_waiting_time);

// Function declaration: Helper functions
int calculate_loadtime(double popularity);
int get_all_station_index(int num_all_stations, int line_station_index, char *line_stations[], char *all_stations_list[]);
int get_next_station(int prev_station, int direction, int num_stations);
void print_output(int iteration, struct train_type trains[], int num_trains, char *G[], char *Y[], char *B[], int num_green_trains, int num_yellow_trains, int num_blue_trains, char *all_stations_list[], int num_all_stations, int num_green_stations, int num_yellow_stations, int num_blue_stations, FILE* fp);

// Function declaration: Slaves
void master(int S, int **links_status, struct train_type trains[], int num_trains, int **link_transit_time);
void slave(int num_trains, int num_stations, char *G[], char *Y[], char *B[);

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
            train->direction = LEFT;
        } else {
            *introduced_train_left = INTRODUCED;
            train->direction = RIGHT;
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
        }
    }
}

// Functions: Calculating waiting time
double get_average_waiting_time(int num_stations, int **station_waiting_times, int N) {
    int i;
    int j;
    int total_waiting_time = 0;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < num_stations; j++) {
            // Skip LEFT ending terminal
            if (i == 0 && j == num_stations -1) {
                continue;
            }
            // Skip RIGHT starting terminal
            if (i == 1 && j == 0) {
                continue;
            }
            total_waiting_time += station_waiting_times[i][j];
        }
    }
    double average_waiting_time;
    average_waiting_time = (double)total_waiting_time / (double)num_stations / (double)2;
    average_waiting_time = average_waiting_time / (double)N;
    return average_waiting_time;
}
void get_longest_shortest_average_waiting_time(int num_stations, int **station_waiting_times, int N, double *longest_average_waiting_time, double *shortest_average_waiting_time) {
    int i;
    int j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < num_stations; j++) {
            // Skip LEFT ending terminal
            if (i == 0 && j == num_stations -1) {
                continue;
            }
            // Skip RIGHT starting terminal
            if (i == 1 && j == 0) {
                continue;
            }
            double waiting_time = (double)station_waiting_times[i][j];
            double station_average_waiting_time = waiting_time / (double)N;
            // Update waiting times
            if (*longest_average_waiting_time < station_average_waiting_time) {
                *longest_average_waiting_time = station_average_waiting_time;
            }
            if (*shortest_average_waiting_time > station_average_waiting_time) {
                *shortest_average_waiting_time = station_average_waiting_time;
            }
        }
    }
}

// Functions: Helper functions
int calculate_loadtime(double popularity) {
    double random_number;
    random_number = (rand() % 10) + 1;
    return ceil(random_number * popularity);
}
int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[]) {
    for (int i = 0; i < num_stations; i++) {   
        if (strcmp(line_stations[line_station_index], all_stations_list[i]) == 0) {
            return i;
        }
    }
    return -1; // Error return code - this is to fix the compile error that no return type is specified
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

void print_output(int iteration, struct train_type trains[], int num_trains, char *G[], char *Y[], char *B[], int num_green_trains, int num_yellow_trains, int num_blue_trains, char *all_stations_list[], int num_all_stations, int num_green_stations, int num_yellow_stations, int num_blue_stations, FILE* fp) {
    int i;
    int train_index;
    int current_station_index;
    int prev_station_index;
    fprintf(fp, "%d:", iteration);
    // Print satus of all green trains first
    for (i = 0; i < num_green_trains; i++) {
        if (trains[i].status == NOT_IN_NETWORK) {
            continue;
        }
        else if (trains[i].status == IN_STATION) {
            current_station_index = get_all_station_index(num_all_stations, trains[i].station, G, all_stations_list);
            fprintf(fp, " g%d-s%d,", i, current_station_index);
        } 
        else if (trains[i].status == IN_TRANSIT) {
            prev_station_index = get_all_station_index(num_all_stations, trains[i].station, G, all_stations_list);
            current_station_index = get_next_station(trains[i].station, trains[i].direction, num_green_stations);
            current_station_index = get_all_station_index(num_all_stations, current_station_index, G, all_stations_list);
            fprintf(fp, " g%d-s%d->s%d,", i, prev_station_index, current_station_index);
        }
    }
    // Yellow
    for (i = num_green_trains; i < num_green_trains + num_yellow_trains; i++) {
        train_index = i - num_green_trains;
        if (trains[i].status == NOT_IN_NETWORK) {
            continue;
        }
        else if (trains[i].status == IN_STATION) {
            current_station_index = get_all_station_index(num_all_stations, trains[i].station, Y, all_stations_list);
            fprintf(fp, " y%d-s%d,", train_index, current_station_index);
        } else if (trains[i].status == IN_TRANSIT) {
            prev_station_index = get_all_station_index(num_all_stations, trains[i].station, Y, all_stations_list);
            current_station_index = get_next_station(trains[i].station, trains[i].direction, num_yellow_stations);
            current_station_index = get_all_station_index(num_all_stations, current_station_index, Y, all_stations_list);
            fprintf(fp, " y%d-s%d->s%d,", train_index, prev_station_index, current_station_index);
        }
    }
    for (i = num_green_trains + num_yellow_trains; i < num_trains; i++) {
        train_index = i - num_green_trains - num_yellow_trains;
        if (trains[i].status == NOT_IN_NETWORK) {
            continue;
        }
        else if (trains[i].status == IN_STATION) {
            current_station_index = get_all_station_index(num_all_stations, trains[i].station, B, all_stations_list);
            fprintf(fp, " b%d-s%d,", train_index, current_station_index);
        } else if (trains[i].status == IN_TRANSIT) {
            prev_station_index = get_all_station_index(num_all_stations, trains[i].station, B, all_stations_list);
            current_station_index = get_next_station(trains[i].station, trains[i].direction, num_blue_stations);
            current_station_index = get_all_station_index(num_all_stations, current_station_index, B, all_stations_list);
            fprintf(fp, " b%d-s%d->s%d,", train_index, prev_station_index, current_station_index);
        }
    }
    fprintf(fp, "\n");
}

// Functions: Master Slaves
// This function distributes respective link statuses to the slave thread and 
void master(int S, int **links_status, struct train_type trains[], int num_trains, int **link_transit_time) {
    int num_slaves = 0;
    int slave_id = 0;
    int row_id;
    int col_id;

    // Send link statuses to the slaves
    for (row_id = 0; row_id < S; row_id++) {
        for (col_id = 0; col_id < S; col_id++) {
            // A link exists between station: row_id and station: col_id
            if (link_transit_time[row_id][col_id] != 0) {
                // Send to slave thread an array containing information about the link. 
                // [0] row_id, aka starting station
                // [1] col_id, aka destination station
                // [2] link status
                // [3] link transit time
                int link_information[4] = {row_id, col_id, links_status[row_id][col_id], link_transit_time[row_id][col_id]};
                MPI_Send(link_information, 4, MPI_INTEGER, slave_id, row_id, MPI_COMM_WORLD);
                slave_id++;
            }
        }
    }

    // Send over the list of trains to the slaves
    printf("+++ MASTER: Now sending all the trains to the slaves.");
    num_slaves = slave_id;
    int i;
    int j;
    slave_id = 0;
    for (i = 0 ; i < num_trains; i++) {
        int train_status[6] = { trains[i].direction, trains[i].line, trains[i].loading_time, trains[i].station, trains[i].status, trains[i].transit_time};
        for (slave_id = 0; slave_id < num_slaves; slave_id++) {
            MPI_SEND(train_status, 6, MPI_INTEGER, slave_id, i, MPI_COMM_WORLD);
        }
    }

    // Receive results back from slaves
    fprintf(stderr, " +++ MASTER : Now receiving results back from the slaves\n");
}

void slave(int num_trains) {
    MPI_Status status;
    int i;
    int size = 4;
    int link_information_buffer[4];
    int trains_buffer[num_trains][6];
    int train_id;
    // ------------------ RECEIVE DATA ------------------
    // Link data
    int row_id;
	MPI_Recv(&link_information_buffer, size, MPI_INTEGER, MASTER_ID, row_id, MPI_COMM_WORLD, &status);

    // Trains Data
    size = 6;
    for (i = 0; i < num_trains; i++) {
        MPI_Recv(&trains_buffer[i], size, MPI_INTEGER, MASTER_ID, train_id, MPI_COMM_WORLD, &status);
    }

    // ------------------ Compute data ------------------
    int link_row_index = link_information_buffer[LINK_ROW_INDEX];
    int link_col_index = link_information_buffer[LINK_COL_INDEX];
    int link_status = link_information_buffer[LINK_STATUS_INDEX];
    int link_transit_time = link_information_buffer[LINK_TRANSIT_TIME_INDEX];

// int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[]) {
// }
// int get_next_station(int prev_station, int direction, int num_stations) {
// }
    if (link_status == LINK_IS_USED) {
        // Find the train that is related to this link;
        for (i = 0; i < num_trains; i++) {
            if (trains_buffer[i][TRAIN_STATUS_INDEX] != IN_TRANSIT) {
                continue;
            }
            int index_of_train = link_status;
            int transit_time_of_train = trains_buffer[index_of_train][TRAIN_TRANSIT_TIME_INDEX] --;
            // Simply decrement and return
            if (transit_time_of_train != 0) {
                int[6] updated_train_information = {
                    trains_buffer[index_of_train][TRAIN_DIRECTION_INDEX],
                    trains_buffer[index_of_train][TRAIN_LINE_INDEX],
                    trains_buffer[index_of_train][TRAIN_LOADING_TIME_INDEX],
                    trains_buffer[index_of_train][TRAIN_STATION_INDEX],
                    trains_buffer[index_of_train][TRAIN_STATUS_INDEX],
                    trainsit_time_of_train
                }
            }


        }
    } else if (link_status == LINK_IS_EMPTY) {

    } else {
        // Do nothing.
    }

// // (For trains)
// #define TRAIN_DIRECTION_INDEX 0
// #define TRAIN_LINE_INDEX 1
// #define TRAIN_LOADING_TIME_INDEX 2
// #define TRAIN_STATION_INDEX 3
// #define TRAIN_STATUS_INDEX 4
// #define TRAIN_TRANSIT_TIME_INDEX 5
    // Send data
}

int main(int argc, char *argv[]) {
    int i;
    int j;
    int time_tick;

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
   
    // Creating all stations list.
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
    slaves = 0; // To initialize what the Master ID should be.
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
            if (int_value != 0) {
                slaves++;
            }
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
    for (i = 0 ; i < num_green_stations; i++ ){
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
    for (i = 0 ; i < num_yellow_stations; i++ ){
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
    for (i = 0 ; i < num_blue_stations; i++ ){
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

    // Get count of number of trains and close file pointer
    fgets(c, 1000, fptr);
    int N = atoi(c); 
    fgets(c, 1000, fptr);

    value = strtok(c, delimiter);
    int g = atoi(value);
    value = strtok(NULL, delimiter);
    int y = atoi(value);
    value = strtok(NULL, delimiter);
    int b = atoi(value);
    fclose(fptr);
    
    //---------------------------- PARSING INPUT FROM THE INPUT FILE. -------------------------------//
    // INITIALISATION of link statuses.
    int *links_status[S];
    for (i = 0; i < S; i++) {
        links_status[i] = (int*)malloc(S * sizeof(int));
    }
    for (i = 0; i < S; i++) {
        for (j = 0; j < S; j++) {
            links_status[i][j] = LINK_IS_EMPTY;
        }
    }

    // INITIALISATION of the status of all the trains.
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

    // INITIALISATION of arrays that keep track of the status of the station on each line.
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

    // INITIALISATION of 1d Array to keep track of the status of each station
    int station_status[S];
    for (i = 0; i < S; i ++) {
        station_status[i] = READY_TO_LOAD;
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

    // INITIALISATION of logs
    FILE* fp = fopen("log.txt", "w");

    // INITIALISATION of clock
    clock_t before = clock();
    int msec;

    // STEP 0: ---------------------------- START NETWORK ----------------------------
    for (time_tick = 0; time_tick < N; time_tick++) {

        // STEP 1: ---------------------------- INTRODUCE TRAINS ----------------------------
        int introduced_train[2][3]; // keeps track of at every iteration if a train has been introduced into the line.
        for (i = 0 ; i < 2; i++) {
            for (j = 0 ; j < 3; j++) {
                introduced_train[i][j] = NOT_INTRODUCED;
            }
        }
        for (i = 0 ; i < num_all_trains; i++) {
            // Initialising variables for each train's line.
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
            if (trains[i].status == NOT_IN_NETWORK) {
                introduce_train_into_network(&trains[i], all_stations_popularity_list, line_stations, line_stations_name_list, all_stations_list, num_stations, S, i, introduced_train_left, introduced_train_right);
            }
        }

        // STEP 2: ---------------------------- PARALLEL (Update Links) ----------------------------
        // 1. Pick up trains from stations and put them onto any empty link.
        // 2. Decrease travelling time in the link if the link is occupied.
        // 3. When travelling time decreased to 0, move train into station.
        // Initialize parallel parameters
	    int nprocs;
	    MPI_Init(&argc,&argv);
	    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

        // Send the data of each link_status to a thread
        if (myid == MASTER_ID) {
		    fprintf(stderr, " +++ Process %d is master\n", myid);
		    master(S, links_status, trains, num_all_trains, link_transit_time);
	    } else {
		    fprintf(stderr, " --- Process %d is slave\n", myid);
		    slave();
	    }

	    MPI_Finalize();
        //---------------------------- END OF PARALLEL CODE -------------------------------//
        // MASTER THREAD.
        // 1. For every station, choose train that has not loaded yet to load. (Maybe use a queue or randomize.)
        // 2. For every station that has a train which is loading, decrement the loading time. 
        // 3. Count the waiting times of stations.

        // Go through all the trains. If a train is in a station and not yet loaded, check the station status.
        // If the station is without any train loading, load it. 
        // note(lowjiansheng): At the parallel code, have to make sure to change station status to READY_TO_LOAD
        for (i = 0 ; i < num_all_trains; i++) {
            if (trains[i].status == IN_STATION) {
                int **line_stations;
                int all_stations_index;
                // note(lowjiansheng): The station index here is probably the local station index?
                // NEED SOME WAY TO RANDOMIZE THIS. Else yellow and blue line trains might get starved.
                if (trains[i].line == GREEN) {
                    line_stations = green_stations;
                    all_stations_index = get_all_station_index(num_all_trains, trains[i].station, G, all_stations_list);
                }
                else if (trains[i].line == BLUE) {
                    line_stations = blue_stations;
                    all_stations_index = get_all_station_index(num_all_trains, trains[i].station, B, all_stations_list);
                }
                else {
                    line_stations = yellow_stations;
                    all_stations_index = get_all_station_index(num_all_trains, trains[i].station, Y, all_stations_list);
                }
                // Load the current train in the station.
                if (station_status[all_stations_index] == READY_TO_LOAD){
                    station_status[all_stations_index] = LOADING;
                    int load_time = calculate_loadtime(123);
                    trains[i].loading_time = load_time;
                    // note(lowjiansheng): do we need to use the green_stations / blue_stations / yellow_stations arrays?
                    line_stations[trains[i].direction][trains[i].station] = i;
                }
            } 
            else if (trains[i].status == LOADING) {
                trains[i].loading_time--;
            }
        }

        // Count the waiting times of stations.
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
        // Log current iteration information to an output file
        print_output(time_tick, trains, num_all_trains, G, Y, B, g, y, b, all_stations_list, S, num_green_stations, num_yellow_stations, num_blue_stations, fp);
    }
    // Close clock for time
    clock_t difference = clock() - before;
    msec = difference * 1000 / CLOCKS_PER_SEC;
    printf("Time taken: %d seconds %d milliseconds\n", msec/1000, msec%1000);

    // Get waiting time
    double green_longest_average_waiting_time = 0;
    double green_shortest_average_waiting_time = INT_MAX;
    double green_average_waiting_time = get_average_waiting_time(num_green_stations, green_station_waiting_times, N);
    get_longest_shortest_average_waiting_time(num_green_stations, green_station_waiting_times, N, &green_longest_average_waiting_time, &green_shortest_average_waiting_time);

    double yellow_longest_average_waiting_time = 0;
    double yellow_shortest_average_waiting_time = INT_MAX;
    double yellow_average_waiting_time = get_average_waiting_time(num_yellow_stations, yellow_station_waiting_times, N);
    get_longest_shortest_average_waiting_time(num_yellow_stations, yellow_station_waiting_times, N, &yellow_longest_average_waiting_time, &yellow_shortest_average_waiting_time);
    
    double blue_longest_average_waiting_time = 0;
    double blue_shortest_average_waiting_time = INT_MAX;
    double blue_average_waiting_time = get_average_waiting_time(num_blue_stations, blue_station_waiting_times, N);
    get_longest_shortest_average_waiting_time(num_blue_stations, blue_station_waiting_times, N, &blue_longest_average_waiting_time, &blue_shortest_average_waiting_time);
    
    fprintf(fp, "\nAverage waiting times:\n");
    fprintf(fp, "green: %d trains -> %lf, %lf, %lf\n", g, green_average_waiting_time, green_longest_average_waiting_time, green_shortest_average_waiting_time);
    fprintf(fp, "yellow: %d trains -> %lf, %lf, %lf\n", y, yellow_average_waiting_time, yellow_longest_average_waiting_time, yellow_shortest_average_waiting_time);
    fprintf(fp, "blue: %d trains -> %lf, %lf, %lf", b, blue_average_waiting_time, blue_longest_average_waiting_time, blue_shortest_average_waiting_time);

    // Close file for logs
    fclose(fp);
}

