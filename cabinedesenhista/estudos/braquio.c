#include <wiringPi.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "opencv.hpp"
#include "iostream"
// #include "coordenadas.h"

void control_servo(float angulo, int pin_out)
{
	int high_us = (int)(angulo*50.0/9.0 + 1500);
	int low_us  = 20000 - high_us;
	while(1)
	{
		digitalWrite(pin_out, HIGH);
		usleep(high_us);
		digitalWrite(pin_out, LOW);
		usleep(low_us);
	}
}

int cartesiano_to_polar(int x, int y){ // Essa funcao serah imbutida na funcao abaixo

    return polar
}

void montagem_matrix(int coord[1200]){ // Essa funcao serah movida para dentro da biblioteca coordenadas.h

    polar = [30][40];
    int i, j, k = 0;

    for(i=0; i<30; i++) {
        for(j=0;j<40;j++) {
            polar[i][j] = cartesiano_to_polar(i, j)
            
            k++
        }
    }
}

// void take_picture(){
//     cv::VideoCapture camera(0);
//     cv::Mat frame;
// }

int main(void)
{
	int pin_servo = 0;
	int pin_led = 4;
	int pin_btn = 7;
	float angulo;
	int pid_filho = 0;
	wiringPiSetup();
	pinMode(pin_servo, OUTPUT);
	pinMode(pin_led, OUTPUT);
	pinMode(pin_btn, INPUT);

	while(1)
	{
		printf("Digite um ângulo entre -90 e 90 graus: ");
		scanf("%f", &angulo);

		digitalWrite(pin_led, HIGH);
		usleep(3000000);
		take_picture();
		usleep(300000);
		digitalWrite(pin_led, LOW);

		if(pid_filho != 0)
			kill(pid_filho, SIGKILL);
		pid_filho = fork();
		if(pid_filho==0)
			control_servo(angulo, pin_servo);
	}
	return 0;
}
