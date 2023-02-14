// Made by Eli Lewis
// Due: 2-4-23 Completed: 2-5-23
// Cs 232 Calvin University
// Command Shell

// Useful run commands
// git add . && git commit -m "someday Ill be done with this " && git push && clear && gcc shell.c -g -o shell && ./shell
// gcc shell.c -g -o shell && gdb shell
// gcc shell.c -g -o shell && valgrind --leak-check=full --track-origins=yes  ./shell

// Theres a higher chance that I can fly than there is that there are no memory leaks in here
// Ok so according to valgrind theres ~ 150 bytes of lost memory - not bad,
// and I cant figure out where that is

// This shell does not support
// - strings as arguments eg. git -m "this doesnt work"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> //scandir
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
// #include <stddef.h>

int search_path(char *path, char *target)
{
    /// @brief finds if a file is present in the given path
    /// @param path a specific path to a directory
    /// @param target target file
    /// @return present? 1 else 0

    int found = 0;

    struct dirent **contents;

    // modified example code from "man scandir"
    // cant pass parameters to the filter function, basically makes it useless
    int num_results = scandir(path, &contents, NULL, alphasort);
    if (num_results == -1)
        perror("Scandir error");
    else
    {
        while (num_results--)
        {
            if (0 == (strcmp(contents[num_results]->d_name, target)))
            {
                found = 1;
            }

            free(contents[num_results]);
        }
        free(contents);
    }
    return found;
}

char *find(char **path, int *path_length, char *target)
{
    /// @brief finds if a file is present in the system path
    /// @param path the system path
    /// @param target target file
    /// @return a char * pointing to the full path of the target, or NULL if not found

    for (int i = 0; i < *path_length; i++)
    {
        if (1 == search_path(path[i], target))
        {

            char *result;
            result = (char *)malloc((strlen(path[i]) * sizeof(path[i][0])) + sizeof("/") + (strlen(target) * sizeof(char)) + sizeof("\0"));

            strcpy(result, path[i]);
            strcat(result, "/");
            strcat(result, target);
            strcat(result, "\0");

            return result;
        }
    }

    return NULL; // not found
}

char **import_path(int *path_length)
{
    /// @brief gets the path envrioment variable and then segments it into a char **
    /// @param path_length a pointer to ouput the length of the char ** to
    /// @return a char ** of all the paths in the path

    char *tmp_path = getenv("PATH");
    uint path_len = strlen(tmp_path);
    int path_section_count = 1;

    // count how many different path sections there are
    for (int i = 0; i < path_len; i++)
    {
        if (tmp_path[i] == ':')
        {
            path_section_count++;
        }
    }

    // segments path into array
    char segmented_path[path_section_count][path_len - (path_section_count * 2)];
    {
        int current_section = 0;
        int current_section_position = 0;
        for (int i = 0; i < path_len; i++)
        {
            if (tmp_path[i] == ':')
            {
                segmented_path[current_section][current_section_position] = '\0'; // null term for strcpy
                current_section++;
                current_section_position = 0;
            }
            else
            {
                segmented_path[current_section][current_section_position] = tmp_path[i];
                current_section_position++;
            }
        }
    }

    char **path = (char **)malloc((path_section_count) * sizeof(char *));

    for (int i = 0; i < path_section_count; i++)
    {
        path[i] = (char *)malloc(path_len - (path_section_count * 2) * sizeof(char));
        strcpy(path[i], segmented_path[i]);
    }

    *path_length = path_section_count;

    return path;
}

struct Parsed_command
{
    char *command_name; // name of command, eg. in ls -sh -a -l -E this would be "ls"
    int *foreground;    // run in the foreground?
    int *argc;          // number of arguments
    char **argv;        // arguments
};

/// @brief
/// @param command
/// @return
struct Parsed_command *parse_command(char *command)
{
    /// @brief parses a command into a command name, argc, argv, and if its a background command
    /// @param command the command as a unparsed string
    /// @return a parsed command struct

