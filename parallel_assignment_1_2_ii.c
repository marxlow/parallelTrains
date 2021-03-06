/**
 * CS3210 - Matrix Multiplication in MPI
 **/

// ASSUMPTIONS:
// 1. Number of processes = Number of links in the network. Else the program will not work.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>
#include <limits.h>

// Train Status
#define IN_TRANSIT 1
#define IN_STATION 0
#define NOT_IN_NETWORK -1

#define GREEN 0
#define BLUE 1
#define YELLOW 2

// Links
#define LINK_IS_EMPTY -1
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
#define TRAIN_INFORMATION_TAG 1
#define LINK_INFORMATION_TAG 2
// (For links)
#define MSG_LINK_ROW_ID 0
#define MSG_LINK_COL_ID 1
#define MSG_LINK_STATUS 2
#define MSG_LINK_TRANSIT_TIME 3
#define NUM_TRAINS 4

// (receiving trains)
#define MSG_TRAIN_CURRENT_STATION 0
#define MSG_TRAIN_NEXT_STATION 1
#define MSG_TRAIN_LINE 2
#define MSG_TRAIN_LOADING_TIME 3
#define MSG_TRAIN_TRANSIT_TIME 4
#define MSG_TRAIN_STATUS 5
#define MSG_TRAIN_GLOBAL 6

struct train_type
{
    int loading_time; // -1 waiting to load | 0 has loaded finish at the station| > 0 for currently loading
    int status;       // 1 for in transit | 0 for in station | -1 for not in network
    int direction;    // 1 for up  | 0 for down
    int station;      // -1 for not in any station | > 0 for index of station it is in
    int transit_time; // -1 for NA | > 0 for in transit
    int line;
};

int num_trains;
int slaves;
int myid;
int num_blue_stations;
int num_green_stations;
int num_yellow_stations;

#define MASTER_ID slaves

// Function Declarations
void introduce_train_into_network(struct train_type *train, double all_stations_popularity_list[], int **line_stations, char *line_stations_name_list[], char *all_stations_list[], int num_stations, int num_network_train_stations, int train_number, int *introduced_train_left, int *introduced_train_right);
int calculate_loadtime(double popularity);
int get_all_station_index(int num_stations, int line_station_index, char *line_stations[], char *all_stations_list[]);
int get_next_station(int prev_station, int direction, int num_stations);
void print_output(int iteration, struct train_type trains[], int num_trains, char *G[], char *Y[], char *B[], int num_green_trains, int num_yellow_trains, int num_blue_trains, char *all_stations_list[], int num_all_stations, int num_green_stations, int num_yellow_stations, int num_blue_stations, FILE* fp);

// Function declaration: Calculating waiting time
double get_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N);
void get_longest_shortest_average_waiting_time(int num_green_stations, int **green_station_waiting_times, int N, double *longest_average_waiting_time, double *shortest_average_waiting_time);

