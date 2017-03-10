// FloydOMP.cpp : Defines the entry point for the console application.
//

#include <cstdlib>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#define INT_MAX 2147483647

// struct for user input
struct path
{
	int start;
	int destination;
};

// global variables
int** dist;
std::string* edgenames;
int num_nodes;
int **paths;
int threads;

// function prototypes
path getUserInput();
double floydWarshall();
void getPathRecursive(int, int);

int main(int argc, char** argv)
{
	// local variables
	std::string filename;
	std::cout << "Enter number of threads: ";
	std::cin >> threads;
	std::cout << "Enter filename: ";
	std::cin >> filename;

	// open the file with the graph data
	std::ifstream infile;
	infile.open(filename.c_str());

	// make sure the file was open successfully before reading data
	if (infile.is_open()) {

		// get the number of nodes from the first line of the file
		std::string line;
		getline(infile, line);
		num_nodes = stoi(line);

		// read edge names into the edge name array
		edgenames = new std::string[num_nodes];
		for (int x = 0; x < num_nodes; x++)
			getline(infile, edgenames[x]);

		// declare a dynamic sized 2D array based on number of nodes and
		dist = new int*[num_nodes];
		for (int x = 0; x < num_nodes; x++)
		{
			dist[x] = new int[num_nodes];
			// fill adjacency matrix with INT_MAX to start
			for (int y = 0; y < num_nodes; y++)
			{
				// each nodes distance to itself is 0
				if (x == y)
					dist[x][y] = 0;
				// node to other node dist all start at "infinity"
				else
					dist[x][y] = INT_MAX;
			}
		}

		// read the data from the file into the adjacency matrix
		int s, e, d;
		while (!infile.eof())
		{
			getline(infile, line);
			if (line != "")
			{
				// parse integers from each line read
				std::stringstream ss(line);
				ss >> s >> e >> d;

				// -1 as start node marks the end of the file
				if (s != -1)
				{
					dist[s][e] = d;
					dist[e][s] = d;
				}
			}
		}
	}
	else
	{
		edgenames = nullptr;
		dist = nullptr;
		num_nodes = 0;
	}

	infile.close();
	double time = floydWarshall();
	std::cout << "Total time on " << threads << " threads: " << time << std::endl;
	char cont = 'y';
	do
	{
		path p = getUserInput();
		std::cout << "** " << edgenames[p.start] << " to " << edgenames[p.destination] << " **" << std::endl;
		std::cout << "Path:" << std::endl;
		getPathRecursive(p.start, p.destination);
		std::cout << std::endl;
		std::cout << "Distance: " << dist[p.start][p.destination] << std::endl;
		std::cout << std::endl;
		std::cout << "Another path (y/n)? ";
		std::cin >> cont;
	} while (cont == 'y');
	delete dist, paths, edgenames;
	return 1;
}

path getUserInput()
{
	path ret;
	std::cout << "NQMQ Menu" << std::endl;
	std::cout << "-----------------------------" << std::endl;
	for (int x = 0; x < num_nodes; x++)
		std::cout << x + 1 << ". " << edgenames[x] << std::endl;
	std::cout << std::endl;
	std::cout << "Path from? ";
	std::cin >> ret.start;
	std::cout << "To? ";
	std::cin >> ret.destination;
	ret.start -= 1;
	ret.destination -= 1;
	std::cout << std::endl;
	return ret;
}

double floydWarshall() {
	int i, j, k;
	// initialize paths array with all -1s 
	paths = new int*[num_nodes];
	for (int x = 0; x < num_nodes; x++)
	{
		paths[x] = new int[num_nodes];
		for (int y = 0; y < num_nodes; y++)
			paths[x][y] = -1;
	}
	omp_set_num_threads(threads);
	double startTime, finishTime;
	#pragma omp parallel
	startTime = omp_get_wtime();
	for (k = 0; k < num_nodes; ++k) {
	#pragma omp for private(i, j)
		for (i = 0; i < num_nodes; ++i) {
			for (j = 0; j < num_nodes; ++j)
			{
				// ignore cities that don't have a path or are to themselves
				if (dist[i][k] == INT_MAX || dist[k][j] == INT_MAX || i == j)
					continue;

				// check if there is a faster path 
				int new_dist = dist[i][k] + dist[k][j];
				if (dist[i][j] <= new_dist)
					continue;

				// this way is faster, update the array and store the parent for the path
				dist[i][j] = new_dist;
				paths[i][j] = k;
			}
		}
	}
	finishTime = omp_get_wtime();
	return finishTime - startTime;
}

void getPathRecursive(int a, int b)
{
	int i = paths[a][b];
	if (i == -1)
		std::cout << edgenames[a] << " to " << edgenames[b] << std::endl;
	else
	{
		getPathRecursive(a, i);
		getPathRecursive(i, b);
	}
}