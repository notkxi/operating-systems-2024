// Name: Kai Ibarrondo
// RUID: 210004237

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

void cd(char *directory);

void pwd();

void help();

void makdir(char *directory);

void rmvdir(char *directory);

void ls(char *directory);

void cp(char *source, char *destination);

void mv(char *source, char *destination);

void rm(char *file);

void echo(char *str);

void cat(char *source);

void grep(char *pattern, char *source);

void head(char *flag, char *num, char *source);

void tail(char *flag, char *num, char *source);

void wc(char *source);

void touch(char *destination);

int isDirectory(const char *path);

int main() {
    char input[1024];

    while (1) {
        printf("myshell> ");
        fgets(input, 1024, stdin);

        input[strcspn(input, "\n")] = 0;

        char *input_copy = strdup(input);
        if (input_copy == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        // Tokenize the input into command and arguments
        char *command = strtok(input_copy, " ");
        if (command == NULL) {
            // No command entered, continue to the next iteration
            continue;
        }

        char *arg1 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, " ");
        char *arg3 = strtok(NULL, " ");

        // Execute the corresponding command based on user input
        if (strcmp(command, "cd") == 0) {
            cd(arg1);
        } else if (strcmp(command, "pwd") == 0) {
            pwd();
        } else if (strcmp(command, "exit") == 0) {
            exit(0);
        } else if (strcmp(command, "help") == 0) {
            help();
        } else if (strcmp(command, "mkdir") == 0) {
            makdir(arg1);
        } else if (strcmp(command, "rmdir") == 0) {
            rmvdir(arg1);
        } else if (strcmp(command, "ls") == 0) {
            ls(arg1);
        } else if (strcmp(command, "cp") == 0) {
            cp(arg1, arg2);
        } else if (strcmp(command, "mv") == 0) {
            mv(arg1, arg2);
        } else if (strcmp(command, "rm") == 0) {
            rm(arg1);
        } else if (strcmp(command, "echo") == 0) {
            echo(input);
        } else if (strcmp(command, "cat") == 0) {
            cat(arg1);
        } else if (strcmp(command, "grep") == 0) {
            grep(arg1, arg2);
        } else if (strcmp(command, "head") == 0) {
            head(arg1, arg2, arg3);
        } else if (strcmp(command, "tail") == 0) {
            tail(arg1, arg2, arg3);
        } else if (strcmp(command, "wc") == 0) {
            wc(arg1);
        } else if (strcmp(command, "touch") == 0) {
            touch(arg1);
        } else {
            printf("command '%s' not found\n", command);
        }
    }
    return 0;
}


// Function to change directory
void cd(char *directory) {
    // Check input is empty or if chdir completed successfully
    if (directory != NULL && chdir(directory) != 0) {
        printf("myshell: cd: %s: No such file or directory\n", directory);
    }
}

// Function to print current working directory
void pwd() {
    // Allocate 1024 bytes to store the current working directory
    char *buf;
    buf = (char *) malloc(1024 * sizeof(char));
    // Print current working directory, if unsuccessful print error message
    if (getcwd(buf, 1024) != NULL) {
        printf("%s\n", buf);
    } else {
        printf("myshell: pwd: Unable to print the current working directory\n");
    }
    // Free memory to avoid memory leaks
    free(buf);
}

// Function to display help information
void help() {
    printf(
            "cd <directory> - change directory\n"
            "pwd - print working directory\n"
            "exit - exit the shell\n"
            "help - display help\n"
            "mkdir <directory> - create a directory\n"
            "rmdir <directory> - remove a directory\n"
            "ls <directory> - list files in a directory\n"
            "cp <source> <destination> - copy a file\n"
            "mv <source> <destination> - move a file\n"
            "rm <file> - remove a file\n"
            "echo <message> - print message\n"
            "cat <file> - display file content\n"
            "grep <pattern> <file> - search for pattern in file\n"
            "head [-n <num>] <file> - display first lines of file\n"
            "tail [-n <num>] <file> - display last lines of file\n"
            "wc <file> - count lines, words, and characters in file\n"
            "touch <file> - create an empty file\n");
}