    struct Parsed_command *parsed_command;
    parsed_command = (struct Parsed_command *)malloc(sizeof(struct Parsed_command));
    parsed_command->argc = (int *)malloc(sizeof(int));
    parsed_command->foreground = (int *)malloc(sizeof(int));

    int argc = 0;
    int foreground = 1;

    char str[strlen(command)];
    strcpy(str, command);
    char *token;
    const char s[2] = " ";

    // partial string implementation
    // const char q = '\"';

    // char qoute_string[1000];  //max qoute size of 1000
    // char *first_qoute_pointer = NULL;
    // char *second_qoute_pointer = NULL;

    // // presumes null-termed
    // first_qoute_pointer = strchr(str, q);
    // if (first_qoute_pointer != NULL)
    // {
    //     second_qoute_pointer = strchr(++first_qoute_pointer, q);
    //     if (second_qoute_pointer != NULL)
    //     {
    //         char temp_str[strlen(command)];
    //         ptrdiff_t size_of_sub_string = (second_qoute_pointer-first_qoute_pointer);
    //         ptrdiff_t size_of_front_to_first_qoute = (first_qoute_pointer-str);
    //         ptrdiff_t size_of_end_to_last_qoute = (second_qoute_pointer-str);
    //         memmove(qoute_string, first_qoute_pointer, size_of_sub_string);
    //         memmove(qoute_string, str, size_of_front_to_first_qoute-1);
    //         memmove(qoute_string+size_of_front_to_first_qoute, str+size_of_sub_string+size_of_front_to_first_qoute+1, size_of_end_to_last_qoute);
    //         printf("%s\n",qoute_string);
    //     }
    // }

    // get argc first
    token = strtok(str, s);
    while (token != NULL)
    {
        if (strcmp(token, "&"))
        {
            argc++;
        }
        token = strtok(NULL, s);
    }

    strcpy(str, command); // reset str

    parsed_command->argv = (char **)malloc((2 + argc) * sizeof(char *)); // this is always the culprit of all memory issues
    argc = 0;

    // now get the arguments
    token = strtok(str, s);
    while (token != NULL)
    {
        if (strcmp(token, "&"))
        {
            parsed_command->argv[argc] = (char *)malloc((strlen(token) + 1) * sizeof(char));
            strcpy(parsed_command->argv[argc], token);
            strcat(parsed_command->argv[argc], "\0");
            argc++;
        }
        else
        {
            foreground = 0;
        }
        token = strtok(NULL, s);
    }

    parsed_command->command_name = (char *)malloc((strlen(parsed_command->argv[0]) + 1) * sizeof(char));
    strcpy(parsed_command->command_name, parsed_command->argv[0]);

    *parsed_command->foreground = foreground;
    *parsed_command->argc = argc;

    parsed_command->argv[argc] = NULL;

    return parsed_command;
}

char *read_command()
{
    /// @brief reads a unspecified length command from stdin
    /// @return returns a string of the read command

    // partially inspired / sourced from https://stackoverflow.com/a/16871702

    char *command;
    char c;
    size_t size = 64; // most commands will be less than 64 chars
    size_t len = 0;   // current len of command

    command = (char *)malloc(size * sizeof(*command));
    if (command == NULL)
    {
        perror("Malloc");
    }
    while (EOF != (c = getc(stdin)) && c != '\n')
    { // while the read char is not \n or EOF

        command[len++] = c;
        if (len == size)
        {
            command = realloc(command, sizeof(*command) * (size += 16)); // realloc 16 bytes bigger
            if (command == NULL)
            {
                perror("Realloc");
            }
        }
    }

    if (c == EOF)
    {
        printf("\nBye!\n");
        exit(0); // ctrl D or EOF from a file read into the shell should exit it
    }

    command[len++] = '\0';                              // append null term
    command = realloc(command, sizeof(*command) * len); // trim to size

    return command;
}

