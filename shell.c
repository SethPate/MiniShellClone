// A simple shell, "Mini-Shell".
// Has built-in commands: date, cd, help, exit.
// signal handler receives ctrl+C exit command.
// runs until exit() or signal received.

#include <stdio.h> // printing and receiving input, stderr
#include <stdlib.h> // malloc
#include <string.h> // strtok and strcmp
#include <signal.h> // exit signal
#include <unistd.h> // fork, pid, execvp, chdir
#include <sys/wait.h> // waitpid
#include <time.h> // time_t and time()
#include <stdint.h> // uintmax_t for date_shell()

// DEFINITIONS

#define DELIMITER " \n\t\r"
#define BUFFSIZE 80

// FUNCTION DECLARATIONS

void sigint_handler(int sig);
int shell_exit(char **args);
int shell_help(char **args);
int shell_cd(char **args);
int shell_date(char **args);
void loop_shell();
char *get_line();
char **parse(char *line);
int execute_shell(char **argv);
int run_external(char **argv);
int pipe_handler(char *line);
int pipe_execute(char **left, char **right);

// MAIN FUNCTION

// boots the loop function
int main(){
  // allows signal to terminate program and children
  signal(SIGINT, sigint_handler);
  // launches the shell
  loop_shell();

  return 0;
}

// BUILT-IN SHELL COMMANDS

// array of pointers to this shell's built-in commands
// used by execute_shell to run internal commands
int (*built_in_commands[]) (char**) = {
  &shell_exit,
  &shell_cd,
  &shell_help,
  &shell_date
};

// number of built-in functions in this minishell
int number_of_built_in = 4;

// an array of the names of all built-in commands
// useful for execute_shell function, to check
// before passing instruction to bash shell
char* built_in_strings[] = {
  "exit",
  "cd",
  "help",
  "date"
};

// exits the shell and terminates child processes
int shell_exit(char **args) {
  return 0; // shell_exit is the only internal command
            // that retuns 0; this tell the loop to stop
}

// changes directory within the shell
int shell_cd(char **args) {
  if (!args[1]) { // check that an argument was included
    fprintf(stderr, "cd_shell needs an argument\n");
  } else { 
    // try to execute the chdir
    if (chdir(args[1]) != 0) { // show error if it didn't work
      fprintf(stderr, "couldn't load that directory\n");
    }
  }
  return 1; // indicates successful completion 
}

// prints information about the shell and how to use it
int shell_help(char** args) {
  printf("[CS 5007] Assignment 4\n");
  printf("Mini-Shell by Seth Pate\n");
  printf("August 9th, 2018\n");
  printf("This shell includes the following commands:\n");
  printf("\tcd <arg>: changes directory to <arg>\n");
  printf("\thelp: calls this help file\n");
  printf("\tdate: returns the current date and time\n");
  printf("\texit: exits the shell and terminates child procs\n");
  printf("all other commands are executed by your shell.\n");
  return 1;
}

// prints date and time information from system
// uses greenwich mean time
int shell_date(char** args) {
  time_t current_time = time(NULL);
  if (current_time != -1) {
    // asctime() converts time_t to calendar format
    // gmtime() converts rawtime to greenwich mean time
    printf("Current Greenwich Mean Time: %s",
      asctime(gmtime(&current_time)));
    // seconds from Epoch, for fun
    printf("%ju seconds from January 1st, 1970\n",
      (uintmax_t)current_time); 
  }

  return 1;
}

// HELPER FUNCTIONS

// receives signals and terminates its larger function
void sigint_handler(int sig) {
  write(1,"mini-shell terminated\n",22);
  exit(EXIT_SUCCESS);
}

// the beating heart of the shell.
// runs and gets user input until explicitly exited
// with exit() or exit signal.
void loop_shell() {
  int fn_signal; // if 0, loop will terminate

  // these will hold user input
  char* line;
  char** args;

  do { // loop at least once
    // tell the user what shell they're in
    printf("mini-shell> ");
    // read a line from the user
    line = get_line();
    if (strstr(line, "|")) { // handle pipes separately
      pipe_handler(line);
    } else {
      // parse the line into an argv
      args = parse(line);
      // attempt to execute that loop
      // only exit_shell returns 0
      fn_signal = execute_shell(args);
    }
    // free up memory
    free(line);
    free(args);
  } while (fn_signal); // if 0, stop the loop
} 

