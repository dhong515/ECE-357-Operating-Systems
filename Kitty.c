// Danny Hong and Ravindra Bisram 
// ECE 357 Problem Set 1 Question 3 
// Kitty Source Code written in C

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char* argv[]){
    int bufferSize = 4096, systemReadCalls = 0, systemWriteCalls = 0, totalWritten = 0, writeCount = 0, readCount, outputFd = STDOUT_FILENO, inputFd, flag;
    char *outfile = "";
    char *buffer = malloc((sizeof(char)) * bufferSize);

    /*Checks for the possibility of a malloc error when defining the buffer.*/
    if(buffer == NULL) {
        fprintf(stderr, "Error! Malloc error. \n%s", strerror(errno));
    }

    /*Uses getopt to search for the [-o outfile] and [-b ###] options.*/
    while((flag = getopt(argc, argv, "b:o:")) != -1){
        switch(flag){
            case 'o':
                outfile = optarg;
                break;
            case '?':
                fprintf(stderr, "Error! Unrecognized argument detected.");
                return -1;
            default:
                fprintf(stderr, "Error! Incorrect format: No argument following the -o and -b flags. \n%s\n", argv[0]);
                return -1;
        }
    }

    /*Checks to see if there is a specified outfile. If there is, it is then opened.*/ 
    if(strcmp(outfile, "")){
        outputFd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        /*Returns an error statement if output file can't be opened for writing.*/
        if(outputFd < 0){
            fprintf(stderr, "Error! Cannot open output file: %s for writing. \n%s\n", outfile, strerror(errno));
            return -1;
        }
    }
    
    /*Checks for zero input. If there is, it is treated as a "-" and "-" is appended to argv.*/
    if (optind == argc){
        optind = argc = 0; 
        argv[argc++] = "-";
    }

    /*Iterates through the arguments beginning at optind + 1 and ending at the final argument.*/ 
    for(; optind < argc; optind++){
        /*Returns an error statement if input file can't be opened for writing.*/
        if(!(strcmp(argv[optind], "-"))){
            inputFd = STDIN_FILENO;
            argv[optind] = "standard input";
            }   
        else if((inputFd = open(argv[optind], O_RDONLY)) < 0){
            fprintf(stderr, "Error! Cannot open input file: %s for reading. \n%s\n", argv[optind], strerror(errno));
            return -1;
            }
        /*Implementing the read and write operations with correction to deal with partial writes.*/
        while ((readCount = read(inputFd, buffer, (sizeof(char)) * bufferSize)) != 0){
            if(readCount > 0){
                /*Returns an error warning if an input file happens to be binary file. */
                for(int index = 0; index < readCount; index++){
                    if(!(isspace(buffer[index]) || isprint(buffer[index]))){
                        fprintf(stderr, "Warning! Trying to concatenate the binary file: %s\n", argv[optind]);
                        return -1;
                    }
                }
		        systemReadCalls = systemReadCalls + 1;
                while(readCount > writeCount){
                    if((writeCount = write(outputFd, buffer, readCount)) < 0){
                        fprintf(stderr, "Error! Issues writing to output file: %s. \n%s\n", outfile, strerror(errno));
					    return -1;
                    }
                    systemWriteCalls = systemWriteCalls + 1;
                    totalWritten = totalWritten + writeCount;
                    readCount = readCount - writeCount;
                    writeCount = 0;
                    buffer = buffer + readCount;
                }
            }    
            else{
                fprintf(stderr, "Error! Issues reading input file: %s. \n%s\n", argv[optind], strerror(errno));
                return -1;
            }
        }
        if(inputFd != STDIN_FILENO){
            fprintf(stderr, "%d bytes transferred to output file: %s from input file: %s. Number of system read calls = %d. Number of system write calls = %d\n", totalWritten, outfile, argv[optind], systemReadCalls, systemWriteCalls);
            /*Returns an error statement if there are issues closing an input file that is not standard input.*/
            if(close(inputFd) < 0){
                fprintf(stderr, "Error! Cannot close input file: %s %s\n", argv[optind], strerror(errno));
				return -1;
            }
		} 
        else {
			fprintf(stderr, "%d bytes transferred to output file: <standard output> from input file: <standard input>. Number of system read calls = %d. Number of system write calls = %d\n", totalWritten, systemReadCalls, systemWriteCalls);
		}
    }
    
    /*Returns an error statement if there are issues closing an output file that is not standard output.*/
    if (close(outputFd) < 0 && outputFd != STDOUT_FILENO){
        fprintf(stderr, "Error! Cannot close output file: %s. \n%s\n", outfile, strerror(errno));
        return -1;
    }
    return 0;
}