#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>

#define num_Replic 1     //Set the replication times of simulation
#define num_Gen 10       //Select the first 10 generations

float max_RN_p;          //Maximum reproduction number of first ten generation of both types of pendemic flu
float max_RN_s;          //Maximum reproduction number of first ten generation of both types of seasonal flu
float max_symRN_p;       //Maximum reproduction number of first ten generation of only systematic pendemic flu
float max_asymRN_p;      //Maximum reproduction number of first ten generation of only asystematic pendemic flu
float max_symRN_s;       //Maximum reproduction number of first ten generation of only systematic seasonal flu
float max_asymRN_s;      //Maximum reproduction number of first ten generation of only asystematic seasonal flu

float outputRN[11][13];  //Array of first ten generations of outputed reproduction number files

void main(void);
int repli(void);
float max_array(float array[11][13], int row, int column);

FILE *output4, *output5;

void main(void)
{	
	int i = 0, j = 0, k = 0;
	float sum_max_RN_p = 0, sum_max_RN_s = 0, sum_max_symRN_p = 0, sum_max_asymRN_p = 0, sum_max_symRN_s = 0, sum_max_asymRN_s = 0;
	float ave_max_RN_p = 0, ave_max_RN_s = 0, ave_max_symRN_p = 0, ave_max_asymRN_p = 0, ave_max_symRN_s = 0, ave_max_asymRN_s = 0;
	for (i = 0; i < num_Replic; ++i){ //Replication of simulation loop
		printf("Replication: %d\n", i);
		repli();
		output4 = fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\Single_city_43\\output_R.txt", "r");
		for (j = 0; j <= num_Gen; ++j){ //Record the first line title and first 10 generations of output reproduction numbers, totally 11 lines
			for (k = 0; k <13; ++k){ //Take all 13 columns of output_R file
				fscanf(output4, "%f", &outputRN[j][k]);
			}
		}
		max_RN_p = max_array(outputRN, 11, 2);
		max_RN_s = max_array(outputRN, 11, 4);
		max_symRN_p = max_array(outputRN, 11, 7);
		max_asymRN_p = max_array(outputRN, 11, 8);
		max_symRN_s = max_array(outputRN, 11, 11);
		max_asymRN_s = max_array(outputRN, 11, 12);
		
		sum_max_RN_p = max_RN_p + sum_max_RN_p;
		sum_max_RN_s = max_RN_s + sum_max_RN_s;
		sum_max_symRN_p = max_symRN_p + sum_max_symRN_p;
		sum_max_asymRN_p = max_asymRN_p + sum_max_asymRN_p;
		sum_max_symRN_s = max_symRN_s + sum_max_symRN_s;
		sum_max_asymRN_s = max_asymRN_s + sum_max_asymRN_s;
	}
	ave_max_RN_p = sum_max_RN_p / num_Replic;
	ave_max_RN_s = sum_max_RN_s / num_Replic;
	ave_max_symRN_p = sum_max_symRN_p / num_Replic;
	ave_max_asymRN_p = sum_max_asymRN_p / num_Replic;
	ave_max_symRN_s = sum_max_symRN_s / num_Replic;
	ave_max_asymRN_s = sum_max_asymRN_s / num_Replic;

	fopen("C:\\Users\\Yuwen\\Dropbox\\PI\\PI_Replications\\")
	fprintf(output5, "%f	%f	%f	%f	%f	%f	\n", ave_max_RN_p, ave_max_RN_s, ave_max_symRN_p, ave_max_asymRN_p, ave_max_symRN_s, ave_max_asymRN_s);

}
int repli(void)
{
	HINSTANCE hNewExe = ShellExecuteA(NULL, "open", "C:\\Users\\Yuwen\\Dropbox\\PI\\Single_city_43\\Debug\\Single_city_43.exe", NULL, NULL, SW_SHOW);
	if ((DWORD)hNewExe <= 32)
	{
		printf("return value:%d\n", (DWORD)hNewExe);
	}
	else
	{
		printf("successed!\n");
	}
	printf("GetLastError: %d\n", GetLastError());
	system("pause");
	return 1;
}

/*max function for two way array float type*/
float max_array(float array[11][13], int row, int column)
{
	int i = 0, m = 1;
	for (i = 2; i < row; ++i)
	if (array[i][column] > array[m][column])
		m = i;
	return array[m][column];
}