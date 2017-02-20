/*##############################################################*/
/*SYSTEM PROGRAMMING HW03										*/
/*Written by AIKEBOER_AIZEZI_131044086							*/
/*USAGE: ./grepFromDir "path" "wordToSearch"					*/
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

#define BUFSIZE 256
#define tempLineMaxSize 100
#define FIFO_PERM (S_IRUSR | S_IWUSR)
static int numOfOccurence=0;

struct fd{ int fildes[2]; };
struct direc{ char dirName[PATH_MAX+1]; };

/********************FUNCTION PROTOTYPES*************************************/
int searchDir( char* Dir, char* wordToSearch ,int fildes);
int searchTheWord(char* fileName,char* wordToSearch ,int fildes);
int isDirectory(char* path);

/********************************MAIN*************************************/
int main (int argc, char *argv[]) {
	int totalOccurence=0;
	int fd;
	FILE *logFile;

	if(argc!=3){
		printf("Parameters given to the program are not oppropriate!, program exits.");
		exit(1);
	}
	if((logFile=fopen("gfF.log","a")) == NULL){
		perror("unable to open this file");
		return 1;
	}
	fd = fileno(logFile);
	totalOccurence=searchDir(argv[1],argv[2],fd);
	printf("The word \"%s\" is found %d times in total.\n",argv[2],totalOccurence);
  
  return 0;
}
/*
*search the given word from the directory.
*@param Dir Directory path
*@param wordToSearch word to search
*return the total count
*/
int searchDir( char* Dir, char* wordToSearch, int fildes ) {
	DIR  *d;
  	struct dirent *dir;
	int numOfOccurence=0;
	char tempPath[PATH_MAX]; /*tempdirectory*/
	pid_t child_pid;
	int tempLogFileName;
	struct fd* pipes;
	struct direc* fifos;
	int fifoFd;
	int filNum=0, dirNum=0;
	int countPipe=0 ,countFifo=0;
	int words=0,num=0,i;
	
  	if( (d = opendir(Dir) ) == NULL ) {
		perror("failed to open directory");
    	return 1;
  	}
	/*errno=0;*/
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
	pipes =(struct fd*)malloc(sizeof(struct fd)*filNum);
	fifos ==(struct direc*)malloc(sizeof(struct direc)*filNum);

  	while( ( dir = readdir( d ) ) !=NULL) {
    	if( strcmp( dir->d_name, "." ) == 0 || 
        	strcmp( dir->d_name, ".." ) == 0 ) {
      		continue;
    	}
		/*check is it a directory*/
		snprintf(tempPath,PATH_MAX,"%s/%s", Dir, dir->d_name);
    	if( isDirectory(tempPath)) {

			/*memset(tempPath,'\0',sizeof(Dir)+sizeof(dir->d_name));*/
      		/*strcpy(fifos[countFifo].dirName,tempPath);
    		if(mkfifo(fifos[countFifo].dirName,FIFO_PERM) == -1){  
    			if(errno != EEXIST){
    				fprintf(stderr, "%ld :failed to create fifo %s: %s\n",
    				(long)getpid(), fifos[countFifo].dirName,strerror(errno));
    				closedir(d);
    				exit(1);
    			}
    		}
    		if((child_pid= fork()) == -1){  
				perror("Failed to fork");
				closedir(d);
				exit(1);
			}
			if(child_pid == 0){	  
				while (((fifoFd= open(fifos[countFifo].dirName, O_WRONLY)) == -1) && (errno == EINTR)) ;
				if (fifoFd == -1) {
		      		fprintf(stderr, "[%ld]:failed to open named pipe %s for read: %s\n",
		             		(long)getpid(), fifos[countFifo].dirName, strerror(errno));    
		      		return 1; 
		  		}
				num =searchDir( tempPath, wordToSearch ,fifoFd);
				close(fifoFd);
				exit(num);
			}*/
			++countFifo;
    	}else { 	/*pipe and Fork */
			if((pipe(pipes[countPipe].fildes))==-1 || (child_pid= fork() ) == -1)
			{
				perror("Failed to setuo pipeline");
				closedir(d);
				exit(1);
			}
			if(child_pid == 0){ 	/*child is created*/
				close(pipes[countPipe].fildes[0]);
				num =searchTheWord(tempPath ,wordToSearch ,pipes[countPipe].fildes[1]);
				exit(num);
			}
			++countPipe;
    	}
    } 
    /*parent*/
    /*Need to wait for all*/
  	for(i=0; i<(countPipe); i++){
		wait(&words);
		numOfOccurence+=WEXITSTATUS(words);
	}
	for (i = 0; i <= countPipe; ++i)
	{	
		close(pipes[i].fildes[1]);

		copyfile(pipes[i].fildes[0],fildes);
	}
	/*for (i = 0; i <= countFifo; ++i)
	{
		while (((fifoFd = open(fifos[i].dirName, O_RDONLY)) == -1) && (errno == EINTR))  ; 
   		if (fifoFd == -1) {
      		fprintf(stderr, "[%ld]:failed to open named pipe %s for read: %s\n",
             		(long)getpid(), fifos[countFifo].dirName, strerror(errno));    
      		return 1; 
  		}   
   		copyfile(fifoFd,fildes);
   		close(fifoFd);
   		if(unlink(fifos[i].dirName)== -1){
   			perror("Failed to remove file");
   			return -1;
   		}   
	}*/
	free(pipes);
	free(fifos);
  	closedir( d );
  	return numOfOccurence;
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
	char ch;
	int rval;
	size_t lenOfWordToSearch;
	int line=0,column=0;
	char buf[PATH_MAX];
	ssize_t strsize;
	FILE* fileToRead;

	if((fileToRead=fopen(fileName,"r")) == NULL){
		perror("unable to open this file");
		return 0;
	}
	/*fprintf(logFile,"%s\n",fileName);
	fprintf(logFile,"%-10s %-10s %-10s\n","No","line","column");*/
	lenOfWordToSearch=strlen(wordToSearch);
	text=(char*)malloc(sizeof(char)*tempLineMaxSize);
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
		rval=snprintf(buf,PATH_MAX,"%s\n",fileName);
		if(rval<0){
			fprintf(stderr,"[%ld]: failded to make the string:\n",(long)getpid());
			return 1;
		}
		strsize=strlen(buf)+1;
		rval=r_write(fildes, buf, strsize);
		if(rval != strsize){
			fprintf(stderr,"[%ld]: failed to write to pipe:%s\n",(long)getpid(),strerror(errno));
			return 1;
		}
		for(column=0;column<strlen(text);++column){
			if(strncmp(&text[column] ,wordToSearch ,lenOfWordToSearch)==0){
				++numOfOccurence;
				rval=snprintf(buf,10,"#%d\t%d\t%d\n",numOfOccurence,line,column);
				if(rval<0){
					fprintf(stderr,"[%ld]: failded to make the string:\n",(long)getpid());
					return 1;
				}
				strsize=strlen(buf)+1;
				rval=r_write(fildes, buf, strsize);
				if(rval != strsize){
					fprintf(stderr,"[%ld]: failed to write to pipe:%s\n",(long)getpid(),strerror(errno));
					return 1;
				}
				/*fprintf(logFile,"#%-9d %-10d %-10d\n",numOfOccurence,line,column);*/
			}
		}
		memset(buf,'\0',0);
		r_write(fildes, buf, 2);
		lineLength=0;
		++line;
	}
	fclose(fileToRead);
	/*fclose(logFile);*/
		
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
	



