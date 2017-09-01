#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */

 int bsh_cd(char **args);
 int bsh_help(char **args);
 int bsh_exit(char **args);

 /*
   list of Builtins
*/
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char**) = {
    &bsh_cd,
    &bsh_help,
    &bsh_exit
};

int bsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  builtin function implementations
*/
int bsh_cd(char **args) 
{
    if (args[1] == NULL) {
        fprintf(stderr, "expected argument to cd\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("bsh");
        }
    }
    return 1;
}
int bsh_help(char **args)
{
  int i;
  printf("a homemade shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < bsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int bsh_exit(char **args)
{
  return 0;
}

// 
#define LSH_RL_BUFSIZE 1024
char *bsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if(!buffer){
        fprintf(stderr, "bsh: allocation error.n\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        // read a character
        c = getchar();

        // if we hit eof, replace with null char and return
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // if we have exceeded the buffer, reallocate
        if (position >= bufsize) {
            // add another 1024 to the buffer
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "bsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

// new stdio.h includes a getline function. Alternate implementation
char *bsh_read_line2(void)
{
    char *line = NULL;
    ssize_t bufsize = 0; // have getline allocate a buffer for us
    getline(&line, &bufsize, stdin);
    return line;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **bsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char));
    char *token;

    if (!tokens){
        fprintf(stderr, "bsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char));
            if (!tokens){
                fprintf(stderr, "bsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int bsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
            perror("LSH");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // error forking
        perror("LSH");
    } else {
        // parent process
        do { 
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int bsh_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < bsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return bsh_launch(args);
}

void bsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = bsh_read_line();
        args = bsh_split_line(line);
        status = bsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv)
{
    // load config files

    // run command loop
    bsh_loop();

    // perform shutdown+cleanup

    return EXIT_SUCCESS;
}