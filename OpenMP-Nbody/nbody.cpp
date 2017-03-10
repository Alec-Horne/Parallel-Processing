#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <omp.h>
#include <sstream>

// definition of node struct
struct node
{
	// position variables
	double px, py;
	// velocity variables
	double vx, vy;
	// acceleration variables
	double ax, ay;
};

// global constants
const double g = 1;
const double m = 1;
const double timestep = 0.01;
const int p = 1000;

// global variables
int n = 1000;
node nodes[1000];

// function prototypes
void readData();
void compute();

int main(int argc, char ** argv) {

	// read data and initialize each node in nodes
	readData();

	double startTime = omp_get_wtime();

	// compute node movement for 100 timesteps
	for (int x = 0; x < 100; x++)
		compute();

	// get wall computation time
	double endTime = omp_get_wtime();
	double time = endTime - startTime;

	// output the data for the last 20 nodes and the time taken
	for (int x = 980; x < n; x++)
		std::cout << "Node "<< x+1 << " position: (" << nodes[x].px << ", " << nodes[x].py << ")" << std::endl;
	std::cout << "Operation took " << time << " seconds on " << p << " processors" << std::endl;

	std::cin.get();
}

/* read positions from file and initialize each nodes member variables with 0's */
void readData() {
	std::ifstream inFile;
	inFile.open("nbodies.dat");
	std::string str;
	double x, y;
	for (int i = 0; i < n; ++i)
	{
		getline(inFile, str);
		if (str != "")
		{
			std::stringstream ss(str);
			ss >> x >> y;
			nodes[i].px = x;
			nodes[i].py = y;
		}
		nodes[i].vx = 0;
		nodes[i].vy = 0;
		nodes[i].ax = 0;
		nodes[i].ay = 0;
	}
	inFile.close();
}

/* calculate acceleration/force, velocity, and position for each node during a single timestep */
void compute() {
	int i, j;
	double dist;

	// set number of threads
	omp_set_num_threads(n);
	// declare parallel region and set loop j to run in parallel
	#pragma omp parallel private(j) 
	{
		// compute force/acceleration
		#pragma omp for schedule(static)
		for (i = 0; i < n; ++i) {
			// reset acceleration values
			nodes[i].ax = 0.0;
			nodes[i].ay = 0.0;
			for (j = 0; j < n; ++j) {
				if (i != j)
				{
					dist = pow((nodes[i].px - nodes[j].px), 2) + pow((nodes[i].py - nodes[j].py), 2);
					// dont compute interactions between nodes too close
					if (dist >= 0.1)
					{
						nodes[i].ax -= g * m * (nodes[i].px - nodes[j].px) / pow(dist, 1.5);
						nodes[i].ay -= g * m * (nodes[i].py - nodes[j].py) / pow(dist, 1.5);
					}
				}
			}
			// compute velocity
			nodes[i].vx += timestep * nodes[i].ax;
			nodes[i].vy += timestep * nodes[i].ay;
		}

		// compute position
		#pragma omp for schedule(static)
		for (int i = 0; i < n; ++i) {
			nodes[i].px += nodes[i].vx * timestep;
			nodes[i].py += nodes[i].vy * timestep;
		}
	}
	// synchronize threads after each time step
	#pragma omp barrier
}