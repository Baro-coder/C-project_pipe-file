/*
    -------------------------------------
    |       Process sync - C Linux      |
    |         Pipe, File, Signal        |
    -------------------------------------
*/

/*
    -------------
   -- LIBRARIES --
    -------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <semaphore.h>

/*
    ----------------
   -- DECLARATIONS --
    ----------------
*/
#define READ 0          // pipe channel to read
#define WRITE 1         // pipe channel to write
#define BUFF_SIZE 128   // default buffer size

void P1_run();  // Main function of process 1
void P2_run();  // Main function of process 2
void P3_run();  // Main function of process 3
void P1_sig_handler(int signo); // signal interpreter of process 1
void P2_sig_handler(int signo); // signal interpreter of process 2
void P3_sig_handler(int signo); // signal interpreter of process 3

void P1_manual();   // read data from stdin
void P1_file();     // read data from file

void P2_convert(char * input, char * output);   // convert 'input' to hex and save in 'output'

int buildTheSyncStructs();          // build   the synchronization structures
int removeTheSyncStructs();         // remove  the synchronization structures
void callTheProcesses();            // build and run the child processes
void Main_sig_handler(int signo);   // signal interpreter of main process

/*
    ---------------
   -- GLOBAL VARS --
    ---------------
*/
pid_t p1, p2, p3;    // PIDs
bool running;        // flag for 'pause' or 'resume' program

key_t shmkey;       // shared memory IPC key
int shmid;          // shared memory ID
int *p;             // temp ptr to shared memory

int pipe_1_2[2];                         // pipe:        P1 -> P2
char * file_data = "./.data_2_3.txt";    // file:        P2 -> P3
sem_t *mutex_2;                          // semaphore: P2
sem_t *mutex_3;                          // semaphore: P3

/*
    --------------
   -- MAIN DRIVE --
    --------------
*/
int main()
{
    signal(SIGINT,  Main_sig_handler);
    signal(SIGUSR1, Main_sig_handler);
    signal(SIGUSR2, Main_sig_handler);

    fprintf(stdout, "MAIN[%d]: Building the sychronization structures...\n", getpid());
    if(buildTheSyncStructs() != 0) {
        printf("\nMAIN[%d]: End of program - FAILURE.\n\n", getpid());
        exit(EXIT_FAILURE);
    } else {
        fprintf(stdout, "MAIN[%d]: Sychronization structures build.\n\n", getpid());    
    }

    callTheProcesses();

    fprintf(stdout, "\nMAIN[%d]: Removing the sychronization structures...\n", getpid());
    if(removeTheSyncStructs() != 0){
        printf("\nMAIN[%d]: End of program - FAILURE.\n\n", getpid());
        exit(EXIT_FAILURE);
    } else {
        fprintf(stdout, "MAIN[%d]: Sychronization structures removed.\n", getpid());    
        printf("\nMAIN[%d]: End of program - SUCCESS.\n\n", getpid());
        exit(EXIT_SUCCESS);
    }
}

/*
     -----------------
    -- PROCESS FUNCS --
     -----------------
*/
void P1_run()
{
    signal(SIGINT,  P1_sig_handler);
    signal(SIGUSR1, P1_sig_handler);
    signal(SIGUSR2, P1_sig_handler);

    close(pipe_1_2[READ]);

    int opt = -1;

    printf("P1[%d]: Ready.\n", getpid());

    while(true)
    {
        usleep(1000);

        printf("\n--------------------------\n");
        printf(" -- MENU -- \n");
        printf(" [1] - Manual\n");
        printf(" [2] - File\n");
        printf(" Choose option:\n");
        printf(" > ");
        fscanf(stdin, "%d", &opt);

        if(running)
        {
            switch(opt)
            {
                case 1:
                    P1_manual();
                    break;
                case 2:
                    P1_file();
                    break;
                default:
                    printf("\nChoose a correct option!\n");
                    break;
            }
        }
        opt = -1;
    }
    
    exit(EXIT_SUCCESS);
}