// gets a line of input from the user
// can only handle 80 characters
char *get_line() {
  // malloc the line to return
  char* line = malloc(BUFFSIZE * sizeof(char));
  int c; // holds each character
  int i = 0; // our position in the line to return

  while (1) { // run unless returned or error
    c = getchar();
    if (c == EOF || c == '\n') {
      line[i] = '\0'; // null terminate the line
      return line; // if no pipe, treat normally
    } else {
      line[i] = c;
      i++;
    }
  }
}

// parses a line delimited by whitespace into an
// array of char*
char **parse(char *line) {
  int buffer = BUFFSIZE; // should be long enough
  // store tokens in a char**
   char **words = malloc(buffer * sizeof(char*));

  // exit with an error if malloc failed
  if (!words) {
    fprintf(stderr, "parse could not allocate\n");
    exit(EXIT_FAILURE);
  }

  int i = 0; // holds the index of the words array
 
  char* word = strtok(line, DELIMITER); // grabs first token

  while (word) { // will not run when strtok returns NULL
    // store the first token 
    words[i] = word;
    i++;
    // now get the NEXT token from the same line
    word = strtok(NULL, DELIMITER);
  }
 
  words[i] = NULL; // terminate char** 
  return words;
}

// executes commands after the parser does it work
// attempts to run a built-in command if possible
// otherwise passes command to bash shell through
// run_extenal(char **argv) function
int execute_shell(char **argv) {
  if (argv[0] == NULL) {
    return 1; // no command
  }

  for (int i = 0; i < number_of_built_in; i++) {
    if (strcmp(argv[0],built_in_strings[i]) == 0) {
      return (*built_in_commands[i])(argv);
    }
  }

  return run_external(argv);
}

// splits a line that is known to contain "|"
// and sends the left and right arguments to
// the execute_pipe function 

int pipe_handler(char *line) {
  int i = 0;
  char *components[BUFFSIZE];
 
  char *component = strtok(line, "|");

  while (component) {
    components[i] = component;
    i++;
    component = strtok(NULL, "|");
  }

  components[i] = NULL;

  char **left = parse(components[0]);
  char **right = parse(components[1]);

  pipe_execute(left,right);

  return 0;
}

// the pipe command transfers the output of one function
// into the input of another
// eg ls -l | wc
// will execute ls -l, and pipe its output to wc

int pipe_execute(char **left, char **right) {
  int pipefd[2]; // 0 for read FD, 1 for write FD
  
  if (pipe(pipefd) == -1) { // create the pipe
    fprintf(stderr, "error creating pipe\n");
    exit(EXIT_FAILURE);
  }  

  pid_t pid1, pid2;

  pid1 = fork();

  if (pid1 == 0) { // run the first child
    dup2(pipefd[1],STDOUT_FILENO); // connect write to output
    close(pipefd[0]); // don't need read end 
    close(pipefd[1]); // no longer need write end
    execvp(left[0], left); // execute left command
    // shouldn't get here
    fprintf(stderr, "error in left fork\n");
    exit(EXIT_FAILURE);
  } else if (pid1 < 0) {
    fprintf(stderr,"error forking\n");
    exit(EXIT_FAILURE);
  } else {
    waitpid(pid1,NULL,0);
    pid2 = fork();
    // wait for first child to finish 
    if (pid2 == 0) { // run the second child 
      dup2(pipefd[0],STDIN_FILENO); // connect read to input
      close(pipefd[1]); // no longer need read one
      close(pipefd[0]); // don't need write end
      execvp(right[0], right);
      // shouldn't get here
      fprintf(stderr, "error in right fork\n");
      exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
      fprintf(stderr,"error forking\n");
      exit(EXIT_FAILURE);
    }
  }
  close(pipefd[0]); // no longer need read one
  close(pipefd[1]); // don't need write end
  waitpid(pid2,NULL,0); // close up the patient

  return 0;
}

// uses bash shell and execvp to run command in new pid
// waits for child process to finish before returning
int run_external(char** argv) {
  pid_t pid;

  pid = fork();
  
  if (pid == 0) { // child process
    if (execvp(argv[0],argv) == -1) {
      	fprintf(stderr, "Command not found -- did you mean something else?\n");
    }
    exit(EXIT_FAILURE); // child: indicate failure to call execvp
  } else if (pid < 0) {
      fprintf(stderr, "fork failed\n");
      return EXIT_FAILURE;
  } else {
    wait(NULL); // parent: wait for child to finish
  }

  return 1; // parent: show that you are finished
}
