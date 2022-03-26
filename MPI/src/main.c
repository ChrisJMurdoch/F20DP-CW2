
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>

// Number of tasks assigned to a worker each time it's available
int const TASK_PACKET_SIZE = 3;

// Different message tags
int const TAG_WORK_REQUEST = 1,
          TAG_WORK_SEND    = 2;

// Signal to finish working
int const SIGNAL_TERMINATE = -1;

int calculate(int input)
{
    sleep(1);
    return input;
}

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
        MPI_Send(&i, 1, MPI_INT, workerRank, TAG_WORK_SEND, MPI_COMM_WORLD);
    }

    // Tell all workers to finish
    for (int i=1; i<processes; i++)
    {
        // Get work request from any worker
        int workerRank;
        MPI_Status status;
        MPI_Recv(&workerRank, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &status);

        // Tell worker to finish
        MPI_Send(&SIGNAL_TERMINATE, 1, MPI_INT, workerRank, TAG_WORK_SEND, MPI_COMM_WORLD);
    }

    printf("[CONTROLLER] Finished\n");
}

void worker(int rank, int min, int max)
{
    while (1)
    {
        // Send task request
        MPI_Send(&rank, 1, MPI_INT, 0, TAG_WORK_REQUEST, MPI_COMM_WORLD);
        
        // Recieve task details
        int taskMin;
        MPI_Status status;
        MPI_Recv(&taskMin, 1, MPI_INT, 0, TAG_WORK_SEND, MPI_COMM_WORLD, &status);

        // Check done
        if (taskMin==SIGNAL_TERMINATE)
            break;

        // Perform work on task range
        int taskMax = max<(taskMin+TASK_PACKET_SIZE-1) ? max : (taskMin+TASK_PACKET_SIZE-1);
        printf("Calculating for range %d-%d\n", taskMin, taskMax);
        for (int i=taskMin; i<=taskMax; i++)
            calculate(i);
    }
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
    int min=0, max=10;
    if (rank==0)
        controller(processes, min, max);
    else
        worker(rank, min, max);

    // Finalise MPI
    MPI_Finalize();

    return 0;
}
