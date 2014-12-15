#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define JOBNUM 1000

#define SEED_A 5
#define SEED_B 5
#define SEED_C 5

int main(){

	FILE *jobfile; 

	jobfile = fopen("JobList","w");
	int i;
	int total = JOBNUM;

	char line[100]="test" 
           
	fwrite(&total,sizeof(int),1,jobfile);
	for(i=0;i<total;i++){
		strcat(line,itoa(10));
		strcat(line,",");
		strcat(line,itoa(10));
		fputs(line,jobfile);
		line = "test";
	}

	fclose(jobfile);

	retrun 0;

}