void P2_run()
{
    signal(SIGINT,  P2_sig_handler);
    signal(SIGUSR1, P2_sig_handler);
    signal(SIGUSR2, P2_sig_handler);

    close(pipe_1_2[WRITE]);

    FILE * fp;
    char * input = (char *) malloc(BUFF_SIZE * sizeof(char));
    char * output = (char *) malloc(BUFF_SIZE * sizeof(char));

    printf("P2[%d]: Ready.\n", getpid());
    
    while(true)
    {
        if((read(pipe_1_2[READ], input, sizeof(input))) > 0)
        {
            P2_convert(input, output);

            //printf("\nP2: %d : %s\n", strlen(input), input);
            //printf("\nP2: %d : %s\n", strlen(output), output);

            sem_wait(mutex_2);
            
            fp = fopen(file_data, "w");
            fprintf(fp, "%s", output);
            fclose(fp);

            sem_post(mutex_3);

            memset(input, 0, BUFF_SIZE);
            memset(output, 0, BUFF_SIZE);
        }
    }
    
    exit(EXIT_SUCCESS);
}

void P3_run()
{
    signal(SIGINT,  P3_sig_handler);
    signal(SIGUSR1, P3_sig_handler);
    signal(SIGUSR2, P3_sig_handler);

    FILE * fp;
    char * result = (char *) malloc(2 * BUFF_SIZE * sizeof(char));
    int chrPrinted = 0;
    int position = 0;

    printf("P3[%d]: Ready.\n", getpid());
    
    while(true)
    {
        sem_wait(mutex_3);

        fp = fopen(file_data, "r");

        if(fp != NULL){
            fgets(result, sizeof(result), fp);

            //printf("P3[%d]: %s\n", getpid(), result);
            
            int i = 0;
            char c = result[i];

            while(c != '\0'){
                printf("%c", c);
                chrPrinted++;
                
                if(chrPrinted == 2){
                    printf(" ");
                    position++;
                    chrPrinted = 0;
                }
                if(position == 15){
                    printf("\n");
                    position = 0;
                }

                i++;
                c = result[i];
            }

            fclose(fp);
        }
        
        sem_post(mutex_2);

        memset(result, 0, BUFF_SIZE);
    }

    exit(EXIT_SUCCESS);
}

void P1_manual()
{
    char * buffer = (char *) malloc(BUFF_SIZE * sizeof(char));

    while(true)
    {
        usleep(400);

        printf("\nType the data ('exit' to back to menu):\n");
        printf(" > ");
        fgets(buffer, sizeof(buffer), stdin);

        buffer[strcspn(buffer, "\n")] = '\0';

        if(strncmp(buffer, "exit", 4) == 0) break;

        write(pipe_1_2[WRITE], buffer, strlen(buffer));

        memset(buffer, 0, BUFF_SIZE);
    }
}

void P1_file()
{
    FILE * fp;
    char filepath[BUFF_SIZE];
    char * buffer = (char *) malloc(2 * BUFF_SIZE * sizeof(char));

    printf("\nType the filepath: ");
    fscanf(stdin, "%s", &filepath);

    if((fp = fopen(filepath, "r")) != NULL)
    {
        while(true)
        {
            if(running)
            {
                if(fgets(buffer, sizeof(buffer), fp) == NULL) break;

                write(pipe_1_2[WRITE], buffer, strlen(buffer));

                memset(buffer, 0, BUFF_SIZE);
            }
        }
        
        fclose(fp);
    } else {
        fprintf(stderr, "\n %s : Cannot open the file.\n", filepath);
    }
}

void P2_convert(char * input, char * output)
{
    int i = 0;
    char * d2c = input;
    char * n = output;

    while(*d2c != '\0')
    {
        sprintf(n, "%02X ", *d2c++);
        n += 2;
    }
    (*n) = '\0';
}


void P1_sig_handler(int signo)
{
    //fprintf(stderr, "P1[%d]: Received signal: %d\n", getpid(), signo);
    
    if(signo == SIGINT)
    {
        // end
        kill(getppid(), SIGINT);
    }
    else if(signo == SIGUSR1)
    {
        // pause
        running = false;
    }
    else if(signo == SIGUSR2)
    {
        // resume
        running = true;
    }  
    else
    {
        fprintf(stderr, "P1[%d]: Received unknown signal: %d\n", getpid(), signo);
    }
}

