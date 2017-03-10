/**
* A simple program that demonstrates MPI sending and receiving to different processors. The program
* initializes a "potato" with a random number and sends it back and forth to the different processors,
* who decrement it as the receive it. Once the potato variable gets to 0, that processor reports it is
* "it" and ends the game.
* 
* @since October 1, 2016
* @author Alec J. Horne
*/

#include "mpi.h"
#include <iostream>
#include <time.h>
#include <cstdlib>

int main(int argc, char** argv) {
	// local variables
	int rank, size, potato, dest, arrsize;
	MPI_Status status;

	// initialize the MPI environment, get rank and size
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// initialize potato with random number between process size and 25
	srand(time(0));
	potato = rand() % (25 - size) + size;
	int length[potato + 2];
	length[0] = potato;
	
	// if master process 0
	if (rank == 0)
	{
		// get a random destination
		dest = rand() % (size - 1) + 1;
		
		// send potato to random destination
		MPI_Send(length, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);

		// wait for potato to come back
		MPI_Recv(length, potato + 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
		// get the message size and print out the data
		MPI_Get_count(&status, MPI_INT, &arrsize);	
		for(int x = 1; x < arrsize; x++) {
			if(x == arrsize - 1)
				std::cout << "Node " << length[x] << " is it, game over" << std::endl;
			else
				std::cout << "Node " << length[x] << " has the potato, passing to node " << length[x+1] << std::endl;
		}
	}

	// else other process besides 0
	else {
		// seed random generator with rank so everyone has different numbers
		srand(rank);
	
		bool over = false;
		while (!over) {

			// receive the potato
			MPI_Recv(length, potato + 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			// if potato isnt 0 keep passing it
			if (length[0] > 0)
			{
				do {	
					dest = rand() % (size - 1) + 1;
				} while (dest == rank);
				MPI_Get_count(&status, MPI_INT, &arrsize);
				length[0] -= 1;
				length[arrsize] = rank;
				MPI_Send(length, arrsize + 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
			}

			// if potato is 0 have process send termination message
			else if (length[0] == 0)
			{
				// send the termination message
				length[0] = -1;
				length[potato + 1] = rank;
				for (dest = 0; dest < size; dest++)
					if (dest != rank)
						MPI_Send(length, potato + 2, MPI_INT, dest, 1, MPI_COMM_WORLD);
				over = true;
			}
			
			// if potato is -1 end the game for every process
			else
			{
				over = true;
			}
		}
	}

	// finalize the MPI environment
	MPI_Finalize();
}