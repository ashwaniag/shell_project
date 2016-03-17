#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "header.h"
#define MAX_SUB_COMMANDS   5                                                     
#define MAX_ARGS           10   

//static int shell;
struct SubCommand                                                                
{                                                                                
        char *line;                                                              
        char *argv[MAX_ARGS];                                                    
};
                                                                               
struct Command                                                                   
{                                                                                
        struct SubCommand sub_commands[MAX_SUB_COMMANDS];                        
        int num_sub_commands;
	char *stdin_redirect;
	char *stdout_redirect;
	int background;                                                    
}; 

void signalHandler_child(int p)
{
	
        pid_t pid;
        int status;
        pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
	
        if (pid > 0) { 
		                                                             
                job_t* job = get_job(pid, process_id);
		                  
                if (job == NULL)
                        return;
           	if (WIFEXITED(status)){
			
                        if (job->status == 1) {                             
                                printf("[%d]  finished", job->pid); 
                     		jobhead = del_job(job);              
                        }
                } else if (WIFSIGNALED(status)) {                                  
                        printf("\n[%d]  killed\n", job->pid); 
                    	jobhead = del_job(job);       
                     
                } else if (WIFSTOPPED(status)) {                                  
                        if (job->status == 1) {                           
                                tcsetpgrp(shell, shell_pgid);
                                change_status(pid, 2);                    
       				printf("\n[%d]   suspended",job->pid);
               		}else { 
                                tcsetpgrp(shell, job->pgid);
                                change_status(pid, 2);                         
                                printf("\n[%d]  stopped\n", job->pid);
				 
                        }
                        return;
                } else {
                        if (job->status == 1)                        
                                jobhead = del_job(job);
				
                        
                }
		
                if (tcsetpgrp(shell, shell_pgid) == -1)     
                        tcgetattr(shell, &shell_tmodes);
        }//pid>0
		
}

