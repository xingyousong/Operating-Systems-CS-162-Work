#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "tokenizer.h"
/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_wait(struct tokens *tokens);
/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "show current working directory"},
  {cmd_cd, "cd", "change working directory"},
  {cmd_wait, "wait", "wait until all background jobs are done"}
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/*Prints working directory*/ 
int cmd_pwd(unused struct tokens *tokens){
  char personal_dir[100];
  getcwd(personal_dir, sizeof(personal_dir));
  fprintf(stdout, "%s\n", personal_dir);

  return 0;
}


/* Change directory */
int cmd_cd(struct tokens *tokens){
  char* temp = tokens_get_token( tokens, 1);
  chdir(temp);
  return 0;
}

int cmd_wait(struct tokens *tokens){
  int status; 
  pid_t pid; 
  while ((pid = waitpid(-1, &status, 0)) > 0 ){
    if (WIFEXITED(status)){

    }
  }
  return 0;
}


/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}


void sighandler(int signum){
    printf("Signal %d caught\n", signum);
    killpg(tcgetpgrp(0), signum); 
}


/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }

    signal(SIGINT, sighandler);
    signal(SIGTSTP, sighandler);
}



int check_executable(char* input_dir, char* exe){
  char temp_buffer[1000];
  
  strcpy(temp_buffer, input_dir);
  strcat(temp_buffer, "/");
  strcat(temp_buffer, exe);
  
  //printf("%s\n", temp_buffer);
  if (access(temp_buffer, X_OK) == 0){
    memset(temp_buffer, 0, sizeof(temp_buffer));
    return 1; //success
  }
  else{
    memset(temp_buffer, 0, sizeof(temp_buffer));
    return 0; //fail
  }
}

int index_redirect(struct tokens *tok){
  char* left = "<";
  char* right = ">";
  int length_tok = tokens_get_length(tok);
  for(int i = 0; i < length_tok; i++){
    char* temp = tokens_get_token(tok, i);
    if ( strcmp(temp, left ) == 0 || strcmp(temp,right ) == 0 ){
      return i;
    }
  }
  return -1; //did not find
}


int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);





  int directories = 0; 
  char* path = getenv("PATH");
  char* dir[100];

  char* p = strtok(path, ":");
  while(p != NULL){
    dir[directories++] = p;
    p = strtok(NULL, ":");
  }

 
  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tok = tokenize(line);
    //printf("tokenize ok\n");
    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tok, 0));



    if (fundex >= 0) {
      cmd_table[fundex].fun(tok);
    } else {
      /* REPLACE this to run commands as programs. */
      

      char* exe = tokens_get_token(tok, 0);
      
      char total_executable[1000]; 
      int shortcut = 0;

      for(int i = 0; i< directories; i++ ){
        if (check_executable(dir[i], exe) == 1) {
          strcpy(total_executable, dir[i]);
          strcat(total_executable, "/");
          strcat(total_executable, exe);
          shortcut = 1;
          //printf("exe found in directory: %s\n", total_executable); 
          break;
        }  
      }
        //printf("total executable: %s \n", total_executable);

      char* ampersand = "&";
      int background = 0; 

      int index_red = index_redirect(tok);
      size_t leng; 
      if (index_red != -1){

        leng = index_red; 
        }
      else{
        leng = tokens_get_length(tok);
      }

      if(strcmp(tokens_get_token(tok, tokens_get_length(tok)-1), ampersand) == 0){
        background = 1;
      }

      if(background == 1 && index_red == -1){
        leng = leng - 1;
      }
      
      //printf("%zd \n", leng);
      char* temp[leng+1];

      for(int i = 0; i < leng; i ++){
        temp[i] = tokens_get_token(tok, i);
      }

      if(shortcut == 1){
        temp[0] = total_executable; 
      }

      temp[leng] = NULL;

      //must check resets
      //printf("temp[0]: %s \n", temp[0]);

      pid_t pid = fork(); 
      if(pid == 0){


        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);





        if (index_red != -1){
        char* left = "<";
        char* right = ">";
        
        char* file_name = tokens_get_token(tok, index_red+1);
        
        if( strcmp(tokens_get_token(tok, index_red), right) == 0 ){
          int file_desc = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
          dup2(file_desc,1);
        }
        if (strcmp(tokens_get_token(tok, index_red), left) == 0){
          int file_desc = open(file_name, O_RDONLY);
          dup2(file_desc,0);
        }
      }



        execv(temp[0], temp);
        memset(total_executable, 0, sizeof(total_executable));
        exit(0);
      }
      else{
        setpgid(pid, pid);
        tcsetpgrp(shell_terminal, pid);
        if(background != 1){
        int child_stat;
        if(waitpid(pid, &child_stat, 0) < 0){
          fprintf(stderr, "error\n");
        }
        }
        else{
          
        }


        tcsetpgrp(shell_terminal, shell_pgid);
        tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
        //printf("exec successful \n");
      }
      
      //fprintf(stdout, "This shell doesn't know how to run programs.\n");
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tok);
  }

  return 0;
}
