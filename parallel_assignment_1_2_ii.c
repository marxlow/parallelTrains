/**
 * CS3210 - Matrix Multiplication in MPI
 **/

// ASSUMPTIONS:
// 1. Number of processes = Number of links in the network. Else the program will not work.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
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

struct train_type
{
    int loading_time; // -1 waiting to load | 0 has loaded finish at the station| > 0 for currently loading
    int status;       // 1 for in transit | 0 for in station | -1 for not in network
    int direction;    // 1 for up  | 0 for down
    int station;      // -1 for not in any station | > 0 for index of station it is in
    int transit_time; // -1 for NA | > 0 for in transit
    int line;
};

int slaves;
int myid;
#define MASTER_ID slaves


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


/*************************************************************************************************************************************/

/**
 * Function used by the slaves to receive data from the master
 * Each slave receives a link_status_buffer which is an array containing value of the link
 * and a num_slaves x 6 matrix containing information of the train in the network
 **/
void slave_receive_data(int link_information_buffer[5], int **trains_information_buffer) {
    // [0] row_id, aka starting station
    // [1] col_id, aka destination station
    // [2] link status
	// [3] link transit time
	// [4] num_trains;
	int num_trains;
	int i;
	MPI_Status status;
	// Receiving information regarding the status of the link.
	MPI_Recv(&link_information_buffer, 5, MPI_INTEGER, MASTER_ID, 1, MPI_COMM_WORLD, &status);
	num_trains = link_information_buffer[4];

	for (i = 0 ; i < num_trains; i++){
		trains_information_buffer[i] = (int*)(7 * sizeof(int));
	}
	// Receiving a 7 * num_trains array containing information about the trains.
	// [0] current station of the train
	// [1] next station of the train
	// [2] line of the train
    // [3] loading time of the train
	// [4] transit time of the train
	// [5] status of the train
	// [6] global index of the train
	for (i = 0 ; i < num_trains; i++) {
		MPI_Recv(&trains_information_buffer[i], 7, MPI_INTEGER, MASTER_ID, i, MPI_COMM_WORLD, &status);
	}
}

/** 
 * Function used by the slaves to compute the update to the network.
 **/
void slave_compute(int link_information_buffer[], int **trains_information_buffer, int train_to_return[]) {
	// there are no trains in the link, so loop through the trains to find one that can enter.
	if (link_information_buffer[2] == LINK_IS_EMPTY){
		int num_trains = link_information_buffer[4];
		int i;
		// TODO(lowjiansheng): Probably need to randomize, this current implementation WILL cause starvation.
		for (i = 0 ; i < num_trains ; i++) {
			if (trains_information_buffer[i][0] == link_information_buffer[0] &&
				trains_information_buffer[i][1] == link_information_buffer[1] && 
				trains_information_buffer[i][5] == IN_STATION &&
				trains_information_buffer[i][3] == 0) {
				train_to_return[0] = trains_information_buffer[i][6];
				train_to_return[1] = IN_TRANSIT;
				train_to_return[2] = link_information_buffer[3];
				// Setting the link to store global index of train. Remember to return this to master and store.
				link_information_buffer[2] = trains_information_buffer[6];
				break;
			}
		}
	} 
	else if (link_information_buffer[2] == LINK_DEFAULT_STATUS){
		printf("There is an error. Master should not be sending over a link that does not exist.\n");
	}
	else {
		int index_of_train = link_information_buffer[2];
		// Decrement the transit time of the train. 
		// note(lowjiansheng): The result will be sent over as a size 3 array. It is a subset of the train struct.
		// [0]: global index of the train
		// [1]: status of train
		// [2]: transit time of train
		train_to_return[0] = index_of_train;
		train_to_return[1] = trains_information_buffer[index_of_train][5];
		train_to_return[2] = trains_information_buffer[index_of_train][4] - 1;
		// The link will have to be vacant for the next train to come in.
		if (trains_information_buffer[index_of_train][4] - 1 == 0) {
			link_information_buffer[2] = LINK_IS_EMPTY;
		}
	}
}

/**
 * Function used by the slaves to send the updated train/link back to the master
 **/
void slave_send_result() {
}


/**
 * Main function called by slaves
 *
 **/
void slave() {
	int link_information_buffer[5];
	int **trains_information_buffer;
	int train_to_return[7];
	// Receive data
	slave_receive_data(link_information_buffer, trains_information_buffer);

	// Doing the computations
	slave_compute(link_information_buffer, trains_information_buffer, train_to_return);

	// Sending the results back
	slave_send_result();
}


/*************************************************************************************************************************************/

/**
 * Function called by the master to distribute link_status
 * and the entire trains array to the child
 **/
