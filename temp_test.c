#include "mraa.h"
#include "mraa/aio.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <tgmath.h>

const int B  = 4275;
const int R0 = 100000;

int main(void){
	//uint16_t value = 0;
	mraa_aio_context adcPin;
	float adcValue, R, temp;
	time_t ltime;
	struct tm * timeptr;
	char output[25];
		
	mraa_init();
	adcPin = mraa_aio_init(0); 
	
	int logfd = open("log_part1.txt", O_CREAT | O_WRONLY | O_TRUNC);
	if(logfd < 0){
		fprintf(stderr, "Error opening log_part1.txt.\n");
		exit(1);
	}
	while(1){
		adcValue = mraa_aio_read(adcPin); // This reads from the sensor and stores in this value.
		// We need to convert this function into a temperature.
		R = 1023.0/((float)adcValue)-1.0;
		R *= 100000.0;
		
		temp = 1.0/(log(R/100000.0)/B+1/298.15)-273.15;
		temp = temp * 1.8 + 32;
		
		ltime = time(NULL);
		timeptr = localtime(&ltime);
		//strftime(output, 8, "%H:%M:%S", timeptr);		
		
		sprintf(output, "%.2d:%.2d:%.2d %0.1f F\n", timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, temp);
		printf("%s", output);
		//fprintf(logfd, "%s", output);
		write(logfd, output, strlen(output));
		
		sleep(1);
	}

	close(logfd);
	
	return 0;
	// g prints 
}
