#include "mraa.h"
#include "mraa/aio.h"
#include "mraa/gpio.h"
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>


#define B 		4275
#define R0		100000
#define MAX_LEN 50

int PORTNO = 18000;
int sockfd;
char scale_opt = 'F';
char id_num[10] = {0};
int log_fd;
int log_opt = 0;
int shutdown_flag = 0;
int stop = 0;
int period = 1;
char command[MAX_LEN] = {0};
pthread_t button_thread;
pthread_t input_thread;
pthread_mutex_t mutex;

//Continuously cheks for button press
void *button_press(void *arg)
{
	mraa_gpio_context button = (mraa_gpio_context)arg;
	
	while (mraa_gpio_read(button) != 1);

	pthread_mutex_lock (&mutex);
	shutdown_flag = 1;
	pthread_mutex_unlock (&mutex);
	return NULL;
}

int command_digest(int len)
{
    char *compare[] = {"START", "STOP", "OFF", "SCALE=C", "SCALE=F", "PERIOD="};
    char *ptr = command;
    int i = 0;
    int ptr_i = 0;
    char cmd[25] = {0};
    pthread_mutex_lock (&mutex);
    while (i < len)
    {
        if (command[i] == '\n')
        {
            if (memcmp(ptr, compare[0], i-ptr_i) == 0 && i-ptr_i == 5) //START
            {
                printf("START\n");
                strcpy(cmd, "START\n");
				if(log_opt)
				{
					write(log_fd, cmd, strlen(cmd));
				}
                //write(sockfd, cmd, strlen(cmd));

				stop = 0;
                ptr += 6;
                ptr_i = i+1; 
            }
            else if (memcmp(ptr, compare[1], i-ptr_i) == 0 && i-ptr_i == 4) //STOP
            {
                printf("STOP\n");
                strcpy(cmd, "STOP\n");
				if(log_opt)
				{	
					write(log_fd, cmd, strlen(cmd));
				}
                //write(sockfd, cmd, strlen(cmd));

				stop = 1;
                ptr += 5;
                ptr_i = i+1; 
            }
            else if (memcmp(ptr, compare[2], i-ptr_i) == 0 && i-ptr_i == 3) //OFF
            {
                shutdown_flag = 1;
                return -1;
                ptr += 4;
                ptr_i = i+1; 
            }
            else if (memcmp(ptr, compare[3], i-ptr_i) == 0 && i-ptr_i == 7) //CELCIUS
            {
                printf("SCALE=C\n");
                strcpy(cmd, "SCALE=C\n");
				if(log_opt)
				{
					write(log_fd, cmd, strlen(cmd));
				}
                //write(sockfd, cmd, strlen(cmd));

				scale_opt = 'C';
                ptr += 8;
                ptr_i = i+1; 
            }
            else if (memcmp(ptr, compare[4], i-ptr_i) == 0 && i-ptr_i == 7) //FARHENHEIT
            {
                printf("SCALE=F\n");
                strcpy(cmd, "SCALE=F\n");
				if(log_opt)
				{
					write(log_fd, cmd, strlen(cmd));
				}
                //write(sockfd, cmd, strlen(cmd));
				scale_opt = 'F';
                ptr += 8;
                ptr_i = i+1; 
            }
            else if (memcmp(ptr, compare[5], 7) == 0) // PERIOD CHANGE
            {
                char *num = (char*) calloc(i - (ptr_i + 7) + 1, sizeof(char));
                memcpy(num,(ptr + 7), i - (ptr_i + 7));
                sprintf(cmd,"PERIOD=%d\n", atoi(num));
                printf("%s", cmd);
                if(log_opt)
				{
					write(log_fd, cmd, strlen(cmd));
				}
                //write(sockfd, cmd, strlen(cmd));

				period = atoi(num);
                free(num);
                ptr += (7+i - (ptr_i + 7)+1);
                ptr_i = i+1; 
            }
            else 
            {
                ptr += i - ptr_i + 1;
                ptr_i = i+1;
            }
        }
        i++;
    }

    memmove(command, ptr, i - ptr_i);

    pthread_mutex_unlock (&mutex);
    return (i - ptr_i);
}


void *user_input()
{
	int read_count = 0;
    int offset = 0;
    struct pollfd u_input;
    u_input.fd = sockfd;
    u_input.events = POLLIN;

    while (1)
    {
        poll(&u_input, 1, -1);
        if (u_input.revents == POLLIN)
        {
            read_count += read(sockfd, (command + offset), 68);
            offset = command_digest(read_count);
            if(offset == -1)
            {
            	return NULL;
            }
            read_count = offset;       
        }
    }
}

