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
	int rank, size, potato, dest;
	MPI_Status status;

	// initialize the MPI environment, get rank and size
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// if master process 0
	if (rank == 0)
	{
		// initialize potato with random number between process size and 25
		srand(time(0));
		potato = rand() % (25 - size) + size;

		// get a random destination
		dest = rand() % (size - 1) + 1;

		std::cout << "Node 0 has the potato, passing it to node " << dest << std::endl;
		potato -= 1;

		// send potato to random destination
		MPI_Send(&potato, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);

		// wait for potato to come back
		MPI_Recv(&potato, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}

	// else other process besides 0
	else {
		// seed random generator with rank so everyone has different numbers
		srand(rank);
	
		bool over = false;
		while (!over) {

			// receive the potato
			MPI_Recv(&potato, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			if (potato > 0)
			{
				do {	
					dest = rand() % (size - 1) + 1;
				} while (dest == rank);
				potato -= 1;
				std::cout << "Node " << rank << " has the potato, passing it to node " << dest << std::endl;
				MPI_Send(&potato, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
			}

			else if (potato == 0)
			{
				std::cout << "Node " << rank << " is it, game over" << std::endl;
				// send the termination message
				potato = -1;
				for (dest = 0; dest < size; dest++)
					if (dest != rank)
						MPI_Send(&potato, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
					over = true;
			}

			else
			{
				over = true;
			}
		}
	}

	// finalize the MPI environment.
	MPI_Finalize();
}