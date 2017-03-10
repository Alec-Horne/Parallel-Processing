/**
* An C++ MPI program that reads the XFL team names and scores and computes the power ratings in parallel
* using jacobi iteration. Each process of 8 receives one teamname and their corresponding scores, constructs
* the power equation with an initial guess of 100, and sends the new power rating to eachother using 
* MPI_Allgather(). This continues in a loop until the relative error for each process is less than 0.05 or 
* correct up to one decimal place.
*
* @since October 13, 2016
* @author Alec J. Horne
*/

#include <cstdlib>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cmath>
#include <sstream>

// function prototype
int isDone(double, double, int, double[]);

int main(int argc, char** argv)
{
	// local variables
	int rank, size, score, team, sum, iterations;
	sum = iterations = 0;
	int done = 1;
	double tolerance = 0.05;
	int coeff[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	MPI_Status status;

	// global arrays
	std::string teamnames[8];
	double power[8] = { 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0 };

	// initialize the MPI environment, get rank and size
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// make sure that process size is 8 for algorithm to work properly
	if (size != 8)
	{
		if (rank == 0)
			std::cout << "ERROR: Process size must be 8 to run this program" << std::endl;
		MPI_Finalize();
		return 1;
	}

	// if rank is 0, read the files and distribute the data
	if (rank == 0)
	{
		// open the file with the team names
		std::ifstream infile;
		std::string teamname;
		infile.open("xfltms.dat");

		// send each teamname to the correct process
		int teamnamessize = 0;
		while (!infile.eof())
		{
			getline(infile, teamname);
			if (teamname != "")
				teamnames[teamnamessize++] = teamname;
		}
		infile.close();

		// open the file with the teams score data
		std::string str;
		int away, home, awayscore, homescore;
		infile.open("xfl.dat");

		// send team data to each process
		int homedif, awaydif;
		while (!infile.eof())
		{
			getline(infile, str);
			if (str != "")
			{
				// parse the integers from each line in the file
				std::stringstream ss(str);
				ss >> home >> away >> homescore >> awayscore;
				homedif = homescore - awayscore;
				awaydif = awayscore - homescore;
				// send the data to both the home and away teams
				if (home != 1) {
					MPI_Send(&away, 1, MPI_INT, home - 1, 1, MPI_COMM_WORLD);
					MPI_Send(&homedif, 1, MPI_INT, home - 1, 1, MPI_COMM_WORLD);
				}
				else
				{
					sum += homedif;
					coeff[away - 1]++;
				}
				if (away != 1) {
					MPI_Send(&home, 1, MPI_INT, away - 1, 1, MPI_COMM_WORLD);
					MPI_Send(&awaydif, 1, MPI_INT, away - 1, 1, MPI_COMM_WORLD);
				} 
				else
				{
					sum += awaydif;
					coeff[home - 1]++;
				}
			}
		}
		infile.close();
	}

	// if process other than 0
	else
	{
		// receive team scores and opponents
		for (int x = 0; x < 10; x++)
		{
			MPI_Recv(&team, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Recv(&score, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			sum += score;
			coeff[team - 1]++;
		}
	}

	// for each iteration compute the new power rating and send it to the other processes
	while (true) 
	{
		double newpower = 0.0;
		double oldpower = power[rank];
		for (int x = 0; x < 8; x++)
			newpower += coeff[x] * power[x];
		newpower += sum;
		newpower /= 10;
		// updates the power array with newly computed values from each process
		MPI_Allgather(&newpower, 1, MPI_DOUBLE, &power, 1, MPI_DOUBLE, MPI_COMM_WORLD);
		iterations++;
		// if rank 0 check if values are within the specified tolerance
		if (rank == 0) 
		{
			done = isDone(oldpower, tolerance, size, power);
			for (int i = 1; i < size; i++)
				MPI_Send(&done, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
		}
		// if other rank send the old power value to process 0 and receive the "done" flag
		else
		{
			MPI_Send(&oldpower, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
			MPI_Recv(&done, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
		}
		// if all values are within the tolerance then end the loop
		if (done == -1)
			break;
	}

	// output data to the console
	if (rank == 0) 
	{
		for (int i = 0; i < size; i++)
			std::cout << teamnames[i] << " power is " << power[i] << std::endl;
		std::cout << std::endl;
		std::cout << "Jacobi's method took " << iterations << " iterations to complete with an error tolerance of "
			<< tolerance << std::endl;
	}
	// finalize the MPI environment
	MPI_Finalize();
}

/* Function to check if the values are within the specified tolerance. If they are, signal to 
 * each processor to end. If they aren't, continue with another iteration. -1 signals to end.
 */
int isDone (double op, double tol, int size, double ps[])
{
	// receive each processes previous power ranks for tolerance check
	MPI_Status status;
	double *oldpowers = new double[size];
	oldpowers[0] = op;
	for (int i = 1; i < size; i++)
		MPI_Recv(&oldpowers[i], 1, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	// if any value is not in tolerance return false
	for (int x = 0; x < size; x++)
		if (abs(oldpowers[x] - ps[x]) >= tol)
			return 1;

	// if all values in tolerance free memory allocated and return true
	delete[] oldpowers;
	return -1;
}


