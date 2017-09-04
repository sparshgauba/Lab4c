#include "mraa.h"
#include "mraa/aio.h"
#include "mraa/gpio.h"
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

int main(void)
{
	int running = 0;
	mraa_gpio_context gpio;
    gpio = mraa_gpio_init(3);
    mraa_gpio_dir(gpio, MRAA_GPIO_IN);
    while (running == 0) {
        fprintf(stdout, "Gpio is %d\n", mraa_gpio_read(gpio));
        sleep(1);
    }
    return 0;
}