void master_distribute(int S, int **links_status, struct train_type trains[], int num_trains, int **link_transit_time, int G[], int Y[], int B[], char *all_stations_list[]) {
    int i, j, k;
	int row_id, col_id;
	int slave_id = 0;
	int num_links;
	// Send link statuses to the slaves
	printf("+++ MASTER: Now sending over link statuses to the slaves.");
    for (row_id = 0; row_id < S; row_id++) {
        for (col_id = 0; col_id < S; col_id++) {
            // A link exists between station: row_id and station: col_id
            if (link_transit_time[row_id][col_id] != 0) {
                // Send to slave thread an array containing information about the link. 
                // [0] row_id, aka starting station
                // [1] col_id, aka destination station
                // [2] link status
                // [3] link transit time
				// [4] num trains
                int link_information[5] = {row_id, col_id, links_status[row_id][col_id], link_transit_time[row_id][col_id], num_trains};
                MPI_Send(link_information, 5, MPI_INTEGER, slave_id, 1, MPI_COMM_WORLD);
                slave_id++;
            }
        }
    }
	// Send over the list of trains to the slaves
    printf("+++ MASTER: Now sending all the trains to the slaves.");
    num_links = slave_id;		// note(lowjiansheng): The number of links is the same as the number of slaves.
    int i;
    int j;
    slave_id = 0;
	// Send to slave thread an array containing information about the link. 
	// [0] current station of the train
	// [1] next station of the train
	// [2] line of the train
    // [3] loading time of the train
	// [4] transit time of the train
	// [5] status of the train
	// [6] global index of the train
    for (i = 0 ; i < num_trains; i++) {
		int current_station;
		int next_station;
		if (trains[i].line == GREEN){
			current_station = get_all_station_index(S, trains[i].station, G,  all_stations_list);
			next_station = get_all_station_index(S, get_next_station(trains[i].station, trains[i].direction, S), G, all_stations_list);
		}
		else if (trains[i].line == BLUE) {
			current_station = get_all_station_index(S, trains[i].station, B,  all_stations_list);
			next_station = get_all_station_index(S, get_next_station(trains[i].station, trains[i].direction, S), B, all_stations_list);	
		}
		else {
			current_station = get_all_station_index(S, trains[i].station, Y,  all_stations_list);
			next_station = get_all_station_index(S, get_next_station(trains[i].station, trains[i].direction, S), Y, all_stations_list);	
		}
        int train_status[7] = { current_station, next_station, trains[i].line, trains[i].loading_time, trains[i].transit_time, trains[i].status, i};
        for (slave_id = 0; slave_id < num_links; slave_id++) {
            MPI_SEND(train_status, 7, MPI_INTEGER, slave_id, i, MPI_COMM_WORLD);
        }
    }
}

/**
 * Receives the result array information from the slaves
 **/
void master_receive_result(int S, int num_trains, int num_link, int **link_transit_time) {
	MPI_Status status;
	// Receive results back from slaves
    fprintf(stderr, " +++ MASTER : Now receiving results back from the slaves\n");
	// Master waits for an array describing the train link status and an array of the train that has been modified.
	int i, j;
	int slave_id = 0;
	int link_information_buffer[4];
	int train_information_buffer[6];
	// Wait for an array describing the train link status.
	for (i = 0 ; i < S; i++) {
		for (j = 0 ; j < S; j++) {
			if (link_transit_time[i][j] != 0) {
				MPI_Recv(&link_information_buffer, 4, MPI_INTEGER, slave_id , i, MPI_COMM_WORLD, &status);
				// TODO(lowjiansheng): Update links status with the information from link_information_buffer
				slave_id++;
			}
		}
	}
	// Wait for an array describing the only train that has been modified by the link. 
	// note(lowjiansheng): The tag_id will be the slave_id as each slave will only return 1 train that has been modified.
	for (slave_id = 0 ; slave_id < num_link; slave_id++) {
		MPI_Recv(&link_information_buffer, 4, MPI_INTEGER, slave_id, slave_id, MPI_COMM_WORLD, &status);
		// TODO(lowjiansheng): Update train status with the information from link_information_buffer.
	}
}

/**
 * Main function called by the master process
 *
 **/
void master() {
	int i, j, k;
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

    //---------------------------- INITIALISATION OF STATUS TRACKING ARRAYS -------------------------------//
	
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
    //---------------------------- INITIALISATION OF STATUS TRACKING ARRAYS -------------------------------//

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

		// Distribute data to slaves
		master_distribute(S, links_status, trains, num_all_trains, link_transit_time, G, Y, B);
		// Gather results from slaves
		master_receive_result();

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
	}
}



/*************************************************************************************************************************************/

/**
 * Train network using master-slave paradigm
 * The master initializes and sends the data to the 
 * The slaves represent a link each. And sends the necessary update to the network
 * Number of slaves must be equal to the number of links
 * Total number of processes is 1 + number of slaves
 **/
int main(int argc, char ** argv)
{
	int nprocs;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	// One master and nprocs-1 slaves
	slaves = nprocs - 1;

	if (myid == MASTER_ID) {
		fprintf(stderr, " +++ Process %d is master\n", myid);
		master();
	}
	else {
		fprintf(stderr, " --- Process %d is slave\n", myid);
		slave();
	}
    
	MPI_Finalize();
	return 0;
}
