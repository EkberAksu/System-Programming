/*##############################################################*/
/*SYSTEM PROGRAMMING HW03										*/
/*Written by AIKEBOER_AIZEZI_131044086							*/
/*USAGE: ./grepFromDir /path "wordToSearch"		      			*/
/*##############################################################*/
#include <dirent.h> 
#include <sys/types.h> 
#include <sys/param.h> 
#include <sys/stat.h>
#include <errno.h> 
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/wait.h>
#include "restart.h"
#include <pthread.h>
#include <semaphore.h>

#define BUFSIZE 		256
#define MAX_FILE_NO     256
#define Max_DIR_NO      256
#define tempLineMaxSize 100  /*temprory maximum line size,which will be maximized if neccery*/
#define mainLogFile     "gFDir.log"
#define SEMAPHOURE      "/grepFromDirghjk"
#define FIFONAME        "mainFifo"

#define FIFO_PERM (S_IRUSR | S_IWUSR)
#define PERMS (mode_t)(S_IRUSR | S_IWUSR)
#define FLAGS (O_CREAT | O_EXCL)

typedef struct direc{ char dirName[PATH_MAX]; }direc;

typedef struct parameters{
	char* word;
	char  path[PATH_MAX];
	int   fildes[2];
} parameters;

pthread_t t_id[MAX_FILE_NO];
pid_t    pids[Max_DIR_NO];
int  	countFifo=0;
int  PipeCount=0 ;
direc* 	fifos;
FILE* 	logFile;
sem_t *semlockp;
/********************FUNCTION PROTOTYPES*************************************/
int searchDir( char* Dir, char* wordToSearch ,int fifoFd, int* numOfOccurence);
int searchTheWord(char* fileName,char* wordToSearch ,int fildes);
int getnamed(char *name, sem_t **sem, int val);
int destroynamed(char *name, sem_t *sem);
int countTotalOccurence(char* file);
void* process(void* stParam);
int isDirectory(char* path);

void sighandeler(int signo);
void signal_callback_handler(int signum);