int builtin(struct Parsed_command *parsed_command)
{
    /// @brief checks if the command is a builtin, and implements those builtins
    /// @param parsed_command a Parsed_command struct
    /// @return returns 1 if it is a builtin, otherwise 0

    if (!strcmp(parsed_command->command_name, "cd"))
    {
        if (parsed_command->argv[1] != NULL && chdir(parsed_command->argv[1]))
        {
            // support for ~ cause I can
            if (strcmp(parsed_command->argv[1], "~"))
            {
                fprintf(stderr, "Directory %s could not be accessed. ", parsed_command->argv[1]);
                perror("");
            }
            else
            {
                char *home_dir = getenv("HOME");
                chdir(home_dir); // this should never fail
            }
        }

        return 1;
    }
    if (!strcmp(parsed_command->command_name, "pwd"))
    {
        char buffer[4097] = {0};
        getcwd(buffer, 4097); // max nix path + null space
        printf("%s\n", buffer);
        return 1;
    }
    if (!strcmp(parsed_command->command_name, "exit"))
    {
        printf("\nBye!\n");
        exit(0);
    }
    return 0;
}

int exists(char *path_to_command)
{
    /// @brief checks if there is an executable at the given path
    /// @param path_to_command the path to the file
    /// @return 1 if there is, otherwise 0

    return !access(path_to_command, X_OK);
}

void execute(char *path_to_command, struct Parsed_command *parsed_command)
{
    /// @brief executes the given command
    /// @param path_to_command real path to command to execute
    /// @param parsed_command  Parsed command object

    // this code is incredably weird
    pid_t pid, wpid;
    int status;

    if ((pid = fork()) == 0)
    {
        if (!*parsed_command->foreground)
        {
            printf("\n Child PID: %d \n", pid);
            setpgid(0, 0);
        }
        int status_code = execv(path_to_command, parsed_command->argv); // execvp takes over child
        if (status_code == -1)
        {

            fprintf(stderr, "code %d ", status_code);
            perror("");
        }
        exit(-1);
    }
    else if (pid < 0)
    {
        perror("Forking error");
    }
    else
    {
        if (*parsed_command->foreground)
        {
            waitpid(pid, &status, 0); // Parent process waits here for child to terminate.
        }
        return;
    }
}

void loop(char **path, int *path_length)
{
    /// @brief Main running loop for the shell
    /// @param path char ** containing all the paths in path
    /// @param path_length pointer to an int containing the length of the char ** with the path info

    char *path_to_command;
    char *command;
    struct Parsed_command *parsed_command;
    char buffer[64] = {0};

    while (1)
    {
        path_to_command = NULL;
        command = NULL;
        parsed_command = NULL;

        getcwd(buffer, 64);
        printf("%s $ ", buffer); // change the prompt here

        command = read_command(); // get full command string eg. ls -a -l -la --whatever

        if (strcmp(command, ""))
        {

            parsed_command = parse_command(command); // parse into parts

            if (!builtin(parsed_command))
            {
                if (!exists(parsed_command->command_name))
                {
                    path_to_command = find(path, path_length, parsed_command->command_name);
                    if (NULL == path_to_command)
                    {
                        printf("Command %s not found!\n", parsed_command->command_name);
                        // local cleanup
                        free(path_to_command);
                    }
                    else
                    {
                        if (exists(path_to_command))
                        {
                            execute(path_to_command, parsed_command);
                        }
                    }
                    free(path_to_command);
                }
                else
                {
                    execute(parsed_command->command_name, parsed_command);
                }
            }

            // memory leaks are bad, cleanup
            free(parsed_command->command_name);
            for (int i = 0; i <= *parsed_command->argc; i++)
            {
                free(parsed_command->argv[i]);
            }
            free(parsed_command->argc);
            free(parsed_command->foreground);
            free(parsed_command);
            free(command);
        }
    }
}

int main()
{

    int *path_length = (int *)malloc(sizeof(int));

    char **path = import_path(path_length);

    loop(path, path_length);

    // cleanup
    for (int i = 0; i < *path_length; i++)
    {
        free(path[i]);
    }
    free(path);
    free(path_length);

    return 0;
}