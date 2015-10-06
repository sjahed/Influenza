#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>
#include <conio.h>
#include <time.h>
#include <math.h>

#define num_Replic 50     //Set the replication times of simulation
#define num_Gen 10       //Select the first 10 generations

float max_RN_p;          //Maximum reproduction number of first ten generation of both types of pendemic flu
float max_RN_s;          //Maximum reproduction number of first ten generation of both types of seasonal flu
float max_symRN_p;       //Maximum reproduction number of first ten generation of only systematic pendemic flu
float max_asymRN_p;      //Maximum reproduction number of first ten generation of only asystematic pendemic flu
float max_symRN_s;       //Maximum reproduction number of first ten generation of only systematic seasonal flu
float max_asymRN_s;      //Maximum reproduction number of first ten generation of only asystematic seasonal flu

float outputRN[11][13];  //Array of first ten generations of outputed reproduction number files
float rn_p[num_Replic][num_Gen];      //Array of pendemic reproduction number for each generation each replication
float rn_s[num_Replic][num_Gen];      //Array of seasonal reproduction number for each generation each replication
float symrn_p[num_Replic][num_Gen];   //Array of symptomatic pendemic reproduction number for each generation each replication
float asymrn_p[num_Replic][num_Gen];  //Array of asymptomatic pendemic reproduction number for each generation each replication
float symrn_s[num_Replic][num_Gen];   //Array of symptomatic seasonal reproduction number for each generation each replication
float asymrn_s[num_Replic][num_Gen];  //Array of asymptomatic seasonal reproduction number for each generation each replication

void main(void);
int max_array(float array[11][13], int row, int column);
void statistics(float a[num_Replic][num_Gen]);

FILE *output1, *output2, *output3, *output4, *output5, *output6, *output7, *output8, *output9;

