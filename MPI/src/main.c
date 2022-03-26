
#include <mpi.h>
#include <unistd.h>
#include<stdio.h>

// Number of tasks assigned at once
int const TASK_PACKET_SIZE = 1;

// Different message tags
int const TAG_WORK_REQUEST = 1, // Workers requesting work
          TAG_WORK_BRIEF   = 2, // Controller sending work
          TAG_SUM_RETURN   = 3; // Workers returning the sum of all completed calculations

// Signal to workers that there are no more tasks
int const SIGNAL_COMPLETE = -1;

int calculate(int input)
{
    usleep(0.1f * 1000000);
    return 1;
}

/**
 * Main execution for authoritative process
 * @param processes total number of assigned processes
 * @param min minimum value for calculation range
 * @param max maximum value for calculation range
 */
void controller(int processes, int min, int max)
{
    printf("[CONTROLLER] Starting\n");

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

    printf("[CONTROLLER] Sum: %d\n", sum);
}

/**
 * Main execution for worker processes
 * @param rank MPI rank of running process
 * @param min minimum value for calculation range
 * @param max maximum value for calculation range
 */
void worker(int rank, int min, int max)
{
    // Sum up each calculation to return at end
    int sum = 0;

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
        printf("Calculating for range %d-%d\n", taskMin, taskMax);
        for (int i=taskMin; i<=taskMax; i++)
            sum += calculate(i);
    }

    // Return calculation sum
    int reductionOutput; // Required for reduction
    MPI_Reduce(&sum, &reductionOutput, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
    // Initialise MPI
    MPI_Init(&argc, &argv);

    // Get process data
    int processes, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Run
    int min=0, max=10000;
    if (rank==0)
        controller(processes, min, max);
    else
        worker(rank, min, max);

    // Finalise MPI
    MPI_Finalize();

    return 0;
}
