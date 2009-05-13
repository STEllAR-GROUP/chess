/*
*  C Implementation: file
*
* Description: Provides all of the file services necessary to write the 
* chess moves to a log file.
*
* Author: Phillip LeBlanc (C) 2008
*
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

#define LOOP_FILE "loop"

FILE *loop;  //What file number we are on.


FILE* set_output(FILE *out, int sside)  //Defines the logfile and sets it to the correct log number i.e. chess1245.txt
{
	char lp[5];
	char logfile[15];
	int lpcnt;
	loop = fopen(LOOP_FILE, "r");
	if (!loop) {
		if (loop)
			fclose(loop);
		loop = NULL;
		printf("Loop control file missing. May overwrite existing log files.\n");
		sprintf(logfile, "chess0.%d.log", sside);
		out = fopen(logfile, "w");
		loop = fopen(LOOP_FILE, "w");
		fprintf(loop, "1");
		fclose(loop);
		return out;
	}
	else {
		fgets(lp, sizeof(lp), loop);
		sprintf(logfile, "chess%s.%d.log" , lp, sside);
		out = fopen(logfile, "w");
		sscanf(lp, "%d", &lpcnt);
		lpcnt++;
		loop = fopen(LOOP_FILE, "w");
		fprintf(loop, "%d", lpcnt);
		fclose(loop);
		return out;
	}
}

int close_output(FILE *out)
{
	return (fclose(out));
}

/******************************************************************
Random Number Storer
--------------------
This will calculate some random numbers and store them in a file.
First we test to see if there already exists some random numbers in
a file, if not then we write it. If there are, we read them in. If
we read in the last one, we delete the file.
******************************************************************/

void seed_rand()  //This may not be needed. It attempts to fix the problem of multiple iterations running in the same
{		  //second which would cause the same value to be seeded for rand()
	FILE *randfile;
	int i,n,o;
	int r[60];	//The 60 random integers we create
	char c[20];	//line containing the integer
	for (i=0;i<60;i++)
		r[i]=0;

	randfile = fopen("rand", "r");		//Try to open the file "rand"
	if  (!randfile) {			//If it doesn't exist
		srand(time(NULL)); 		//Seed the randomizer to the time
		randfile = fopen("rand", "w");	//Create it the file
		for (i=0;i<60;i++)		//Create 60 random numbers and write them to the file
			fprintf(randfile, "%i\n", (int)rand());
		return;}	//Return

	i=0;
	while (fgets(c, 20, randfile) != NULL) {	//Read in all of the integer values from the file
		for (o=(strlen(c)-1); o>0 ;o--)
			r[i] += (int)c[o]*(10*(o+1));
		i++;
		}

	fclose(randfile);		//Close the file so we can rewrite it
	randfile = fopen("rand", "w");	//And open it for rewriting
	if (i==1) {			//If there was only 1 integer read in, delete the file.
		fclose(randfile);
		remove("rand");
		srand(time(NULL)); 
		return;}
	n=r[i-1];			//Assign the last value read in to n, which we will eventually return
	r[i-1] = NULL;			//Set the last value of the array r to null, so we dont rewrite it in the file
	for (o=i; o>0 ;o--)		//Loop through all the data in the array r and write it to file
		fprintf(randfile, "%i\n", r[o]);
	fclose(randfile);		//Cleanup
	srand(n);			//Return the random number
	return;
}

