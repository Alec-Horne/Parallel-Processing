/**
* An C++ MPI program that reads the NFL team names and scores and computes the power ratings in parallel
* using jacobi iteration. Each process of 8 receives four teamnames and their corresponding scores, constructs
* the power equation with an initial guess of 100, and sends the new power rating to eachother using basic send
* and receives. This continues in a loop until the relative error for each process is less than 0.05 or correct 
* up to one decimal place.
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

// function prototypes
int isDone(double[], double, int, double[]);
void numGamesPlayed(int[][32], int[]);

int main(int argc, char** argv)
{
	// local process variables
	int rank, size, done, iterations;
	iterations = done = 0;
	double tolerance = 0.05;
	int mycoeffs[4][32], mysums[4];
	double oldpowers[4], newpowers[4] = { 100.0, 100.0, 100.0, 100.0 };
	int numgames[4] = { 0, 0, 0, 0 };
	MPI_Status status;

	// initialize the MPI environment, get rank and size
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// global arrays
	std::string teamnames[32];
	double power[32] = { 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0,
		100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0,
		100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0 };
	int sums[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0 };
	int coeff[32][32];
	for (int x = 0; x < 32; x++)
		for (int y = 0; y < 32; y++)
			coeff[x][y] = 0;

	// make sure that process size is 8 for algorithm to work properly
	if (size % 8 != 0)
	{
		if (rank == 0)
			std::cout << "ERROR: Process size must be 8 to run this program" << std::endl;
		MPI_Finalize();
		return -1;
	}

	// if rank is 0, read the files and distribute the data
	if (rank == 0)
	{
		// open the file with the team names
		std::ifstream infile;
		std::string teamname;
		infile.open("nfltms.dat");

		// read file with teamnames into a string array
		int teamnamessize = 0;
		while (!infile.eof())
		{
			getline(infile, teamname);
			if (teamname != "")
			{
				teamnames[teamnamessize++] = teamname;
			}
		}
		infile.close();

		// open the file with the teams score data
		std::string str;
		int away, home, awayscore, homescore, awaydif, homedif;
		infile.open("nfl.dat");
		while (!infile.eof())
		{
			getline(infile, str);
			if (str != "")
			{
				// parse integers from each line and add the data to the global arrays
				std::stringstream ss(str);
				ss >> home >> away >> homescore >> awayscore;
				homedif = homescore - awayscore;
				awaydif = awayscore - homescore;
				sums[(home - 1)] += homedif;
				coeff[(home - 1)][away - 1]++;
				sums[(away - 1)] += awaydif;
				coeff[(away - 1)][home - 1]++;
			}
		}
		infile.close();

		// distribute equal data amounts to each process - each process gets 4 teams
		// offset is equal to processor size and splits up data in an equal organized way
		for (int i = 0; i < size; i++)
		{
			// process 0 can't send message to itself so stores its data as special case
			if (i == 0)
			{
				int offset = 0;
				for (int x = 0; x < 4; x++)
				{
					mysums[x] = sums[offset];
					for (int y = 0; y < 32; y++)
						mycoeffs[x][y] = coeff[offset][y];
					offset += 8;
				}
			}
			// send data to each process from global arrays
			else
			{
				int offset = 0;
				for (int x = 0; x < 4; x++)
				{
					MPI_Send(coeff[i + offset], 32, MPI_INT, i, 1, MPI_COMM_WORLD);
					MPI_Send(&sums[i + offset], 1, MPI_INT, i, 1, MPI_COMM_WORLD);
					offset += 8;
				}
			}
		}
	}

	// if process besides 0, receive the 4 teams assigned to corresponding process data
	else
	{
		for (int i = 0; i < 4; i++)
		{
			MPI_Recv(mycoeffs[i], 32, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Recv(&mysums[i], 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		}
	}

	// get the correct number of games played for each team for use in power equation
	numGamesPlayed(mycoeffs, numgames);

	// every iteration compute new power rating for each processes 4 teams and send to master
	while (true) {
		int offset = 0;
		for (int z = 0; z < 4; z++)
		{
			oldpowers[z] = newpowers[z];
			newpowers[z] = 0.0;
			for (int x = 0; x < 32; x++)
				newpowers[z] += mycoeffs[z][x] * power[x];
			newpowers[z] += mysums[z];
			newpowers[z] /= numgames[z];
			offset += 8;
		}
		// if rank is 0 update the power rankings array with newly computed values
		if (rank == 0)
		{
			offset = 0;
			for (int x = 0; x < 4; x++)
			{
				power[offset] = newpowers[x];
				offset += 8;
			}
			// receive the new values from each process and update the power rankings array
			for (int i = 1; i < size; i++)
			{
				offset = 0;
				double temp[4];
				MPI_Recv(temp, 4, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status);
				for (int x = 0; x < 4; x++)
				{
					power[i + offset] = temp[x];
					offset += 8;
				}
			}
			// check if the values are in specified tolerance/relative error
			done = isDone(oldpowers, tolerance, size, power);
			iterations += 1;
			for (int i = 1; i < size; i++) {
				MPI_Send(&done, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
				MPI_Send(power, 32, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
			}
		}
		
		// if not process 0 then send info to process 0 and receive updated power rankings array
		else
		{
			MPI_Send(newpowers, 4, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
			MPI_Send(oldpowers, 4, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
			MPI_Recv(&done, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
			MPI_Recv(power, 32, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);
		}
		// if all values are in tolerance then break the loop and stop calculations
		if (done == -1)
			break;
	}

	// output data to the console
	if (rank == 0) {
		for (int x = 0; x < 32; x++)
			std::cout << teamnames[x] << " power is " << power[x] << std::endl;
		std::cout << std::endl;
		std::cout << "Jacobi's method took " << iterations << " iterations to complete with an error tolerance of "
			<< tolerance << std::endl;
	}

	// finalize the MPI environment and return
	MPI_Finalize();
	return 0;
}

/* Function to check if the values are within the specified tolerance. If they are, signal to
 * each processor to end. If they aren't, continue with another iteration. -1 signals to end.
 */
int isDone(double op[], double tol, int size, double ps[])
{
	MPI_Status status;
	double *oldpowers = new double[32];
	double temp[4];
	int offset = 0;
	// gather process 0s previous power rankings
	for (int y = 0; y < 4; y++)
	{
		oldpowers[offset] = op[y];
		offset += 8;
	}

	// receive every other processes previous power rankings and add them to array
	for (int i = 1; i < size; i++)
	{
		offset = 0;
		MPI_Recv(temp, 4, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		for (int x = 0; x < 4; x++)
		{
			oldpowers[i + offset] = temp[x];
			offset += 8;
		}
	}
	
	// check if any teams power ranking is out of tolerance, if it is return false
	for (int x = 0; x < 32; x++)
		if (abs(oldpowers[x] - ps[x]) >= tol)
			return 1;

	// otherwise delete memory allocation and return true
	delete[] oldpowers;
	return -1;
}

/* Function that each process calls to get the correct number of games each team has played for
 * calculations. In the NFL each teams games played amounts are different.
 */
void numGamesPlayed(int arr[][32], int gp[])
{
	//count number of games played for each of the processes 4 teams
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 32; y++)
			if (arr[x][y] == 1)
				gp[x]++;
}