void main(void)
{	
	int i = 0, j = 0, k = 0;
	char buf[1024];
	int numGen_max_RN_p = 0 , numGen_max_RN_s = 0, numGen_max_symRN_p = 0, numGen_max_asymRN_p = 0, numGen_max_symRN_s = 0, numGen_max_asymRN_s = 0;
	float sum_max_RN_p = 0, sum_max_RN_s = 0, sum_max_symRN_p = 0, sum_max_asymRN_p = 0, sum_max_symRN_s = 0, sum_max_asymRN_s = 0;
	float ave_max_RN_p = 0, ave_max_RN_s = 0, ave_max_symRN_p = 0, ave_max_asymRN_p = 0, ave_max_symRN_s = 0, ave_max_asymRN_s = 0;

	output3 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\rn_p.txt", "wt");
	output4 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\rn_s.txt", "wt");
	output5 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\symrn_p.txt", "wt");
	output6 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\asymrn_p.txt", "wt");
	output7 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\symrn_s.txt", "wt");
	output8 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\asymrn_s.txt", "wt");
	output1 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\Max_Avg_RN.txt", "wt");
	fprintf(output1, "Gen#	Max_RN_P	Gen#	Max_RN_S	Gen#	Max_SymRN_P	Gen#	Max_ASymRN_P	Gen#	Max_SymRN_S	Gen#	Max_ASymRN_S\n");
	
	for (i = 1; i <= num_Gen; ++i)
	{
		fprintf(output3, "gen %d	", i);
		fprintf(output4, "gen %d	", i);
		fprintf(output5, "gen %d	", i);
		fprintf(output6, "gen %d	", i);
		fprintf(output7, "gen %d	", i);
		fprintf(output8, "gen %d	", i);
	}

	fprintf(output3, "\n");
	fprintf(output4, "\n");
	fprintf(output5, "\n");
	fprintf(output6, "\n");
	fprintf(output7, "\n");
	fprintf(output8, "\n");

	for (i = 1; i <= num_Replic; ++i)
	{ //Replication of simulation loop
		printf("Replication: %d\n", i);
		system("C:\\Users\\Yuwen\\Dropbox\\PI\\Single_city_43\\Debug\\Single_city_43.exe");
		if ((output2 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\Single_city_43\\output_R.txt", "rt")) != NULL) {
			fgets(buf, 1024, output2);
			//puts(buf);

			for (j = 0; j < num_Gen; ++j) { //Record first 10 generations of output reproduction numbers, totally 10 lines
				for (k = 0; k < 13; ++k) { //Take all 13 columns of output_R file
					fscanf(output2, "%f", &outputRN[j][k]);
					//printf("%f\n", outputRN[j][k]);
				}
			}
			fclose(output2);
		}
		//repli();	

		else {
			perror("Problem occur when opening file!");
		}

		//find the maximum reproduction number of the first ten generations
		numGen_max_RN_p = max_array(outputRN, 11, 2);
		max_RN_p = outputRN[numGen_max_RN_p][2];

		numGen_max_RN_s = max_array(outputRN, 11, 4);
		max_RN_s = outputRN[numGen_max_RN_s][4];

		numGen_max_symRN_p = max_array(outputRN, 11, 7);
		max_symRN_p = outputRN[numGen_max_symRN_p][7];

		numGen_max_asymRN_p = max_array(outputRN, 11, 8);
		max_asymRN_p = outputRN[numGen_max_asymRN_p][8];

		numGen_max_symRN_s = max_array(outputRN, 11, 11);
		max_symRN_s = outputRN[numGen_max_symRN_s][11];

		numGen_max_asymRN_s = max_array(outputRN, 11, 12);
		max_asymRN_s = outputRN[numGen_max_asymRN_s][12];

		//sum up the all the maximum reproducton numbers to get the average of them
		sum_max_RN_p = max_RN_p + sum_max_RN_p;
		sum_max_RN_s = max_RN_s + sum_max_RN_s;
		sum_max_symRN_p = max_symRN_p + sum_max_symRN_p;
		sum_max_asymRN_p = max_asymRN_p + sum_max_asymRN_p;
		sum_max_symRN_s = max_symRN_s + sum_max_symRN_s;
		sum_max_asymRN_s = max_asymRN_s + sum_max_asymRN_s;

		fprintf(output1, "%d	%f	%d	%f	%d	%f	%d	%f	%d	%f	%d	%f\n", numGen_max_RN_p+1, max_RN_p, numGen_max_RN_s+1, max_RN_s, numGen_max_symRN_p+1, \
			max_symRN_p, numGen_max_asymRN_p+1, max_asymRN_p, numGen_max_symRN_s+1, max_symRN_s, numGen_max_asymRN_s+1, max_asymRN_s);

		//write output reproduction numbers for each generation each replications to files
		for (j = 0; j < num_Gen; ++j) //
		{
			fprintf(output3, "%f	", outputRN[j][2]);
			fprintf(output4, "%f	", outputRN[j][4]);
			fprintf(output5, "%f	", outputRN[j][7]);
			fprintf(output6, "%f	", outputRN[j][8]);
			fprintf(output7, "%f	", outputRN[j][11]);
			fprintf(output8, "%f	", outputRN[j][12]);
		}

		fprintf(output3, "\n");
		fprintf(output4, "\n");
		fprintf(output5, "\n");
		fprintf(output6, "\n");
		fprintf(output7, "\n");
		fprintf(output8, "\n");

	}

	fclose(output3);
	fclose(output4);
	fclose(output5);
	fclose(output6);
	fclose(output7);
	fclose(output8);
	fclose(output1);

	ave_max_RN_p = sum_max_RN_p / num_Replic;
	ave_max_RN_s = sum_max_RN_s / num_Replic;
	ave_max_symRN_p = sum_max_symRN_p / num_Replic;
	ave_max_asymRN_p = sum_max_asymRN_p / num_Replic;
	ave_max_symRN_s = sum_max_symRN_s / num_Replic;
	ave_max_asymRN_s = sum_max_asymRN_s / num_Replic;

	output1 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\Max_Avg_RN.txt", "at+");
	fprintf(output1, "ave_max_RN_p	ave_max_RN_s	ave_max_symRN_p	ave_max_asymRN_p	ave_max_symRN_s	ave_max_asymRN_s\n");
	fprintf(output1, "%f	%f	%f	%f	%f	%f	\n", ave_max_RN_p, ave_max_RN_s, ave_max_symRN_p, ave_max_asymRN_p, ave_max_symRN_s, ave_max_asymRN_s);

	output3 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\rn_p.txt", "rt");
	output4 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\rn_s.txt", "rt");
	output5 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\symrn_p.txt", "rt");
	output6 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\asymrn_p.txt", "rt");
	output7 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\symrn_s.txt", "rt");
	output8 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\asymrn_s.txt", "rt");


	fgets(buf, 1024, output3);
	fgets(buf, 1024, output4);
	fgets(buf, 1024, output5);
	fgets(buf, 1024, output6);
	fgets(buf, 1024, output7);
	fgets(buf, 1024, output8);

	for (j = 0; j < num_Replic; ++j)
	{
		for (k = 0; k < num_Gen; ++k)
		{
			fscanf(output3, "%f", &rn_p[j][k]);
			fscanf(output4, "%f", &rn_s[j][k]);
			fscanf(output5, "%f", &symrn_p[j][k]);
			fscanf(output6, "%f", &asymrn_p[j][k]);
			fscanf(output7, "%f", &symrn_s[j][k]);
			fscanf(output8, "%f", &asymrn_s[j][k]);
		}
	}

	fclose(output1);
	fclose(output3);
	fclose(output4);
	fclose(output5);
	fclose(output6);
	fclose(output7);
	fclose(output8);

	output9 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\AvgEachgen.txt", "wt");

	statistics(rn_p, num_Replic, num_Gen);
	statistics(rn_s, num_Replic, num_Gen);
	statistics(symrn_p, num_Replic, num_Gen);
	statistics(asymrn_p, num_Replic, num_Gen);
	statistics(symrn_s, num_Replic, num_Gen);
	statistics(asymrn_s, num_Replic, num_Gen);

	fclose(output9);

}

/*******************************************************************************************************************************************************/
/*max function for two way array float type*/
int max_array(float array[11][13], int row, int column)
{
	int i = 0, m = 0;
	for (i = 1; i < row; ++i)
	{
		if (array[i][column] > array[m][column])
			m = i;
	}
	return m;
}

/*******************************************************************************************************************************************************/
/*calculate aver, var, stdDev*/
void statistics(float a[num_Replic][num_Gen], int m, int n) //m is replic, n is gen
{
	int i, j, v;
	//char s[3];
	float t;
	float(*p)[num_Gen];
	double aver2 = 0;
	double aver = 0, vari = 0, stdDev = 0, confintval = 0;
	//const CONFILEVEL = 0.95;

	p = &a[0][0];
	for (i = 0; i < n; ++i)
	{
		aver = 0;
		aver2 = 0;
		for (j = 0; j < m; ++j)
		{
			printf("%f", *(*(p + j) + i));
			aver += *(*(p + j) + i); //sum up all array elements
			aver2 += (*(*(p + j) + i))*(*(*(p + j) + i)); //sum up the square of all array elements
		}
		aver /= m; //get the average of all elements of each generation
		aver2 /= m; //get the average of the square of all elements of each generation
		vari = aver2 - (aver)*(aver);  //calculate var
		stdDev = sqrt(vari); //calculate stdDev
		//confintval = ttest_confintval(1 - CONFILEVEL, stdDev, num_Replic);
		fprintf(output9, "Generation: %d, aver = %f, var = %f, stdDev = %f\n", i + 1, aver, vari, stdDev);
	}
}

/*******************************************************************************************************************************************************/
float ttest_confintval(float alpha, float std, int size)
{
	int i, v;
	FILE *input1;

	v = size - 1; //degree of freedom;
	input1 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\ttest.txt", "rt");
	//for (i = 0; i < 12; ++i)
}