void init()
{
        shell_pid = getpid();                                                             
        shell = STDIN_FILENO;                                       
        shell_valid = isatty(shell);            

        if (shell_valid) {                                                
                while (tcgetpgrp(shell) != (shell_pgid = getpgrp()))
                   kill(shell_pid, SIGTTIN);        
               
                signal(SIGQUIT, SIG_IGN);
                signal(SIGTTOU, SIG_IGN);
                signal(SIGTTIN, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
                signal(SIGINT, SIG_IGN);
                signal(SIGCHLD, &signalHandler_child);

                setpgid(shell_pid,shell_pid);                                        
                shell_pgid = getpgrp();
               
                if (tcsetpgrp(shell, shell_pgid) == -1)     
                        tcgetattr(shell, &shell_tmodes);             

                
        }
} 

void redirect_out(struct Command *command){
	
	int i=command->num_sub_commands;
	int ret = fork();
	if (ret == 0){

		usleep(20000);
                setpgrp();
		if(command->background==1){

			signal(SIGINT, SIG_DFL);
                	signal(SIGQUIT, SIG_DFL);
                	signal(SIGTSTP, SIG_DFL);
               		signal(SIGCHLD, &signalHandler_child);
			signal(SIGTTIN, SIG_DFL);
			++numjobs;
			printf("[%d]\n", getpid());
			
		}
				
		close(1);
		int fd = open(command->stdout_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0600); 
		if (fd < 0) 
		{ 
			fprintf(stderr, "%s: Cannot create file\n",command->stdout_redirect);
			exit(0); 
			} 
				
				 
		if(execvp(*command->sub_commands[i-1].argv, command->sub_commands[i-1].argv)==-1)
			printf("invalid command\n"); 
	}else{
		setpgid(ret,ret);                                      
                jobhead = insert_job(ret, ret, command->background); 
		job_t* job = get_job(ret, process_id);	
				
		if(command->background==0){
			
			putinfg(job,0); 
         		command->num_sub_commands=0;

		}else if(command->background==1){
				
                   		
				putinbg(job,0); 
				command->num_sub_commands=0;
        			}	
		}
}

void redirect_in(struct Command *command){
	
	int i=command->num_sub_commands;
	int ret = fork();
	if (ret == 0){
		
		usleep(20000);
                setpgrp();
		if(command->background==1){
			signal(SIGINT, SIG_DFL);
                	signal(SIGQUIT, SIG_DFL);
                	signal(SIGTSTP, SIG_DFL);
               		signal(SIGCHLD, &signalHandler_child);
			signal(SIGTTIN, SIG_DFL);
			++numjobs;
			printf("[%d]\n", getpid());
			
		}
				
		close(0);
		int fd = open(command->stdin_redirect, O_RDONLY); 
		if (fd < 0) 
		{ 
			fprintf(stderr, "%s: File not found\n",command->stdin_redirect); 
			exit(0);
			} 
				
		execvp(*command->sub_commands[i-1].argv, command->sub_commands[i-1].argv);
			printf("invalid command\n");
		
				
	}
	else{
		
		
		setpgid(ret,ret);                                      
                jobhead = insert_job(ret, ret, command->background); 
		job_t* job = get_job(ret, process_id);	
				
		if(command->background==0){
			
			putinfg(job,0); 
         		command->num_sub_commands=0;

		}else if(command->background==1){
				
                   		
				putinbg(job,0); 
				command->num_sub_commands=0;
        			}	
	}
}

void changedirectory(struct Command *command){
	if (command->sub_commands[0].argv[1] == NULL) {
                chdir(getenv("HOME"));                         
        } else if (chdir(command->sub_commands[0].argv[1]) == -1) {                           
                     printf(" %s: no such directory\n", command->sub_commands[0].argv[1]);
                }
        
}


void pipes(struct Command *command) 
{
  int fds[2];
  int ret;
  int fd_in = 0;
  int i=0;		
  while (command->sub_commands[i].line != NULL)
    {
	
      pipe(fds);
      ret = fork();
       
      if (ret == 0){
        
		usleep(20000);
                setpgrp();
		if(command->background==1 && i==(command->num_sub_commands-1)){
			signal(SIGINT, SIG_DFL);
                	signal(SIGQUIT, SIG_DFL);
                	signal(SIGTSTP, SIG_DFL);
               		signal(SIGCHLD, &signalHandler_child);
			signal(SIGTTIN, SIG_DFL);
			++numjobs;
			printf("[%d]\n", getpid());
			
		}
	  close(0);
	  dup(fd_in);
          if (command->sub_commands[i+1].line != NULL)
	    dup2(fds[1], 1);
          close(fds[0]);
	             
                	
         if(execvp(*command->sub_commands[i].argv, command->sub_commands[i].argv)==-1)
		printf("Invalid Command");
       
        }
      else
        {		
		setpgid(ret,ret);                                      
                jobhead = insert_job(ret, ret, command->background); 
		job_t* job = get_job(ret, process_id);	
				
		if(command->background==0 || i<command->num_sub_commands-1){
			
			putinfg(job,0); 
         		close(fds[1]);
          		fd_in = fds[0]; 
          		i++;

		}else if(command->background==1 && i==(command->num_sub_commands-1)){
				
                   		putinbg(job,0); 
				close(fds[1]);
          			fd_in = fds[0]; 
          			i++;
        			}
        }
    }//while
}


void builtin(struct Command *command){

	if(command->num_sub_commands==1){
		if(strcmp(command->sub_commands[0].argv[0],"cd")==0)
			changedirectory(command);
	
		else if (command->sub_commands[0].argv[0]==NULL)
			return;
		else if (strcmp(command->sub_commands[0].argv[0],"jobs")==0)
			print_jobs();
		else if (strcmp(command->sub_commands[0].argv[0],"kill")==0){
			
			if(command->sub_commands[0].argv[1]==NULL)
				return;
			kill_job(atoi(command->sub_commands[0].argv[1]));
		}
		else if (strcmp(command->sub_commands[0].argv[0],"fg")==0){
			
			job_t* job = get_job(3, process_status);
			
			if (job == NULL){
                        	job = get_job(2, process_status);
				if (job == NULL)
					return;}
			if (job->status == 2 || job->status == 3)
                        	putinfg(job, 1);
                	else 
                        	putinfg(job, 0);
			return;
		}
		else if (strcmp(command->sub_commands[0].argv[0],"bg")==0){

			if(command->sub_commands[0].argv[1]==NULL)
				return;

			job_t* job = get_job(atoi(command->sub_commands[0].argv[1]), process_id);
			
			if (job == NULL)
                        	return;

			if (job->status == 2 || job->status == 3)
                        	putinbg(job, 1);
				
                	else 
                        	putinbg(job, 0);
			return;
		


		}else{
		
		int ret = fork();
		if (ret == 0){
			 
			
                	usleep(20000);
                	setpgrp();  
                	if (command->background==0)
                                tcsetpgrp(shell, getpid());             
                	if (command->background==1){
				signal(SIGINT, SIG_DFL);
                		signal(SIGQUIT, SIG_DFL);
                		signal(SIGTSTP, SIG_DFL);
               			signal(SIGCHLD, &signalHandler_child);
				signal(SIGTTIN, SIG_DFL);
				++numjobs;
                        	printf("[%d]\n", (int) getpid());  }            

			
			if(execvp(*command->sub_commands[0].argv, command->sub_commands[0].argv)==-1)
				printf("command not found\n"); 
			
		}else if(ret>0){
			
				
		
			setpgid(ret,ret);                                      
                	jobhead = insert_job(ret, ret, command->background); 

                	job_t* job = get_job(ret, process_id);                             

                	if (command->background==0) 
	
                        	putinfg(job,0);

               		if (command->background==1)
				
				putinbg(job,0); 
				  
	     		}
	
		}//parent
	}else if(command->num_sub_commands>1)
		pipes(command);

}



void ReadCommand(char *line, struct Command *command){

char *token;
char *token1;
char *dup;
char *dup1;
token=strtok(line,"|");
int i=0;
int j=0;

while(token!=NULL){
		
		if (i<MAX_SUB_COMMANDS){
		command->sub_commands[i].line=token;
		i++;
		token=strtok(NULL,"|");
		}else {printf("Commands more than 5\n");
			command->num_sub_commands=i;
			exit(0);
		}
	}	
	command->sub_commands[i].line=NULL;
	command->num_sub_commands=i;
	for(i=0;i<command->num_sub_commands;i++){
		dup=strdup(command->sub_commands[i].line);
		token1=strtok(dup," ");
		j=0;
		while(token1!=NULL){

			command->sub_commands[i].argv[j]=token1;
			j++;
			token1=strtok(NULL," ");
		}
		command->sub_commands[i].argv[j]=NULL;
	}
	
	command->stdout_redirect=NULL;
	command->stdin_redirect=NULL;
	command->background=0;
	int k;
	for(k=j-1;k>0;k--){
		
		switch(*command->sub_commands[i-1].argv[k]){
		
		case '&':
			
			command->background=1;
			command->sub_commands[i-1].argv[k]=NULL;
			
			break;
		
		case'<':
			
			command->stdin_redirect=command->sub_commands[i-1].argv[k+1];
			command->sub_commands[i-1].argv[k+1]=NULL;
			command->sub_commands[i-1].argv[k]=NULL;
			
			redirect_in(command);
			break;
		
		case'>':
			
			command->stdout_redirect=command->sub_commands[i-1].argv[k+1];
			command->sub_commands[i-1].argv[k+1]=NULL;
			command->sub_commands[i-1].argv[k]=NULL;
			redirect_out(command);
			
			break;
			
			}
		}
		builtin(command);
		
	
}

int main(){

	char *input=NULL;
	
	init();
	struct Command command;
	
	while(1){
		
		input=NULL;
		input=readline("$ ");
		
		if (input!=NULL){
			
            		add_history(input);
			
		}else
			break;
		if(strcmp(input, "exit") == 0)
			exit(0);
		
		ReadCommand(input, &command);
		
		}
	
	
	return 0;
}
