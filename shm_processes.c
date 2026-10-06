#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

void ClientProcess(int []);

int main(int argc, char *argv[])
{
     int    ShmID;
     int    *ShmPTR;
     pid_t  pid;
     int    status;

     // Request a shared memory of two integers
     ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          exit(1);
     }
     printf("Process has received a shared memory of two integers...\n");

     ShmPTR = (int *) shmat(ShmID, NULL, 0);
     if (*ShmPTR == -1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
     }
     printf("Process has attached the shared memory...\n");

     // Initialize BankAccount (index 0) and Turn (index 1) to 0
     ShmPTR[0] = 0; 
     ShmPTR[1] = 0; 
     printf("Orig Bank Account = %d\n", ShmPTR[0]);

     printf("Server is about to fork a child process...\n");
     pid = fork();
     if (pid < 0) {
          printf("*** fork error (child) ***\n");
          exit(1);
     }
     else if (pid == 0) {
          // Child process
          ClientProcess(ShmPTR);
          exit(0);
     }

     // Seed the random number generator for the parent process
     srand(getpid());

     // Parent (Dear Old Dad) logic
     for (int i = 0; i < 25; i++) {
          sleep(rand() % 6); // Sleep randomly between 0 - 5 seconds

          // Tanenbaum's strict alternation: wait for our turn
          while (ShmPTR[1] != 0) {
               // no-op
          }

          // Enter Critical Section
          int account = ShmPTR[0];
          
          if (account <= 100) {
               int balance = rand() % 101; // Random amount between 0 - 100
               if (balance % 2 == 0) {
                    // Even number: Deposit money
                    account += balance;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, account);
               } else {
                    // Odd number: No money to give
                    printf("Dear old Dad: Doesn't have any money to give\n");
               }
          } else {
               // Account > 100
               printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
          }

          // Update shared memory
          ShmPTR[0] = account;
          ShmPTR[1] = 1; // Pass turn to Child
     }

     // Parent waits for the child to complete
     wait(&status);
     
     // Teardown the shared memory structure
     printf("Process has detected the completion of its child...\n");
     shmdt((void *) ShmPTR);
     printf("Process has detached its shared memory...\n");
     shmctl(ShmID, IPC_RMID, NULL);
     printf("Process has removed its shared memory...\n");
     printf("Process exits...\n");
     exit(0);
}

void ClientProcess(int SharedMem[])
{
     // Seed the random number generator for the child process independently
     srand(getpid());

     // Child (Poor Student) logic
     for (int i = 0; i < 25; i++) {
          sleep(rand() % 6); // Sleep randomly between 0 - 5 seconds

          // Tanenbaum's strict alternation: wait for our turn
          while (SharedMem[1] != 1) {
               // no-op
          }

          // Enter Critical Section
          int account = SharedMem[0];
          int balance = rand() % 51; // Random amount needed between 0 - 50

          printf("Poor Student needs $%d\n", balance);

          if (balance <= account) {
               // Enough money to withdraw
               account -= balance;
               printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
          } else {
               // Not enough money
               printf("Poor Student: Not Enough Cash ($%d)\n", account);
          }

          // Update shared memory
          SharedMem[0] = account;
          SharedMem[1] = 0; // Pass turn back to Parent
     }
}