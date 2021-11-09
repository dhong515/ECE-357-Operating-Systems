// OS PSET 3 Problem 3
// Ravindra Bisram, Danny Hong, Elias Rodriguez

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>

exit_code = 0;

void errorfunction(char* message, char* file);

// Function to print time data
void getTimeData(struct timeval* begin, struct timeval* end, struct rusage* ru) {
	fprintf(stdout, "Real: %ld.%06lds User: %ld.%06lds Sys: %ld.%06lds\n",
		end->tv_sec - begin->tv_sec, end->tv_usec - begin->tv_usec, ru->ru_utime.tv_sec,
		ru->ru_utime.tv_usec, ru->ru_stime.tv_sec, ru->ru_stime.tv_usec);
}

// Function to deal with the pwd command functionality
int pwdfunction() {
    char cwd[4096];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
		printf("\n");
    }
    else {
        errorfunction("Error: Could not get current working directory.", NULL);
		exit_code = -1;
    }
    return 0;
}

// Function to deal with the cd command functionality
int cdfunction(char* path) {
	char* newDirectory;
	if (path == NULL) {
		newDirectory = getenv("HOME");
	}
	else {
		newDirectory = path;
	}

	if (chdir(newDirectory) < 0) {
		errorfunction("Error: Unable to change directory path to ", path);
		exit_code = -1;
	}
	return 0;
}

// Hard Exit function 
void exitfunction(char* code) {
	if (code != NULL)
		exit(atoi(code));
	exit(exit_code);
}

//  Function to display errors 
void errorfunction(char* message, char* file) {
	if (file)
		fprintf(stderr, "%s [%s]. Error code %i: %s.\n", message, file, exit_code, strerror(errno));
	else
		fprintf(stderr, "%s\n", message);
}

int run(char **argVec, char *redirFile, int dupFD, int redirMode){
    int fd, status;
    pid_t process, waiting;
    struct rusage rusage;
    struct timeval timeBegin, timeEnd;
    int resultgettimeofday = gettimeofday(&timeBegin, NULL);

	// Start timer
    if (resultgettimeofday < 0)  {
        errorfunction("Error: Unable to start timing command execution.", NULL);
        return 1;
    }

	// Creating Child Process
    switch ((process = fork()))
    {
    case -1:
        errorfunction("Error: Unable to fork process", NULL);
        exit(1);
        break;
    case 0:
        if (dupFD > -1)
        {
            fd = open(redirFile, redirMode, 0666);
            if (fd < 0) {
                errorfunction("Error: Unable to open file for redirection", redirFile);
                return 1;
            }
            if (dup2(fd, dupFD) < 0) {
                errorfunction("Error: Unable to redirect to appropriate file.", NULL);
                return 1;
            }
            if (close(fd) < 0) {
				errorfunction("Error: Unable to close redirected file descriptor.", NULL);
                return 1;
            }
        }

        if (execvp(argVec[0], argVec) == -1) {
            errorfunction("Error: Unable to execute given command", argVec[0]);
			exit_code = 127;
			exit(exit_code);
        }
        
        break;

    default:		
		if ((waiting = wait3(&status, WUNTRACED, &rusage)) < 0) {
			errorfunction("Error! Failed to wait for child process to finish", NULL);
			return 1;
		}
		
		if (status == 0) {
			fprintf(stderr, "\nChild process %d exited normally.\n", process);
		}
		else {
			if (!(WIFSIGNALED(status))) {
				//fprintf(stdout, "Status is: %d\n", status);
				exit_code = WEXITSTATUS(status);
				fprintf(stderr, "\nChild process %d exited with exit status %d.\n", process, exit_code);
			}
			else {
				exit_code = WEXITSTATUS(status);
				fprintf(stderr, "Child process %d exited with signal %d.\n", process, exit_code);
			}
		}

		// Stop timer
        resultgettimeofday = gettimeofday(&timeEnd, NULL);
		if (resultgettimeofday < 0) {
			errorfunction("Error: Unable to start timing command execution.", NULL);
			return 1;
		}

		getTimeData(&timeBegin, &timeEnd, &rusage);
		fprintf(stdout, "\n");

        break;
    }
    return 0;
}

void readcommand(FILE *infile) {
    char *line = NULL, *delims = " \r\n", *arg;
    int dupFD = -1, redirMode, i = 0;
    size_t len = 0;
    ssize_t bytesRead;
    char **argVec = malloc(BUFSIZ * (sizeof(char *)));
    if (argVec == NULL){
        errorfunction("Error: Failure to dynamically allocate memory.", NULL);
    }
    char *redirFile = malloc(BUFSIZ * (sizeof(char)));
    if (redirFile == NULL){
        errorfunction("Error: Failure to dynamically allocate memory.", NULL);
    }
    while ((bytesRead = getline(&line, &len, infile)) != -1){
        if (bytesRead <= 1 || line[0] == '#'){
            continue;
        }
        else
        {
            arg = strtok(line, delims);
            while (arg != NULL)
            {
				// < filename -- Open filename and redirect stdin
                if (arg[0] == '<') {
                    dupFD = 0;
                    redirMode = O_RDONLY;
                    strcpy(redirFile, (arg + 1));
                }
                else if (arg[0] == '>') {
				// >>filename Open/Create/Append filename and redirect stdout
					if (arg[1] == '>') {
                        redirMode = O_WRONLY | O_APPEND | O_CREAT;
                        strcpy(redirFile, (arg + 2));
                    }
				// >filename Open/Create/Truncate filename and redirect stdout
                    else {
                        redirMode = O_WRONLY | O_TRUNC | O_CREAT;
                        strcpy(redirFile, (arg + 1));
                    }
                    dupFD = 1;
                }
                else if (arg[0] == '2' && arg[1] == '>') {
				// 2>>filename Open/Create/Append filename and redirect stderr
                    if (arg[2] == '>') {
                        redirMode = O_WRONLY | O_APPEND | O_CREAT;
                        strcpy(redirFile, (arg + 3));
                    }
				// 2>filename Open/Create/Truncate filename and redirect stderr
                    else {
                        redirMode = O_WRONLY | O_TRUNC | O_CREAT;
                        strcpy(redirFile, (arg + 2));
                    }
                    dupFD = 2;
                }
                else {
                    argVec[i++] = arg;
                }
                arg = strtok(NULL, delims);
            }

            argVec[i] = NULL;

            if (strcmp(argVec[0], "pwd") == 0) {
                pwdfunction();
            }
            else if (strcmp(argVec[0], "cd") == 0) {
				cdfunction(argVec[1]);
            }
            else if (strcmp(argVec[0], "exit") == 0) {
                exitfunction(argVec[1]);
            }
            else {
                if ((run(argVec, redirFile, dupFD, redirMode)) > 0) {
                    exit(1);
                }
            }

            dupFD = -1;
            redirMode = 0;
            i = 0;
        }
    }

    free(redirFile);
    free(argVec);
    free(line);
    return;
}

int main(int argc, char *argv[]){
    FILE *infile;

    if (argc > 1){
        if ((infile = fopen(argv[1], "r")) == NULL){
            errorfunction("Error: Unable to open input file", argv[1]);
            return -1;
        }
        readcommand(infile);
    }
    else {
        readcommand(stdin);
    }
    int closeFile = fclose(infile);
    if (closeFile != 0 && argc > 1) {
        errorfunction("Error: Unable to close input file", argv[1]);
        return -1;
    }
    return 0;
}