// Function Declarations: MPI related
int** slave_receive_data(int link_information_buffer[], int **trains_information_buffer);
void slave_compute(int link_information_buffer[], int **trains_information_buffer, int train_to_return[]);
void slave_send_result(int link_information_buffer[], int train_to_return[], int link_info_size, int train_to_return_size);
void slave();
void master_distribute(int S, int **links_status, struct train_type trains[], int num_trains, int **link_transit_time, char *G[], char *Y[], char *B[], char *all_stations_list[]);
void master_receive_result(int S, int station_status[], int **link_status, int **link_transit_time, struct train_type trains[], char *G[], char *Y[], char *B[], char *all_stations_list[], int **green_stations, int **yellow_stations, int **blue_stations);
void master();

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
        train->loading_time = WAITING_TO_LOAD;
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
    if (direction == RIGHT) {
        // Reached the end of the station
        if (prev_station == num_stations - 1)
        {
            return prev_station -1;
        }
        return prev_station + 1;
    }
    // Reached the start of the station
    if (prev_station == 0) {
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
            if (i == 1 && j == num_stations -1) {
                continue;
            }
            // Skip RIGHT starting terminal
            if (i == 0 && j == 0) {
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

/*************************************************************************************************************************************/

/**
 * Function used by the slaves to receive data from the master
 * Each slave receives a MSG_LINK_STATUS_buffer which is an array containing value of the link
 * and a num_slaves x 6 matrix containing information of the train in the network
 **/
// note(lowjiansheng): slave is receiving data well
int** slave_receive_data(int link_information_buffer[], int **trains_information_buffer) {
    // [0] row_id, aka starting station
    // [1] col_id, aka destination station
    // [2] link status or train index
	// [3] link transit time
	// [4] num_trains;
	int i;
    int num_trains;
	MPI_Status status;
	// Receiving information regarding the status of the link.
	MPI_Recv(link_information_buffer, 5, MPI_INT, MASTER_ID, 1, MPI_COMM_WORLD, &status);
    num_trains = link_information_buffer[4];
    trains_information_buffer = (int **)malloc(num_trains * sizeof(int*));
    // TODO: The code is not getting past the below for loop.
    // This is because of how trains_information_buffer is initialized.
    // Look to the slave() function to try and debug.
	for (i = 0 ; i < num_trains; i++){
		trains_information_buffer[i] = (int*)malloc(7 * sizeof(int));
	}
	// Receiving a 7 * num_trains array containing information about the trains.
	// [0] current all station of the train
	// [1] next all station of the train
	// [2] line of the train
    // [3] loading time of the train
	// [4] transit time of the train
	// [5] status of the train
	// [6] global index of the train
	for (i = 0 ; i < num_trains; i++) {
        MPI_Recv(trains_information_buffer[i], 7, MPI_INT, MASTER_ID, i, MPI_COMM_WORLD, &status);
	}
    return trains_information_buffer;
}

/** 
 * Function used by the slaves to compute the update to the network.
 **/
void slave_compute(int link_information_buffer[], int **trains_information_buffer, int train_to_return[]) {
    train_to_return[0] = -1; // Set this to -1 to indicate that initially no train is entering the link
	if (link_information_buffer[2] == READY_TO_LOAD){
        int num_trains = link_information_buffer[4];
        int train_to_link_buffer[num_trains];
        int buffer_index = 0;
		int i;
		for (i = 0 ; i < num_trains ; i++) {
            if (trains_information_buffer[i][MSG_TRAIN_STATUS] == NOT_IN_NETWORK) {
                continue;
            }
            //fprintf(stderr, "Slave %d going through i = %d\n",myid, i);
            //fprintf(stderr, "%d\n", trains_information_buffer[i][0]);
            /*
            fprintf(stderr, "%d %d %d %d %d %d %d\n", trains_information_buffer[i][0],
                                                        trains_information_buffer[i][1],
                                                        trains_information_buffer[i][2],
                                                        trains_information_buffer[i][3],
                                                        trains_information_buffer[i][4],
                                                        trains_information_buffer[i][5],
                                                        trains_information_buffer[i][6]);
			*/
            // fprintf(stderr, "Slave %d got here at i = %d, Global index of train is: %d\n", myid, i, trains_information_buffer[i][MSG_TRAIN_GLOBAL]);
            if (trains_information_buffer[i][MSG_TRAIN_CURRENT_STATION] == link_information_buffer[MSG_LINK_ROW_ID] && // Train current station is link's (from)
				trains_information_buffer[i][MSG_TRAIN_NEXT_STATION] == link_information_buffer[MSG_LINK_COL_ID] &&  // Train next station is link's (to)
				trains_information_buffer[i][MSG_TRAIN_STATUS] == IN_STATION && // Train in station
				trains_information_buffer[i][MSG_TRAIN_LOADING_TIME] == FINISHED_LOADING) { // Train has finished loading in station and is ready to move up a link
                // Put train index in buffer to be randomly popped
                train_to_link_buffer[buffer_index] = i;
                buffer_index ++;
			}
		}
        // No trains are waiting to board this link.
        if (buffer_index == 0) {
            return;
        } else {
            // Initialize random number seed
            time_t t;
            srand((unsigned) time(&t));
            // Get random buffer index which holds all the train index that can board the link. This will give us a random train.
            int random_buffer_index = rand() % buffer_index;
            int random_train_index = train_to_link_buffer[random_buffer_index];
            // Update buffers with train & link information
            train_to_return[0] = trains_information_buffer[random_train_index][MSG_TRAIN_GLOBAL];
			train_to_return[1] = IN_TRANSIT;
			train_to_return[2] = link_information_buffer[MSG_LINK_TRANSIT_TIME];
			link_information_buffer[MSG_LINK_STATUS] = trains_information_buffer[random_train_index][MSG_TRAIN_GLOBAL];
            // note(Marx) : Below is the debug statement to ensure a random train is chosen
            // if (buffer_index > 1) {
            //     fprintf(stderr, "\n\nAlert");
            //     int i = 0;
            //     for (i = 0; i < buffer_index; i ++) {
            //         fprintf(stderr, " | Train %d", train_to_link_buffer[i]);
            //     }
            //     fprintf(stderr, " | Are waiting to board the link [%d, %d]\n", link_information_buffer[MSG_LINK_ROW_ID], link_information_buffer[MSG_LINK_COL_ID]);
            //     fprintf(stderr, "We are randomly choosing, train index [%d] to board \n\n\n", random_train_index);
            // }
        }

	} 
	else if (link_information_buffer[2] == LINK_DEFAULT_STATUS){
		printf("There is an error. Master should not be sending over a link that does not exist.\n");
	}
	// There is a train in the link. We have to decrement the transit time of the train in the link.
	else {
		int index_of_train = link_information_buffer[MSG_LINK_STATUS]; // Remember that link_status stores the index of the train on it as well.
		// Decrement the transit time of the train. 
		// note(lowjiansheng): The result will be sent over as a size 3 array. It is a subset of the train struct.
		// [0]: global index of the train
		// [1]: status of train
		// [2]: transit time of train
        int updated_transit_time = trains_information_buffer[index_of_train][MSG_TRAIN_TRANSIT_TIME] - 1;
        int updated_train_status = trains_information_buffer[index_of_train][MSG_TRAIN_STATUS];
        
		train_to_return[0] = index_of_train;
		train_to_return[1] = updated_train_status;
		train_to_return[2] = updated_transit_time;
		// The link will have to be vacant for the next train to come in.
		if (updated_transit_time == 0) {
			link_information_buffer[MSG_LINK_STATUS] = LINK_IS_EMPTY;
		}
	}
    return;
}

/**
 * Function used by the slaves to send the updated train/link back to the master
 **/
void slave_send_result(int link_information_buffer[], int train_to_return[], int link_info_size, int train_to_return_size) {
    // Send the Train info
    MPI_Send(train_to_return, train_to_return_size, MPI_INT, MASTER_ID, TRAIN_INFORMATION_TAG, MPI_COMM_WORLD);
    // Send the Link info
    MPI_Send(link_information_buffer, link_info_size, MPI_INT, MASTER_ID, LINK_INFORMATION_TAG, MPI_COMM_WORLD);
}

/**
 * Main function called by slaves
 *
 **/
void slave() {
    int link_info_size = 5;
    int train_info_size = 7;
    int train_to_return_size = 3;

	int link_information_buffer[link_info_size];
	int **trains_information_buffer;
    // Information to return to master
	int train_to_return[train_to_return_size];
    int time_tick = 0;
	// Receive data
    while (1){
        trains_information_buffer = slave_receive_data(link_information_buffer, trains_information_buffer);
        // Doing the computations
        slave_compute(link_information_buffer, trains_information_buffer, train_to_return);
        // Sending the results back
        slave_send_result(link_information_buffer, train_to_return, link_info_size, train_to_return_size);
        time_tick++;
    }

}


/*************************************************************************************************************************************/

/**
 * Function called by the master to distribute link_status
 * and the entire trains array to the child
 **/
void master_distribute(int S, int **links_status, struct train_type trains[], int num_trains, int **link_transit_time, char *G[], char *Y[], char *B[], char *all_stations_list[]) {
    int i, j, k;
	int row_id, col_id;
	int slave_id = 0;
	int num_links;
	// Send link statuses to the slaves
    // fprintf(stderr, "+++ Master: Now sending link informations to slaves.\n");
    int temp_count = 0;

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
                MPI_Send(link_information, 5, MPI_INT, slave_id, 1, MPI_COMM_WORLD);
                // fprintf(stderr, "+++ MASTER: Sent link information[%d, %d] to slave: %d\n", row_id, col_id, slave_id);
                slave_id++;
            }
        }
    }
	// Send over the list of trains to the slaves
    // fprintf(stderr, "+++ MASTER: Sending all %d trains to the slaves.\n", num_trains);
    num_links = slave_id;
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
		if (trains[i].status == NOT_IN_NETWORK) {
            current_station = -1;
            next_station = -1;
        }
        else if (trains[i].line == GREEN){
			current_station = get_all_station_index(S, trains[i].station, G,  all_stations_list);
            next_station = get_all_station_index(S, get_next_station(trains[i].station, trains[i].direction, num_green_stations), G, all_stations_list);
		}
		else if (trains[i].line == BLUE) {
			current_station = get_all_station_index(S, trains[i].station, B,  all_stations_list);
			next_station = get_all_station_index(S, get_next_station(trains[i].station, trains[i].direction, num_blue_stations), B, all_stations_list);	
		}
		else {
			current_station = get_all_station_index(S, trains[i].station, Y,  all_stations_list);
			next_station = get_all_station_index(S, get_next_station(trains[i].station, trains[i].direction, num_yellow_stations), Y, all_stations_list);	
		}
        int train_status[7] = { current_station, next_station, trains[i].line, trains[i].loading_time, trains[i].transit_time, trains[i].status, i};
        for (slave_id = 0; slave_id < num_links; slave_id++) {
            MPI_Send(train_status, 7, MPI_INT, slave_id, i, MPI_COMM_WORLD);
        }
    }
}