/********************************MAIN*************************************/
int main (int argc, char *argv[]) {
	int numOfOccurence=0;
	int numRead=0;
	int existCount=0;
	int fd, fifoFd;
	pid_t child_pid;
	char buf[256];
	

	if(argc!=3){
		printf("Parameters given to the program are not oppropriate!, program exits.");
		exit(1);
	}
	signal(SIGINT,sighandeler);
	/* Catch Signal Handler SIGPIPE */
	signal(SIGPIPE, signal_callback_handler);

	if((logFile=fopen(mainLogFile,"a")) == NULL){
		perror("unable to open this file");
		return 1;
	}
	existCount=countTotalOccurence(mainLogFile);
	fprintf(logFile,"%-7s %-7s %-7s\n","No","line","column");
	fflush(logFile);
	
	fd = fileno(logFile);

	if(mkfifo(FIFONAME,0666) == -1){  
		if(errno != EEXIST){
			fprintf(stderr, "%ld :failed to create fifo %s: %s\n",
			(long)getpid(), fifos[countFifo].dirName,strerror(errno));
			exit(1);
		}
	}

	if((child_pid= fork()) == -1){  
		perror("Failed to fork");
		exit(1);
	}
	if(child_pid == 0){	
		while (((fifoFd= open(FIFONAME, O_WRONLY)) == -1) && (errno == EINTR)) ;
		if (fifoFd == -1) {
      		fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
             		(long)getpid(), FIFONAME, strerror(errno));   
             unlink(FIFONAME) ;
      		return 1; 
  		}
		if (getnamed(SEMAPHOURE, &semlockp, 1) == -1) {
	       perror("Failed to create named semaphore");
	       return 1;
	    }
		numOfOccurence=searchDir(argv[1],argv[2],fifoFd,&numOfOccurence);
		close(fifoFd);
		exit(numOfOccurence);
	}
	while (((fifoFd = open(FIFONAME, O_RDONLY)) == -1) && (errno == EINTR))  ; 
	if (fifoFd == -1) {
		fprintf(stderr, "main process[%ld]:failed to open named pipe %s for read: %s\n",
     		(long)getpid(), FIFONAME, strerror(errno));  
     	unlink(FIFONAME ) ;
		return 1; 
	}
	if (r_wait(NULL) == -1){
		fprintf(stderr, "Main process Failed to wait for child");
		return -1;
	}   

	/* remainder section */  
	
	while((numRead = read(fifoFd, buf, 255))){
        if (write(fd, buf, numRead)<=0 ){
        	perror("child - partial/failed write");    
        	exit(1);
        }
	}
  	close(fd);
	fclose(logFile);
	close(fifoFd);
	unlink(FIFONAME ) ;
	destroynamed(SEMAPHOURE, semlockp);
	numOfOccurence=countTotalOccurence(mainLogFile) - existCount;
	printf("The word \"%s\" is found %d times in total.\n",argv[2],numOfOccurence);

  return 0;
}
/*
*search the given word from the directory.
*@param Dir Directory path
*@param wordToSearch word to search
*return the total count
*/
int searchDir( char* Dir, char* wordToSearch, int fifoFd,int* numOfOccurence ) {
	DIR  *d;
  	struct dirent *dir;
	char tempPath[PATH_MAX]; /*tempdirectory*/
	pid_t child_pid;
	int filNum=0, dirNum=0;
	int words=0,num=0,i,numRead,error;
	char buf[256];
	char* new_str;
	parameters* regFiles;
	int *exitcode;
	int  countPipe=0 ;

	
  	if( (d = opendir(Dir) ) == NULL ) {
		perror("failed to open directory");
    	return 1;
  	}

	while((dir=readdir(d) ) != NULL){
		if( strcmp( dir->d_name, "." ) == 0 || 
        	strcmp( dir->d_name, ".." ) == 0 ) {
      		continue;
    	}
		/*check is it a directory*/
		sprintf(tempPath,"%s/%s", Dir, dir->d_name);
    	if( isDirectory(tempPath)) {
			++dirNum;
    	}else {
    		++filNum;
		}
	}
	closedir(d);
	if( (d = opendir(Dir) ) == NULL ) {
		perror("failed to open directory");
    	return 1;
  	}
	regFiles =(parameters*)calloc(filNum,sizeof(parameters));
	/*fifos =(direc*)malloc(sizeof(direc)*dirNum);*/

  	while( ( dir = readdir( d ) ) !=NULL) {
    	if( strcmp( dir->d_name, "." ) == 0 || 
        	strcmp( dir->d_name, ".." ) == 0 ||
        	dir->d_name[strlen(dir->d_name)-1]=='~') {
      		continue;
    	}
		/*check is it a directory*/
		snprintf(tempPath,PATH_MAX,"%s/%s", Dir, dir->d_name);

    	if( isDirectory(tempPath)) {

    		if((child_pid= fork()) == -1){  
				perror("Failed to fork");
				closedir(d);
				exit(1);
			}

			if(child_pid == 0){	  

				free(regFiles);
				num =searchDir( tempPath, wordToSearch ,fifoFd, numOfOccurence);
				
				exit(num);
			}
			else{
				
				pids[countFifo]=child_pid;
				++countFifo;
			}
    		
    	}else { 	/*pipe and thread */
			
			regFiles[countPipe].word =wordToSearch;
			strcpy(regFiles[countPipe].path, tempPath);
	
			if((pipe(regFiles[countPipe].fildes))==-1)
			{
				perror("Failed to setup pipeline");
				closedir(d);
				exit(1);
			}
			if(error=pthread_create(&t_id[countPipe], NULL, process,  &regFiles[countPipe]) ){
				fprintf(stderr, "falid to create thread\n" );
				return -1;
			}
			++countPipe;
    	}
    }
    
    /*parent*/
    /*join all of the threads*/
	for(i=0; i<(countPipe); i++){
		if(error=pthread_join(t_id[i], (void**)&exitcode)){
			fprintf(stderr, "falid to join thread\n" );
			return -1;
		}
	}
	/*wait for all childes*/
	for(i=0; i<(countFifo); i++){
		wait(NULL);
	}
  	/*read from all the pipes and write them down to log file*/
	for (i = 0; i < countPipe; ++i)
	{
		while (sem_wait(semlockp) == -1)                        /* entry section */ 
		    if (errno != EINTR) { 
		       perror("Failed to lock semlock");
		       return 1;
		    }
		close(regFiles[i].fildes[1]);
		/* Read data from pipe, echo on stdout */
		while(numRead = read(regFiles[i].fildes[0], buf, 256)){          
            if (write(fifoFd, buf, numRead)<=0 ){
            	perror("child - partial/failed write");   
            	exit(1);
            }
        }
        close(regFiles[i].fildes[0]);
        if (sem_post(semlockp) == -1) {                           /* exit section */
	       perror("Failed to unlock semlock");
	       return 1; 
	    }
	}
	PipeCount=countPipe;
	
	/*frees all allocated memoris*/
	free(regFiles);
	/*free(fifos);*/

	/*close the directory*/
  	closedir( d );

  	return *numOfOccurence;
}
/*processes searching operation inside the thread.
*@param stParam  a void pointer,which is actually a parameters structure*/
void* process(void* stParam){
	
	parameters* param= (parameters*) stParam;
	
	searchTheWord(param->path, param->word, param->fildes[1]);
	
	return ;
}
/*
*search the given word from the file.
*@param fieName file
*@param wordToSearch word to search
*return the total count
*/
int searchTheWord(char* fileName,char* wordToSearch ,int fildes){
	char* text;
	int maxLineSize=tempLineMaxSize;
	int lineLength=0;
	int numOfOccurence=0;
	char ch =NULL;
	int rval=0, length=0;
	size_t lenOfWordToSearch;
	int line=0,column=0;
	char buf[PATH_MAX];
	ssize_t strsize;
	FILE* fileToRead;

	if((fileToRead=fopen(fileName,"r")) == NULL){
		perror("unable to open this file");
		return 0;
	}
	rval=snprintf(buf,PATH_MAX,"\n%s\n",fileName);
	if(rval<0){
		fprintf(stderr,"[%ld]: failded to make the string:\n",(long)getpid());
		return 1;
	}
	strsize=strlen(buf);
	rval=write(fildes, buf, strsize);
	if(rval != strsize){
		fprintf(stderr,"[%ld]: failed to write to pipe:%s\n",(long)getpid(),strerror(errno));
		return 1;
	}
	
	lenOfWordToSearch=strlen(wordToSearch);
	text=(char*)calloc(tempLineMaxSize,sizeof(char));
	while(ch!=EOF){
		ch = getc(fileToRead);
		while ( (ch != '\n') && (ch != EOF) ) {
    			if(lineLength ==maxLineSize) { /* time to expand ?*/
      				maxLineSize*= 2; /* expand to double the current size of anything similar.*/
      				text = (char*)realloc(text, maxLineSize); /* reallocate memory.*/
    			}
    			 /* read from stream.*/
			if((ch != '\n') && (ch != EOF))
    			text[lineLength] = ch; /* stuff in buffer.*/
    			++lineLength;
			ch = getc(fileToRead);
		}
		length=strlen(text)-lenOfWordToSearch;
		for(column=0;column<=length;++column){
			if(strncmp(&text[column] ,wordToSearch ,lenOfWordToSearch)==0){
				++numOfOccurence;
				rval=snprintf(buf,PATH_MAX,"#%d\t\t%d\t\t%d\n",numOfOccurence,line,column);
				if(rval<0){
					fprintf(stderr,"[%ld]: failded to make the string:\n",(long)getpid());
					return 1;
				}
				strsize=strlen(buf);
				rval=write(fildes, buf, strsize);
				if(rval != strsize){
					fprintf(stderr,"[%ld]: failed to write to pipe:%s\n",(long)getpid(),strerror(errno));
					return 1;
				}
			}
		}
		
		lineLength=0;
		++line;
	}
	/*closes file*/
	fclose(fileToRead);
	/*frees allocated memory*/	
	free(text);
	return numOfOccurence;
}