void P2_sig_handler(int signo)
{
    //fprintf(stderr, "P2[%d]: Received signal: %d\n", getpid(), signo);
    
    if(signo == SIGINT)
    {
        // end
        kill(getppid(), SIGINT);
    }
    else if(signo == SIGUSR1)
    {
        // pause
        kill(getppid(), SIGUSR1);
    }
    else if(signo == SIGUSR2)
    {
        // resume
        kill(getppid(), SIGUSR2);
    }  
    else
    {
        fprintf(stderr, "P2[%d]: Received unknown signal: %d\n", getpid(), signo);
    }
}

void P3_sig_handler(int signo)
{
    //fprintf(stderr, "P3[%d]: Received signal: %d\n", getpid(), signo);
    
    if(signo == SIGINT)
    {
        // end
        kill(getppid(), SIGINT);
    }
    else if(signo == SIGUSR1)
    {
        // pause
        kill(getppid(), SIGUSR1);
    }
    else if(signo == SIGUSR2)
    {
        // resume
        kill(getppid(), SIGUSR2);
    }  
    else
    {
        fprintf(stderr, "P3[%d]: Received unknown signal: %d\n", getpid(), signo);
    }
}


/*
     --------------
    -- MAIN FUNCS --
     --------------
*/
int buildTheSyncStructs()
{
    bool passed = true;

    running = true;

    if(pipe(pipe_1_2) == -1){
        fprintf(stderr, "MAIN[%d]: Error creating pipe\n", getpid());
        passed = false;
    }

    shmkey = ftok ("/dev/null", 5);
    shmid = shmget (shmkey, sizeof (int), 0644 | IPC_CREAT);
    if (shmid < 0){
        fprintf(stderr, "MAIN[%d]: Error getting shared memory segment\n", getpid());
        passed = false;
    }

    p = (int *) shmat (shmid, NULL, 0);
    *p = 0;

    mutex_2 = sem_open ("sem_2", O_CREAT | O_EXCL, 0644, 1);
    mutex_3 = sem_open ("sem_3", O_CREAT | O_EXCL, 0644, 0);

    FILE * fp;
    if((fp = fopen(file_data, "w")) == NULL){
        fprintf(stderr, "MAIN[%d]: Error creating pipe\n", getpid());
        passed = false;
    } else {
        fclose(fp);
    }

    if(passed) return 0;
    else return -1;
}

int removeTheSyncStructs()
{
    bool passed = true;

    if(close(pipe_1_2[READ]) != 0){
        fprintf(stderr, "MAIN[%d]: Error closing pipe READ\n", getpid());
        passed = false;
    }

    if(close(pipe_1_2[WRITE]) != 0){
        fprintf(stderr, "MAIN[%d]: Error closing pipe WRITE\n", getpid());
        passed = false;
    }


    shmdt (p);
    shmctl (shmid, IPC_RMID, 0);

    sem_unlink("sem_2");
    sem_unlink("sem_3");

    sem_close(mutex_2);
    sem_close(mutex_3);

    char * cmd = (char *) malloc(64 * sizeof(char));
    sprintf(cmd, "rm %s", file_data);
    system(cmd);

    if(passed) return 0;
    else return -1;
}

void callTheProcesses()
{
    ((p1 = fork()) && (p2 = fork()) && (p3 = fork()));

    if(p1 == 0) // Process 1
    {
        P1_run();
    }
    else if(p2 == 0) // Process 2
    {
        P2_run();
    }
    else if(p3 == 0) // Process 3
    {
        P3_run();
    }
    else // Process Main
    {
        wait(NULL); // waiting for child processes
    }
}

void Main_sig_handler(int signo)
{
    //fprintf(stderr, "MAIN[%d]: Received signal: %d\n", getpid(), signo);

    if(signo == SIGINT)
    {
        kill(p1, SIGTERM);
        kill(p2, SIGTERM);
        kill(p3, SIGTERM);
    }
    else if(signo == SIGUSR1)
    {
        kill(p1, SIGUSR1);
    }
    else if(signo == SIGUSR2)
    {
        kill(p1, SIGUSR2);
    }
    else
    {
        fprintf(stderr, "MAIN[%d]: Received unknown signal: %d\n", getpid(), signo);
    }
}