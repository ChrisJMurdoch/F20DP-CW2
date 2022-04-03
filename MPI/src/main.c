
// Author: Christopher Murdoch
// Partially written for CW1 and partially for CW2

#include "../include/math.h"

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdbool.h>

// Number of tasks assigned at once
int const TASK_PACKET_SIZE = 10;

// Different message tags
int const TAG_WORK_REQUEST = 1, // Workers requesting work
          TAG_WORK_BRIEF   = 2, // Controller sending work
          TAG_SUM_RETURN   = 3; // Workers returning the sum of all completed calculations

// Signal to workers that there are no more tasks
int const SIGNAL_COMPLETE = -1;

/**
 * Main execution path for authoritative process
 * @param min minimum value for calculation range
 * @param max maximum value for calculation range
 */
long controller(long min, long max)
{
    // Distribute tasks until depleted
    for (int i=min; i<=max; i+=TASK_PACKET_SIZE)
    {
        // Get work request from any worker
        int workerRank;
        MPI_Status status;
        MPI_Recv(&workerRank, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &status);

        // Send work details back to worker
        MPI_Send(&i, 1, MPI_INT, workerRank, TAG_WORK_BRIEF, MPI_COMM_WORLD);
    }

    // Tell all workers to finish
    int processes;
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    for (int i=1; i<processes; i++)
    {
        // Get work request from any worker
        int workerRank;
        MPI_Status status;
        MPI_Recv(&workerRank, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &status);

        // Tell worker to finish
        MPI_Send(&SIGNAL_COMPLETE, 1, MPI_INT, workerRank, TAG_WORK_BRIEF, MPI_COMM_WORLD);
    }

    // Collect worker sums
    int const reductionInput = 0; // Required for reduction
    int sum;
    MPI_Reduce(&reductionInput, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    return sum;
}

/**
 * Main execution path for worker processes
 * @param min minimum value for calculation range
 * @param max maximum value for calculation range
 */
void worker(int min, int max)
{
    // Get MPI rank
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Sum up each calculation to reduce once finished
    int sum = 0;

    // Work until done
    while (1)
    {
        // Send task request
        MPI_Send(&rank, 1, MPI_INT, 0, TAG_WORK_REQUEST, MPI_COMM_WORLD);
        
        // Recieve task details
        int taskMin;
        MPI_Status status;
        MPI_Recv(&taskMin, 1, MPI_INT, 0, TAG_WORK_BRIEF, MPI_COMM_WORLD, &status);

        // Check done
        if (taskMin==SIGNAL_COMPLETE)
            break;

        // Perform work on task range
        int taskMax = max<(taskMin+TASK_PACKET_SIZE-1) ? max : (taskMin+TASK_PACKET_SIZE-1);
        for (int i=taskMin; i<=taskMax; i++)
            sum += eulerTotient(i);
    }

    // Return calculation sum
    int reductionOutput; // Required for reduction
    MPI_Reduce(&sum, &reductionOutput, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
}

/**
 * Main execution path when only one process has been allocated
 * Controller-Worker method only works with a minimum of 2 processes
 * @param min minimum value for calculation range
 * @param max maximum value for calculation range
 */
long sequential(long min, long max)
{
    // Sum totient for each value in given range
    int sum = 0;
    for (int i=min; i<=max; i++)
        sum += eulerTotient(i);
    return sum;
}

/**
 * Measure the duration of calling the given function with given arguments
 * @param func Function to be called
 * @param argA First argument
 * @param argB Second argument
 * @param verbose Print output to console
 * @return Duration of call
 */
double duration(long (*func)(long, long), long argA, long argB, bool verbose)
{
    // Record timestamps immediately before and after function execution
    struct timeval start, end;
    gettimeofday(&start, NULL);
    long result = (*func)(argA, argB);
    gettimeofday(&end, NULL);

    // Calculate duration
    double duration = (end.tv_sec-start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0);
    if (verbose)
        printf("∑Φ[%lu-%lu]=%lu took %f seconds.\n", argA, argB, result, duration);
    
    return duration;
}

/**
 * Calculate and profile Euler Totient Sum for given range
 * @param argv[0] Lower value for range to be calculated - defaults to 1
 * @param argv[1] Upper value for range to be calculated - defaults to 15,000
 */
int main(int argc, char **argv)
{
    // Initialise MPI
    MPI_Init(&argc, &argv);

    // Initialise math library
    initPrimeCache();

    // Parse inputs
    int const min = argc>=2 ? atoi(argv[1]) : 1,
              max = argc>=3 ? atoi(argv[2]) : 15000;

    // Get MPI data for this process
    int rank, processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);

    // Run depending on process configuration
    if (processes==1)
        // Sequential operation
        duration(sequential, min, max, true);

    else if (rank==0)
        // Parallel operation - Authoritative process
        duration(controller, min, max, true);

    else
        // Parallel operation - Worker process
        worker(min, max);

    // Finalise MPI
    MPI_Finalize();

    return 0;
}