// Function to create a directory
void makdir(char *directory) {
    // Check if input is empty
    if (directory == NULL) {
        printf("mkdir: missing operand\n");
        return;
    }
    // Check if mkdir was successful, if not print error message
    if (mkdir(directory, 0777) == 0) {
        return;
    } else {
        printf("mkdir: cannot create directory '%s': File exists\n", directory);
    }
}

// Function to remove a directory
void rmvdir(char *directory) {
    if (directory == NULL) {
        // Check if input is empty
        printf("rmdir: missing operand\n");
        return;
    }

    // Open the directory to check if it's empty to throw the appropriate error
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) != NULL) {
        // Check if the directory is empty
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // Directory is not empty
                closedir(dir);
                printf("rmdir: failed to remove '%s': Directory not empty\n", directory);
                return;
            }
        }

        // Directory is empty, remove it
        closedir(dir);
        if (rmdir(directory) != 0) {
            printf("rmdir: failed to remove '%s': No such file or directory\n", directory);
        }
    } else {
        printf("rmdir: failed to remove '%s': No such file or directory\n", directory);
    }
}

// Function to list files in a directory
void ls(char *directory) {

    DIR *dir;
    struct dirent *entry;

    // If no directory is provided, use the current directory
    if (directory == NULL) {
        directory = ".";
    }
    // Variable to determine if a file exist
    int fileCount = 0;

    if ((dir = opendir(directory)) != NULL) {
        // Iterate pointer through the directory entries
        while ((entry = readdir(dir)) != NULL) {
            // Exclude entries starting with a dot (Linux view files starting with '.' as hidden).
            if (entry->d_name[0] != '.') {
                printf("%s  ", entry->d_name);
                fileCount = 1;
            }
        }

        if (fileCount > 0) {
            // Add a newline if there are non-dot entries
            printf("\n");
        }

        closedir(dir);
    } else {
        printf("ls: cannot access '%s': No such file or directory\n", directory);
    }
}

// Function to copy a file
void cp(char *source, char *destination) {
    if (source == NULL || destination == NULL) {
        printf("cp: missing file operands\n");
    } else if (isDirectory(source) && isDirectory(destination)) {
        printf("mv: cannot copy directory into directory\n");
    } else if (isDirectory(source)) {
        printf("mv: cannot copy directory into a file\n");
    } else {
        FILE *srcfile = fopen(source, "r");
        FILE *dstfile;

        if (srcfile != NULL) {
            // Check if the destination is an existing directory
            if (isDirectory(destination)) {
                // Append the source file name to the destination path
                char *filename = strrchr(source, '/');
                if (filename == NULL) {
                    // If no '/' is found, use the whole source as the filename
                    filename = source;
                }
                    // Move past the '/' character
                else {
                    filename++;
                }
                // Allocate memory for the new destination path, considering directory separator and null terminator
                char *newDestination = (char *) malloc(strlen(destination) + strlen(filename) + 2);
                strcpy(newDestination, destination);
                strcat(newDestination, "/");
                strcat(newDestination, filename);

                dstfile = fopen(newDestination, "w");
                free(newDestination);
            } else {
                // The destination is a file
                dstfile = fopen(destination, "w");
            }

            if (dstfile != NULL) {
                // Move the file pointer to the end of the source file to get its size
                fseek(srcfile, 0, SEEK_END);
                long fsize = ftell(srcfile);
                fseek(srcfile, 0, SEEK_SET);
                // Allocate memory to store the content of the source file
                unsigned char *buffer = (unsigned char *) malloc(fsize);
                // Check if memory allocation is successful
                if (buffer == NULL) {
                    perror("memory allocation error");
                }

                // Read the content of the source file into the allocated buffer
                fread(buffer, sizeof(unsigned char), fsize, srcfile);
                // Write the content from the buffer to the destination file
                fwrite(buffer, sizeof(unsigned char), fsize, dstfile);
                // Free the allocated memory
                free(buffer);

                fclose(srcfile);
                fclose(dstfile);
            } else {
                printf("cp: cannot open destination file '%s'\n", destination);
            }
        } else {
            printf("cp: cannot stat '%s': No such file or directory\n", source);
        }
    }
}

