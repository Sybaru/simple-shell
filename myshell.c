#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXLINE 1024
#define PIPE_READ 0
#define PIPE_WRITE 1

static jmp_buf env;
static volatile sig_atomic_t jump_active = 0;

void sigint_handler(int signo)
{
    if (!jump_active)
    {
        return;
    }
    siglongjmp(env, 42);
}

void zombie(int signum) {
    wait(NULL);
}

int main()
{
    char *command[100];
    char buffer[100]; // For commands with up to 100 characters
    signal(SIGINT, sigint_handler);
    while (1)
    {
        if (sigsetjmp(env, 1) == 42)
        {
            printf("\n");
        }
        jump_active = 1; /* Set the flag */
        printf("> %");

        // Read a string from command line:
        /* fgets creates null-terminated string. stdin is pre-defined C constant
         * for standard intput.  feof(stdin) tests for file:end-of-file.
         */

        if (fgets(buffer, MAXLINE, stdin) == NULL && feof(stdin))
        {
            printf("Couldn't read from standard input. End of file? Exiting ...\n");
            exit(1); /* any non-zero value for exit means failure. */
        }
        size_t length = strlen(buffer);
        if (buffer[length - 1] == '\n')
            buffer[length - 1] = '\0';

        int count = parse_command(buffer, command);

        int i = 0;

        for (i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] == '\n')
            {
                buffer[i] = '\0';
            }
        }

        int back = 0;
        int out = 0;
        int pip = 0;
        int piploc = 0;
        char output[64];

        for (i = 0; command[i] != '\0'; i++)
        {
            if (strcmp(command[i], ">") == 0)
            {
                command[i] = NULL;
                strcpy(output, command[i + 1]);
                out = 2;
            }
            else if (strcmp(command[i], "&") == 0)
            {
                command[i] = NULL;
                back = 2;
            }
            else if (strcmp(command[i], "|") == 0)
            {
                command[i] = NULL;
                piploc = i;
                pip = 2;
            }
        }

        if (pip)
        {
            int pipe_fd[2];       /* 'man pipe' says its arg is this type */
            int fd;               /* 'man dup' says its arg is this type */
            pid_t child1, child2; /* 'man fork' says it returns type 'pid_t' */

            char *prev[piploc+1];
            char *post[count - piploc];

            for (int i = 0; i < piploc; i++){
                prev[i] = command[i];
            }

            for (int j = 0; j < count - piploc; j++){
                post[j] = command[j + piploc + 1];
            }

            prev[piploc - 1] = NULL;
            post[count - piploc - 1] = NULL;

            pipe(pipe_fd);

            child1 = fork();
            if (child1 > 0)
            {
                child2 = fork();
            }

            if (child1 == 0)
            { 
                if (-1 == close(STDOUT_FILENO)) {
                    perror("close");
                }

                fd = dup(pipe_fd[PIPE_WRITE]); 
                if (-1 == fd) {
                    perror("dup");
                }

                if (fd != STDOUT_FILENO) {
                    fprintf(stderr, "Pipe output not at STDOUT.\n");
                }

                close(pipe_fd[PIPE_READ]);  
                close(pipe_fd[PIPE_WRITE]); 
                if (-1 == execvp(prev[0], prev)) {
                    perror("execvp");
                }

            }
            else if (child2 == 0)
            { 
                if (-1 == close(STDIN_FILENO)) {
                    perror("close");
                }
                fd = dup(pipe_fd[PIPE_READ]);
                if (-1 == fd) {
                    perror("dup");
                }

                if (fd != STDIN_FILENO) {
                    fprintf(stderr, "Pipe input not at STDIN.\n");
                }

                close(pipe_fd[PIPE_READ]); 
                close(pipe_fd[PIPE_WRITE]);
                if (-1 == execvp(post[0], post)) {
                    perror("execvp");
                }
            }
            else
            {
                int status;
                close(pipe_fd[PIPE_READ]);
                close(pipe_fd[PIPE_WRITE]);
                if (-1 == waitpid(child1, &status, 0)) {
                    perror("waitpid");
                }
                if (-1 == waitpid(child2, &status, 0)) {
                    perror("waitpid");
                }
            }
        }
        else //if not piping
        {
            int childpid = fork();
            if (childpid == 0)
            { // if child
                signal(SIGINT, SIG_DFL);
                char *prev[piploc + 1];
                char *post[count - piploc];
                prev[piploc] = NULL;
                post[count - piploc + 1] = NULL;

                if (out)
                {
                    int fd1 = creat(output, 0644);
                    dup2(fd1, STDOUT_FILENO);
                    close(fd1);
                    out = 0;
                }

                execvp(command[0], command); // See 'man execvp'
            }
            else
            {   // else parent
                if (!back) {
                    waitpid(childpid); // See 'man waitpid'
                }
            }
        }
    }
}

int parse_command(char buffer[], char *command[])
{
    char *token; // split command into separate strings
    token = strtok(buffer, " ");
    int i = 0;
    while (token != NULL)
    {
        command[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    command[i] = NULL;
    return i;
}
