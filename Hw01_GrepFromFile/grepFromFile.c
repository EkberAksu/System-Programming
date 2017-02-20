/*##############################################################*/
/*SYSTEM PROGRAMMING HW01										*/
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

#define tempLineMaxSize 100
static int numOfOccurence=0;

/********************FUNCTION PROTOTYPES*************************************/
int searchDir( char* Dir, char* wordToSearch );
int searchTheWord(char* fileName,char* wordToSearch);
int isDirectory(char* path);

/********************************MAIN*************************************/
int main (int argc, char *argv[]) {
	int totalOccurence=0;
	if(argc!=3){
		printf("Parameters given to the program are not oppropriate!, program exits.");
		exit(1);
	}
	totalOccurence=searchDir(argv[1],argv[2]);
	printf("The word \"%s\" is found %d times in total.\n",argv[2],totalOccurence);
  
  return 0;
}
/*
*search the given word from the directory.
*@param Dir Directory path
*@param wordToSearch word to search
*return the total count
*/
int searchDir( char* Dir, char* wordToSearch ) {
	DIR  *d;
  	struct dirent *dir;
	int numOfOccurence=0;
	int num=0;
	int words=0;
	char tempPath[PATH_MAX]; /*tempdirectory*/
	pid_t child_pid;
	int tempLogFileName;

  	d = opendir(Dir);
  	if( d == NULL ) {
		
    	return 1;
  	}
  	while( ( dir = readdir( d ) ) ) {
    		if( strcmp( dir->d_name, "." ) == 0 || 
        		strcmp( dir->d_name, ".." ) == 0 ) {
      			continue;
    		}
			/*check is it a directory*/
			snprintf(tempPath, PATH_MAX,"%s/%s", Dir, dir->d_name);
    		if( isDirectory(tempPath)) {
				numOfOccurence+=searchDir( tempPath, wordToSearch );
    		} else {
      			child_pid = fork(); /*Fork */
			if(child_pid == -1)
			{
				perror("Failed to fork\n");
				closedir(d);
				return 0;
			}
			else if(child_pid >= 0)
			{
				if(child_pid == 0) /*child is created*/
				{
					/*snprintf(tempPath, PATH_MAX,"%s/%s", Dir, dir->d_name); /*get tempdirectory*/
					/*closedir(d); /*directory kapandi*/
					num = searchTheWord(tempPath, wordToSearch);
					closedir( d );
					exit(num);
				}
				else /*ebeveyn*/
				{
					wait(&words);
					numOfOccurence+=WEXITSTATUS(words);
				}	
			}
      		}
    	}

  	closedir( d );
  	return numOfOccurence;
}
/*
*search the given word from the file.
*@param fieName file
*@param wordToSearch word to search
*return the total count
*/
int searchTheWord(char* fileName,char* wordToSearch){
	char* text=NULL;
	int maxLineSize=tempLineMaxSize;
	int lineLength=0;
	int numOfOccurence=0;
	int ch=0,len=0,i=0;
	size_t lenOfWordToSearch=0;
	int line=0,column=0;
	
	FILE* fileToRead;
	FILE* logFile=fopen("gfF.log","a");

	fileToRead=fopen(fileName,"r");
	fprintf(logFile,"%s\n",fileName);
	fprintf(logFile,"%-10s %-10s %-10s\n","No","line","column");
	lenOfWordToSearch=strlen(wordToSearch); 
	while(ch!=EOF){
		text=(char*)calloc(1,sizeof(char)*tempLineMaxSize);
		ch = getc(fileToRead);
		while ( (ch != '\n') && (ch != EOF) ) {
    			if(lineLength ==maxLineSize) { /* time to expand ?*/
      				maxLineSize=tempLineMaxSize* 2; /* expand to double the current size of anything similar.*/
      				text = (char*)realloc(text, maxLineSize); /* reallocate memory.*/
    			}
    			 /* read from stream.*/
			if((ch != '\n') && (ch != EOF))
    			text[lineLength] = ch; /* stuff in buffer.*/
    			++lineLength;
			ch = getc(fileToRead);
		}
		len=strlen(text);
		for(column=0;column<len;++column){
			if(i=strncmp(&text[column] ,wordToSearch ,lenOfWordToSearch)==0){
				++numOfOccurence;
				fprintf(logFile,"#%-9d %-10d %-10d\n",numOfOccurence,line,column);
			}
		}
		lineLength=0;
		++line;
		free(text);
	}
	fclose(fileToRead);

	fclose(logFile);
		
	
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
	