// Function to move a file
void mv(char *source, char *destination) {
    // Check if either source or destination is missing
    if (source == NULL || destination == NULL) {
        printf("mv: missing file operands\n");
    }
        // Check if moving a directory into another directory
    else if (isDirectory(source) && isDirectory(destination)) {
        printf("mv: cannot move directory into directory\n");
    }
        // Check if moving a directory into a file
    else if (isDirectory(source)) {
        printf("mv: cannot move directory into a file\n");
    } else {
        FILE *srcfile = fopen(source, "r");
        FILE *dstfile;

        if (srcfile != NULL) {
            // Check if the destination is an existing directory
            if (isDirectory(destination)) {
                // Append the source file name to the destination path
                char *filename = strrchr(source, '/');
                // If no '/' is found, use the whole source as the filename
                if (filename == NULL) {
                    filename = source;
                }
                    // Move past the '/' character
                else {
                    filename++;
                }

                // Allocate memory for the new destination path, considering directory separator and null terminator
                char *newDestination = (char *) malloc(strlen(destination) + strlen(filename) + 2);
                strcpy(newDestination, destination);
                strcat(newDestination, "/");
                strcat(newDestination, filename);

                dstfile = fopen(newDestination, "w");
                free(newDestination);
            } else {
                // The destination is a file
                dstfile = fopen(destination, "w");
            }
            // Check if the destination file is successfully opened
            if (dstfile != NULL) {
                // Move the file pointer to the end of the source file to get its size
                fseek(srcfile, 0, SEEK_END);
                long fsize = ftell(srcfile);
                fseek(srcfile, 0, SEEK_SET);

                // Allocate memory to store the content of the source file
                unsigned char *buffer = (unsigned char *) malloc(fsize);
                // Check if memory allocation is successful
                if (buffer == NULL) {
                    perror("memory allocation error");
                }
                // Read the content of the source file into the allocated buffer
                fread(buffer, sizeof(unsigned char), fsize, srcfile);
                // Write the content from the buffer to the destination file
                fwrite(buffer, sizeof(unsigned char), fsize, dstfile);
                // Free the allocated memory
                free(buffer);

                fclose(srcfile);
                fclose(dstfile);

                // Remove the source file after successful copy
                if (remove(source) != 0) {
                    printf("mv: cannot remove '%s': Error removing source file\n", source);
                }
            } else {
                printf("mv: cannot open destination file '%s'\n", destination);
            }
        } else {
            printf("mv: cannot stat '%s': No such file or directory\n", source);
        }
    }
}

// Function to remove a file or directory
void rm(char *file) {
    // Check if the file path is empty
    if (file == NULL) {
        printf("rm: missing operand\n");
        return;
    }
    // Check if the path is a directory
    if (isDirectory(file) != 1) {
        // Attempt to remove the file
        if (remove(file) == 0) {
            // File removal successful
            printf("File '%s' removed successfully.\n", file);
        } else {
            // Unable to remove file as it does not exist
            printf("rm: cannot remove '%s': No such file\n", file);
        }
    } else {
        // Cannot remove a directory using 'rm'
        printf("rm: cannot remove '%s': Is a directory\n", file);
    }
}

void echo(char *str) {
    for (int i = 5; i < strlen(str); i++){
        printf("%c", str[i]);
    }
    printf("\n");
}

void cat(char *source){
    char c;

    FILE *fptr = fopen(source, "r");
    if (fptr == NULL)
    {
        printf("cat: %s: No such file or directory\n", source);
    }
    else{
        // Read contents from file
        c = fgetc(fptr);
        while (c != EOF)
        {
            printf ("%c", c);
            c = fgetc(fptr);
        }
        fclose(fptr);
    }
}

void grep(char *pattern,char *source) {
    int match = 0;

    if(pattern == NULL || source == NULL){
        printf("grep: missing operands\n");
        return;
    }

    FILE *file = fopen(source, "r");
    if (file == NULL) {
        printf("grep: %s: No such file or directory\n", source);
        return;
    }

    char buffer[1024];
    int line_number = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        line_number++;
        if (strstr(buffer, pattern) != NULL) {
            printf("%d:%s", line_number, buffer);
            match = 1;
        }
    }
    if(match == 1){
        printf("\n");
    }
    fclose(file);
}

