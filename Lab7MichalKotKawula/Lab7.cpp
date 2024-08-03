#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define LEN 32

int main(int argc, char *argv[]) {

    //take 3 arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <command1> <command2>\n", argv[0]);
        exit(-1);
    }

    char argument1[LEN];
    char argument2[LEN];
    strcpy(argument1, argv[1]);
    strcpy(argument2, argv[2]);

    char arg1[3][LEN]; 
    char arg2[3][LEN]; 
    int len1 = 0, len2 = 0;
    
    //tokenizing the first argument store in arg1 and keep track of amount in len1
    char *token = strtok(argument1, " ");
    while (token != NULL) {
        strcpy(arg1[len1], token);
        token = strtok(NULL, " ");
        ++len1;
    }
   
   //tokenizing the first argument store in arg2 and keep track of amount in len2
    token = strtok(argument2, " ");
    while (token != NULL) {
        strcpy(arg2[len2], token);
        token = strtok(NULL, " ");
        ++len2;
    }
  
    char *arg1_exec[len1 + 1];
    char *arg2_exec[len2 + 1];

    // copy tokens from both arrays to new arrays with NULL at the end.
    for (int i = 0; i < len1; ++i) {
        arg1_exec[i] = arg1[i];
    }
    
    arg1_exec[len1] = NULL;

    for (int i = 0; i < len2; ++i) {
        arg2_exec[i] = arg2[i];
    }
    
    arg2_exec[len2] = NULL;

   
   
   // create the pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(-1);
    }

  // fork to create child process
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(-1);
    }

    //close the first child pipe and redirect standard output to end of pipe
    if (pid1 == 0) {    
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(arg1_exec[0], arg1_exec) == -1) {
            perror("execvp");
            exit(-1);
        }
    }
    
    
    //create second child process
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(-1);
    }

    if (pid2 == 0) {
    //close the first child pipe and redirect standard output to end of pipe
        close(pipefd[1]); 
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        if (execvp(arg2_exec[0], arg2_exec) == -1) {
            perror("execvp");
            exit(-1);
        }
    }
    
    //close both ends of the pipe in parent
    close(pipefd[0]);
    close(pipefd[1]);

    //wait for child processes to end
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}

