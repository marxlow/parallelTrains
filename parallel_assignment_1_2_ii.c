/**
 * CS3210 - Matrix Multiplication in MPI
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <mpi.h>

int slaves;
int myid;
#define MASTER_ID slaves

/*************************************************************************************************************************************/

/**
 * Function used by the slaves to receive data from the master
 * Each slave receives a link_status_buffer which is an array containing value of the link
 * and a num_slaves x 6 matrix containing information of the train in the network
 **/
void slave_receive_data() {

}

/** 
 * Function used by the slaves to compute the update to the network.
 **/
void slave_compute() {
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
	// Receive data
	slave_receive_data();

	// Doing the computations
	slave_compute();

	// Sending the results back
	slave_send_result();
}


/*************************************************************************************************************************************/

/**
 * Function called by the master to distribute link_status
 * and the entire trains array to the child
 **/
void master_distribute()
{
    int i, j, k;
}

/**
 * Receives the result array information from the slaves
 **/
void master_receive_result() {
}

/**
 * Main function called by the master process
 *
 **/
void master() {
	// Distribute data to slaves
	master_distribute();

	// Gather results from slaves
	master_receive_result();
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
