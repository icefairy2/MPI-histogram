#include "pch.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (argc != 2) {
        if (world_rank == 0) {
            printf("Usage: %s infilename\n", argv[0]);
        }
        MPI_Finalize();
        exit(1);
    }

    char* filename = argv[1];
    if (world_size <= 1) {
        printf("You need some slaves!\n");
        MPI_Finalize();
        exit(1);
    }

    int nr_slaves = world_size - 1;

    if (world_rank == 0) {
        std::ifstream infile;
        infile.open(filename);

        int* arr;
        int n = 0;
		//first line has to contain number of elements
		infile >> n;
		arr = (int*)malloc((n + 1) * sizeof(int));
		//n lines of gray values
		for (int i = 0; i < n; i++) {
			infile >> arr[i];
		}

        infile.close();

        int hist[256];
        for (int i = 0; i < 256; i++) {
            hist[i] = 0;
        }

        int chunk_size = n / nr_slaves;

        for (int i = 1; i < nr_slaves; i++) {
            MPI_Send(&chunk_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&arr[chunk_size*(i - 1)], chunk_size, MPI_INT, i, 0, MPI_COMM_WORLD);
            //printf("Process 0 sent %d numbers from array starting from index %d to process %d\n", chunk_size, i - 1, i);
        }

        //send last separately in case array wasn't evenly distributed
        int rem = n - (chunk_size*nr_slaves) + chunk_size;

        MPI_Send(&rem, 1, MPI_INT, nr_slaves, 0, MPI_COMM_WORLD);
        MPI_Send(&arr[chunk_size*(nr_slaves - 1)], rem, MPI_INT, nr_slaves, 0, MPI_COMM_WORLD);

        for (int i = 1; i <= nr_slaves; i++) {
            int hist_chunk[256];
            MPI_Recv(&hist_chunk, 256, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Process 0 received histogram chunk from process %d\n", i);
            for (int i = 0; i < 256; i++) {
                hist[i] += hist_chunk[i];
            }
        }
        for (int i = 0; i < 256; i++) {
            printf("Value %d count: %d\n", i, hist[i]);
        }
    }
    else {
        int number;
        int hist[256];
        for (int i = 0; i < 256; i++) {
            hist[i] = 0;
        }
        int* arr_piece;
        MPI_Recv(&number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //printf("Process %d received number %d from process 0\n", world_rank, number);

        arr_piece = (int*)malloc((number + 1) * sizeof(int));
        MPI_Recv(arr_piece, number, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //printf("Process %d received chunk_size %d from process 0\n", world_rank, number);
        for (int i = 0; i < number; i++) {
            hist[arr_piece[i]]++;
        }
        MPI_Send(&hist, 256, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}