/*
*check the if its directory or not
*@param path char pinter
*return 0 if not , 1 if it is a directory
*/
int isDirectory(char* path){
	struct stat statbuf;
	
	if(stat(path,&statbuf)== -1)
		return 0;
	else
		return S_ISDIR(statbuf.st_mode);
}
int getnamed(char *name, sem_t **sem, int val) {
   while (((*sem = sem_open(name, FLAGS , PERMS, val)) == SEM_FAILED) &&
           (errno == EINTR)) ;
   if (*sem != SEM_FAILED)
       return 0;
   if (errno != EEXIST)
      return -1;
   while (((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR)) ;
   if (*sem != SEM_FAILED)
       return 0;
   return -1;
}
int destroynamed(char *name, sem_t *sem) {
    int error = 0;

    if (sem_close(sem) == -1)
       error = errno;
    if ((sem_unlink(name) != -1) && !error)
       return 0;
    if (error)        /* set errno to first error that occurred */
       errno = error;
    return -1;
}

int countTotalOccurence(char* file){
	FILE* fp;
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int count=0 ;
    char* found=NULL;

	if((fp=fopen(file,"r")) == NULL){
		perror("unable to open this file");
		return 1;
	}
	while ((read = getline(&line, &len, fp)) != -1) {
        if((found=strstr(line,"."))!=NULL){
        	found=NULL;
        	continue;
        }
        	
        ++count;
    }

    fclose(fp);
    if (line)
        free(line);
    return count;
}
void sighandeler(int signo){
	int i;
	for(i=0; i<countFifo;++i){
		kill(pids[i],SIGINT);
		unlink(fifos[i].dirName);
	}
	for(i=0; i<PipeCount;++i){
		kill(t_id[i],SIGINT);
	}
	fclose(logFile);
	fprintf(stderr, "\nCTRL-C has caught, program is shutting down properly\n");
	destroynamed(SEMAPHOURE, semlockp);
	exit(signo);
}
/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
        exit(1);
}