int main(int argc, char *argv[])
{
	//uint16_t value = 0;
	mraa_aio_context tmpr = mraa_aio_init(0);
	mraa_gpio_context button = mraa_gpio_init(3);
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	pthread_mutex_init(&mutex, NULL);

	float raw_value; 
	float R;
	float temp;
	time_t local_time;
	struct tm * t_ptr;
	char output[25];
	char * host = 0;

	struct option longOptions[] = 
    {
        {"log"   , required_argument, 0, 'l' },
        {"period", required_argument, 0, 'p' },
        {"scale" , required_argument, 0, 's' },
        {"id"    , required_argument, 0, 'i' },
        {"host"  , required_argument, 0, 'h' },
	    {""	 , required_argument, 0, 'p' },
        {0       , 0                , 0, 0   }
    };

    char val;
    while ((val = getopt_long(argc, argv, "", longOptions, 0)) != -1)
    {
        if (val == 'p')
        {
            period = atoi(optarg);
        }
        else if (val == 'i')
        {
            if (strlen(optarg) != 9)
            {
                fprintf(stderr, "Error: Incorrect ID length\n");
                exit(1); //CHECK ERROR CODES
            }
            strcpy(id_num, optarg);
        }
        else if (val == 'h')
        {
            host = (char *) calloc(strlen(optarg) + 1, sizeof(char));
            strcpy(host, optarg);
        }
        else if (val == 's')
        {
            scale_opt = *optarg;
            if (scale_opt != 'C' && scale_opt != 'F')
            {
                fprintf(stderr, "Wrong scale option, default to F\n");
                scale_opt = 'F';
            }
        }
        else if (val == 'l')
        {
        	log_opt = 1;
            if ((log_fd = open(optarg, O_CREAT | O_WRONLY | O_TRUNC)) < 0)
            {
                perror("Error opening log file");
                exit(1); //CHECK ERROR CODES
            }
        }
        else if (val == 'p')
        {
            PORTNO = atoi(optarg);
        }
    }
    if (PORTNO == -1)
    {
    	fprintf(stderr, "Error: Port number not given\n");
    	exit(1);
    }
    if (host == NULL)
    {
        host = (char *) calloc(strlen("131.179.192.136") + 1, sizeof(char));
        strcpy (host, "131.179.192.136");
    }


    struct sockaddr_in serv_addr;
    struct hostent *server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("Error opening socket");
    }

    if ((server = gethostbyname(host)) == NULL) 
    {
        fprintf(stderr,"Error no such host\n");
        exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(PORTNO);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        perror("Connection Error");
        exit(1);
    }

    char init_message[] = "ID=204600605\n";
    write (sockfd, init_message, strlen(init_message));
    if (log_opt)
	write (log_fd, init_message, strlen(init_message));

	pthread_create(&button_thread, NULL, &button_press, (void *)button);
	pthread_create(&input_thread, NULL, &user_input, NULL);
	
	while(!shutdown_flag)
	{
		if (stop)
		{
			continue;
		}
		raw_value = mraa_aio_read(tmpr); // This reads from the sensor and stores in this value.
		// We need to convert this function into a temperature.
		R = 1023.0/raw_value-1.0;
		R *= R0;
		
		temp = 1.0/(log(R/100000.0)/B+1/298.15)-273.15;
		if (scale_opt == 'F')
		{
			temp = (temp*1.8) + 32;
		}
		
		local_time = time(NULL);
		t_ptr = localtime(&local_time);	
		sprintf(output, "%.2d:%.2d:%.2d %0.1f\n", t_ptr->tm_hour, t_ptr->tm_min, t_ptr->tm_sec, temp);
		
		printf("%s", output);
       		write (sockfd, output, strlen(output));
		if(log_opt)
		{
			write (log_fd, output, strlen(output));
		}
		
		sleep(period);
	}
	local_time = time(NULL);
	t_ptr = localtime(&local_time);
	sprintf(output, "%.2d:%.2d:%.2d SHUTDOWN\n", t_ptr->tm_hour, t_ptr->tm_min, t_ptr->tm_sec);
	printf("%s", output);
    	write (sockfd, output, strlen(output));
	if(log_opt)
	{
		write(log_fd, output, strlen(output));
	}
	pthread_cancel(button_thread);
	pthread_cancel(input_thread);
	if (log_opt)
	{
		close(log_fd);
	}
	return 0; 
}