void head(char *flag, char *num, char *source) {
    int n;

    if (flag == NULL) {
        printf("head: missing operands\n");
        return;
    }
    else {
        if (strcmp(flag, "-n") != 0) {
            n = 10;
            source = flag;
        }
        else {
            if (num == NULL) {
                printf("head: option requires an argument -- 'n'\n");
                printf("Try 'help' for more information.\n");
                return;
            }
            else {
                if (!isdigit(*num)) {
                    printf("head: invalid number of lines: %s\n", num);
                    return;
                }
                n = atoi(num); // Move the conversion here
            }
        }

        FILE *file = fopen(source, "r");
        if (file == NULL) {
            printf("head: cannot open '%s' for reading: No such file or directory\n", source);
            return;
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        int lines_printed = 0;

        // Read lines from the file until reaching the specified number of lines or end of file
        while (lines_printed < n && (read = getline(&line, &len, file)) != -1) {
            printf("%s", line); // Print the line
            lines_printed++;
        }
        free(line); // Free the memory allocated
        fclose(file);
    }
}

void tail(char *flag, char *num, char *source) {
    int n;

    if (flag == NULL) {
        printf("tail: missing operands\n");
        return;
    }
    else {
        if (strcmp(flag, "-n") != 0) {
            n = 10;
            source = flag;
        } else {
            if (num == NULL) {
                printf("tail: option requires an argument -- 'n'\n");
                printf("Try 'help' for more information.\n");
                return;
            } else {
                if (!isdigit(*num)) {
                    printf("tail: invalid number of lines: %s\n", num);
                    return;
                }
                n = atoi(num);
            }
        }

        FILE *srcfile = fopen(source, "r");
        if (srcfile == NULL) {
            printf("tail: cannot open '%s' for reading: No such file or directory\n", source);
            return;
        }

        fseek(srcfile, 0, SEEK_END);
        long fsize = ftell(srcfile);
        fseek(srcfile, 0, SEEK_SET);
        // Allocate memory to store the content of the source file
        unsigned char *buffer = (unsigned char *) malloc(fsize);
        // Check if memory allocation is successful
        if (buffer == NULL) {
            perror("memory allocation error");
        }

        // Read the content of the source file into the allocated buffer
        fread(buffer, sizeof(unsigned char), fsize, srcfile);
        buffer[fsize] = '\0';

        fclose(srcfile); // Close the file

        int newline_count = 0;
        char *current_pos = buffer + fsize; // Start from the end of the buffer

        // Iterate backward until n new lines are found or we reach the beginning of the buffer

        if (*(current_pos - 1) == '\n'){
            n++;
        }

        while (newline_count < n && current_pos > buffer) {
            current_pos--;
            if (*current_pos == '\n') {
                newline_count++;
            }
        }
        // Print everything after the last newline character found
        if (*current_pos == '\n') {
            printf("%s", current_pos + 1);
        }
        else {
            printf("%s", current_pos);
        }

        // Free allocated memory
        free(buffer);
    }
}

void wc(char *source) {
    FILE *file = fopen(source, "r");
    if (file == NULL) {
        printf("wc: cannot open '%s' for reading: No such file or directory\n", source);
        return;
    }

    int lines = 0;
    int words = 0;
    int characters = 0;
    int in_word = 0;
    char c;

    while ((c = fgetc(file)) != EOF) {
        characters++;

        if (c == '\n') {
            lines++;
        }

        if (!isspace(c)) {
            if (!in_word) {
                in_word = 1;
                words++;
            }
        }
        else {
            in_word = 0;
        }
    }
    fclose(file);
    printf("%d %d %d %s\n", lines, words, characters, source);
}

void touch(char *destination) {
    if (access(destination, F_OK) == 0) {
        return;
    }
    else {
        FILE *file = fopen(destination, "w");
        if (file == NULL){
            perror("touch: error creating file");
        }
        fclose(file);
    }
}

// Function to check if a given path is a directory
int isDirectory(const char *path) {
    struct stat buf;
    // Use stat() to retrieve information about the file
    if (stat(path, &buf) != 0) {
        return 0;
    }
    // Check if the obtained information indicates a directory
    return S_ISDIR(buf.st_mode);
}
