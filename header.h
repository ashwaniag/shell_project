#include <stdio.h>
#include <stdlib.h>                                                                           
#include <unistd.h>                                                                          
#include <string.h>                                                                           
#include <sys/types.h>                                                                     
#include <signal.h>                                                                           
#include <sys/wait.h>                                                                       
#include <fcntl.h>                                                                              
#include <termios.h> 
#include <wait.h>

#define process_id 1
#define process_status 2

typedef struct Job { 									
        int id;
       
        int pid;
        int pgid; 
        int status;
     
        struct Job *next;
} job_t;

static job_t* jobhead=NULL;
static int shell_pid;
static int shell_pgid;
static int shell, shell_valid;
static struct termios shell_tmodes;

static int numjobs=0;

job_t* insert_job(pid_t pid, pid_t pgid,
                 int status)
{
        usleep(10000);
        job_t* newjob = malloc(sizeof(job_t));
 
       
        newjob->pid = pid;
        newjob->pgid = pgid;
        newjob->status = status;
        
        newjob->next = NULL;

        if (jobhead == NULL) {
                numjobs++;
                newjob->id = numjobs;
                return newjob;
        } else {
                job_t* temp = jobhead;
                while (temp->next != NULL) {
                        temp = temp->next;
                }
                newjob->id = temp->id + 1;
                temp->next = newjob;
                numjobs++;
                return jobhead;
        }
}


int change_status(int pid, int status)
{
        usleep(10000);
        job_t* job = jobhead;
        if (job == NULL) {
                return 0;
        } else {
                int counter = 0;
                while (job != NULL) {
                        if (job->pid == pid) {
                                job->status = status;
                                return 1;
                        }
                        counter++;
                        job = job->next;
                }
                return 0;
        }
}


job_t* del_job(job_t* job1)
{
        usleep(10000);
        if (jobhead == NULL)
                return NULL;
        job_t* currentJob;
        job_t* beforeCurrentJob;

        currentJob = jobhead->next;
        beforeCurrentJob = jobhead;

        if (beforeCurrentJob->pid == job1->pid) {

                beforeCurrentJob = beforeCurrentJob->next;
                numjobs--;
                return currentJob;
        }

        while (currentJob != NULL) {
                if (currentJob->pid == job1->pid) {
                        numjobs--;
                        beforeCurrentJob->next = currentJob->next;
                }
                beforeCurrentJob = currentJob;
                currentJob = currentJob->next;
        }
        return jobhead;
}


job_t* get_job(int x, int y)
{
        usleep(10000);
        job_t* job = jobhead;
        switch (y) {
        case process_id:
                while (job != NULL) {
                        if (job->pid == x)
                                return job;
                        else
                                job = job->next;
                }
                break;
        
               
        case process_status:
                while (job != NULL) {
                        if (job->status == x)
                                return job;
                        else
                                job = job->next;
                }
                break;
        default:
                return NULL;
                break;
        }
        return NULL;
}


void print_jobs()
{
        
        job_t* job = jobhead;
        if (job == NULL) {
                printf( "No Active Jobs.\n");
        } else {
                while (job != NULL) {
                        printf(" %d.  %5d \n", job->id,
                               job->pid);
                        job = job->next;
                }
        }
        
}
void wait_job(job_t* job)
{
        int status;
        while (waitpid(job->pid, &status, WNOHANG) == 0) {      
                if (job->status == 2)                              
                        return;
        }
        jobhead = del_job(job);                                            
}

void kill_job(int jobpid)
{
        job_t* job = get_job(jobpid, process_id);                                   
        kill(job->pid, SIGKILL);                                                              
}

void putinfg(job_t* job, int c)
{
        job->status = 0;                                                   
        tcsetpgrp(shell, job->pgid);                                 
        if (c==1) {                                                                      
                if (kill(-job->pgid, SIGCONT) < 0)                                           
                        perror("kill (SIGCONT)");
        }

  	wait_job(job);                                    
        tcsetpgrp(shell, shell_pgid);                              
}


void putinbg(job_t* job, int c)
{
        if (job == NULL)
                return;

        if (c==1 && job->status != 2)
                job->status = 2;		
		

        if (c==1)                        
                if (kill(-job->pgid, SIGCONT) < 0)
                        perror("kill (SIGCONT)");
	
	if (tcsetpgrp(shell, shell_pgid) == -1)     
                        tcgetattr(shell, &shell_tmodes); 
	                         
}