/**
 * Receives the result array information from the slaves
 **/
void master_receive_result(int S, int station_status[], int **link_status, int **link_transit_time, struct train_type trains[], char *G[], char *Y[], char *B[], char *all_stations_list[], int **green_stations, int **yellow_stations, int **blue_stations) {
	MPI_Status status;
	// Master waits for an array describing the train link status and an array of the train that has been modified.
    // fprintf(stderr, "+++ MASTER : Now receiving results back from the slaves\n");
	int i, j;
	int slave_id = 0;
	int link_information_buffer[5];
	int train_information_buffer[7];

	// Wait for an array describing the only train that has been modified by the link. 
	// note(lowjiansheng): The tag_id will be the slave_id as each slave will only return 1 train that has been modified.
    for (slave_id = 0 ; slave_id < slaves; slave_id++) {
        MPI_Recv(&train_information_buffer, 7, MPI_INT, slave_id, TRAIN_INFORMATION_TAG, MPI_COMM_WORLD, &status);
		MPI_Recv(&link_information_buffer, 5, MPI_INT, slave_id, LINK_INFORMATION_TAG, MPI_COMM_WORLD, &status);
        //---------------------------- NO UPDATE FROM THIS SLAVE -------------------------------//
        if (train_information_buffer[0] < 0) {
            continue;
        }
        //---------------------------- UPDATE TRAINS -------------------------------//
        int train_index = train_information_buffer[0];
        int train_status = train_information_buffer[1];
        int train_transit_time = train_information_buffer[2];

        // Case 1: Train is on the link and still has transit time, just decrement transit time and move on to the next slave
        if (trains[train_index].status == IN_TRANSIT && train_transit_time > 0) {
            trains[train_index].transit_time = train_transit_time;
        }
        // Case 2: Train was on the link and reached the next station.
        else if (trains[train_index].status == IN_TRANSIT && train_transit_time == 0) {
            int num_stations;
            int next_station;
            int next_direction;
            int **line_stations;

            // Find the local index of the next station
            int prev_station = trains[train_index].station;
            if (trains[train_index].line == GREEN) {
                num_stations = num_green_stations;
                line_stations = green_stations;
            } else if (trains[train_index].line == BLUE) {
                num_stations = num_blue_stations;
                line_stations = blue_stations;
            } else {
                num_stations = num_yellow_stations;
                line_stations = yellow_stations;
            }

            next_station = get_next_station(prev_station, trains[train_index].direction, num_stations);
            // fprintf(stderr, "\nFrom local: %d ---> To Local: %d", prev_station, next_station);
            // Update the direction of the train (For trains reaching a terminal station)
            next_direction = trains[train_index].direction;
            // Change train direction for reaching terminals.
            if (next_station == num_stations - 1) {
                next_direction = LEFT;
            } else if (next_station == 0) {
                next_direction = RIGHT;
            }
            line_stations[next_direction][next_station] = train_index;
            trains[train_index].transit_time = 0;
            trains[train_index].station = next_station;
            trains[train_index].status = IN_STATION;
            trains[train_index].loading_time = WAITING_TO_LOAD;
            trains[train_index].direction = next_direction;
        } 
        // Case 3: Train just got onto the link
        else {
            int current_station = trains[train_index].station;
            int **line_stations;
			char **line_stations_names;
            if (trains[train_index].line == GREEN) {
                line_stations = green_stations;
				line_stations_names = G;
            } else if (trains[train_index].line == BLUE) {
                line_stations = blue_stations;
				line_stations_names = B;
            } else {
                line_stations = yellow_stations;
				line_stations_names = Y;
            }
            int current_all_station_index = get_all_station_index(S, current_station, line_stations_names, all_stations_list);
            int current_direction = trains[train_index].direction;
            trains[train_index].status = train_status;
            trains[train_index].transit_time = train_transit_time;
        }
        //---------------------------- UPDATE LINKS -------------------------------//
		link_status[link_information_buffer[MSG_LINK_ROW_ID]][link_information_buffer[MSG_LINK_COL_ID]] = link_information_buffer[MSG_LINK_STATUS];
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
    num_green_stations = 0;
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
    num_yellow_stations = 0;
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
    num_blue_stations = 0;
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
    num_trains = g + y + b;
    //---------------------------- PARSING INPUT FROM THE INPUT FILE. -------------------------------//
    fprintf(stderr, " ~~~~~~~~~~~~~~~~~~~~~~~~ Master done parsing input file. With num trains: %d\n", num_trains);
    //---------------------------- INITIALISATION OF STATUS TRACKING ARRAYS -------------------------------//
	
	// INITIALISATION of link statuses.
    // LINK STATUS:
    // STORES GLOBAL INDEX OF THE TRAIN IF THE TRAIN IS IN THE LINK.
    // ELSE LINK_IS_EMPTY.
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
    fprintf(stderr, " ~~~~~~~~~~~~~~~~~~~~~~~~ Master done Initializing network \n");
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
        // fprintf(stderr, " ~~~~~~~~~~~~~~~~~~~~~~~~ Time tick: %d | Master distributing parallel code\n", time_tick);
		master_distribute(S, links_status, trains, num_all_trains, link_transit_time, G, Y, B, all_stations_list);
		master_receive_result(S, station_status, links_status, link_transit_time, trains, G, Y, B, all_stations_list, green_stations, yellow_stations, blue_stations);
        // STEP 3: ---------------------------- MASTER (Load trains into empty stations) ----------------------------
        for (i = 0 ; i < S; i++) {
            int station_trains_buffer[num_all_trains];
            int buffer_index = 0;
            time_t t;
            srand((unsigned) time(&t));
            if (station_status[i] == LOADING) {
                // note(Marx): Should expect that station 7 is here in first few iterations. but it is not
                // fprintf(stderr, "\nIteration %d. Station %d is loading\n", time_tick, i);
            }
            if (station_status[i] == READY_TO_LOAD) {
                for (j = 0 ; j < num_all_trains; j++) {
                    if (trains[j].status == NOT_IN_NETWORK) {
                        continue;
                    }
                    int all_stations_index;
                    if (trains[j].line == GREEN) {
                        all_stations_index = get_all_station_index(S, trains[j].station, G, all_stations_list);
                    }   
                    else if (trains[j].line == BLUE) {
                        all_stations_index = get_all_station_index(S, trains[j].station, B, all_stations_list);
                    }
                    else {
                        all_stations_index = get_all_station_index(S, trains[j].station, Y, all_stations_list);
                    }
                    // fprintf(stderr, "\n[Step 3] Debug here: Train status: %d, train international station: %d. Train station: %d, trian line: %d", trains[j].status, all_stations_index, trains[j].station, trains[j].line);
                    // Put all trains that are waiting to load in the station to a buffer. Trains in this buffer will be randomly chosen
                    if (all_stations_index == i && trains[j].status == IN_STATION && trains[j].loading_time == WAITING_TO_LOAD){
                        station_trains_buffer[buffer_index] = j;
                        buffer_index ++;
                    }
                }
                // Randomly pick a train index to start loading in this station
                if (buffer_index > 0) {
                    int random_buffer_index = rand() % buffer_index;
                    int random_train_index = station_trains_buffer[random_buffer_index];
                    station_status[i] = LOADING;
                    trains[random_train_index].loading_time = calculate_loadtime(all_stations_popularity_list[i]);
                    if (trains[random_train_index].line == GREEN) {
                        green_stations[trains[random_train_index].direction][trains[random_train_index].station] = LOADING;
                    } else if (trains[random_train_index].line == BLUE) {
                        blue_stations[trains[random_train_index].direction][trains[random_train_index].station] = LOADING;
                    } else {
                        yellow_stations[trains[random_train_index].direction][trains[random_train_index].station] = LOADING;
                    }
                    // note(Marx): Debug statement to ensure that we are allowing a random train to load here.
                    // if (buffer_index > 1) {
                    //     fprintf(stderr, "\n\nAlert");
                    //     int i = 0;
                    //     for (i = 0; i < buffer_index; i ++) {
                    //         fprintf(stderr, " | Train %d", station_trains_buffer[i]);
                    //     }
                    //     fprintf(stderr, " | Are waiting to board the station [%d]\n", i);
                    //     fprintf(stderr, "We are randomly choosing, train index [%d] to board \n\n", random_train_index);
                    // }
                }
            }
        }
        // STEP 4: ---------------------------- MASTER (Count stations that are idle in this iteration) ----------------------------
        for (i = 0; i < 2; i++) {
            char *c;
            if (i == 0) {
                c = "left";
            } else {
                c = "right";
            }
            for (j = 0; j < num_green_stations; j++) {
                if (green_stations[i][j] == READY_TO_LOAD) {
                    int station_index = get_all_station_index(num_all_trains, j, G, all_stations_list);
                    // fprintf(stderr, "The train [%d, %d] is waiting. Global index: %d. And idle at this iteration %d\n", i, j, station_index, time_tick);
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

        // STEP 5: ---------------------------- MASTER (Decrement loading time of trains in stations) ----------------------------
        for (i = 0 ; i < num_all_trains; i++){
            if (trains[i].status == NOT_IN_NETWORK) {
                continue;
            }
            int **line_stations;
            int all_stations_index;
            int num_line_stations;
            if (trains[i].line == GREEN) {
                line_stations = green_stations;
                num_line_stations = num_green_stations;
                all_stations_index = get_all_station_index(S, trains[i].station, G, all_stations_list);
            }   
            else if (trains[i].line == BLUE) {
                line_stations = blue_stations;
                num_line_stations = num_blue_stations;
                all_stations_index = get_all_station_index(S, trains[i].station, B, all_stations_list);
            }
            else {
                line_stations = yellow_stations;
                num_line_stations = num_yellow_stations;
                all_stations_index = get_all_station_index(S, trains[i].station, Y, all_stations_list);
            }
            // note(Marx): This is an intense debugging session that I spent too much time on
            // if (i == 0) {
            //     struct train_type train = trains[i];
            //     int temp_loading_time = train.loading_time;
            //     int temp_transit_time = train.transit_time;
            //     int temp_local_station = train.station;
            //     int temp_direction = train.direction;
            //     int temp_next_station = get_next_station(temp_local_station, temp_direction, num_line_stations);
            //     // fprintf(stderr, "Next station index here: %d | Local station: %d | num stations: %d\n", temp_next_station, temp_local_station, g);
            //     int temp_international_station = get_all_station_index(num_all_trains, trains[i].station, G, all_stations_list);
            //     int temp_next_international_station = get_all_station_index(num_all_trains, temp_next_station, G, all_stations_list);
            //     fprintf(stderr, "\n\nIntense debug: Train[%d] | transit time: %d | Loading time: %d | local station: %d | next station: %d | direction: %d\n\n", 
            //         i,
            //         temp_transit_time,
            //         temp_loading_time,
            //         temp_local_station,
            //         temp_next_station,
            //         temp_direction
            //     );
            // }
            if (trains[i].loading_time > FINISHED_LOADING) {
                trains[i].loading_time--;
                if (trains[i].loading_time == FINISHED_LOADING){
                    station_status[all_stations_index] = READY_TO_LOAD;
                    line_stations[trains[i].direction][trains[i].station] = READY_TO_LOAD; // Set station back to ready to load for counting idle time
                }
            }
            // The update to move trains from transit into station --> ALREADY DONE AT SLAVE
        }
        print_output(time_tick, trains, num_all_trains, G, Y, B, g, y, b, all_stations_list, S, num_green_stations, num_yellow_stations, num_blue_stations, fp);
        // fprintf(stderr, "\n\n");
	}
    // Close clock for time
    clock_t difference = clock() - before;
    msec = difference * 1000 / CLOCKS_PER_SEC;
    printf("\nTime taken: %d seconds %d milliseconds\n", msec/1000, msec%1000);

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
    MPI_Abort(MPI_COMM_WORLD, 0);
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
