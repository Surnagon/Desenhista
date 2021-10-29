#include <wiringPi.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
// #include "coordenadas.h"

void control_servo(float angulo_anterior, float angulo, int pin_out)
{
	float t = 0.0;
	float distancia = angulo - angulo_anterior;
	int high_us = (int)(-angulo*100.0/9.0 + 1500.0);
	int low_us  = 20000 - high_us;
	while(t<fabs(distancia))
	{
		digitalWrite(pin_out, HIGH);
		usleep(high_us);
		digitalWrite(pin_out, LOW);
		usleep(low_us);
		t = t + 1.0;
	}
	printf("Caiu!!!");
	kill(getpid(), SIGKILL);
}

// void take_picture(){
//     cv::VideoCapture camera(0);
//     cv::Mat frame;
// }

int main(void)
{
	int pin_servo[3] = {15, 9, 8};
	float angulo[3];
	float angulo_anterior[3] = {0.0, 0.0, 0.0};
	int pin_led = 4;
	int pin_btn = 7;
	pid_t pid_filho = 0;
	wiringPiSetup();
	pinMode(pin_servo[0], OUTPUT);
	pinMode(pin_servo[1], OUTPUT);
	pinMode(pin_servo[2], OUTPUT);
	pinMode(pin_led, OUTPUT);
	pinMode(pin_btn, INPUT);
	int i;
	while(1)
	{
		printf("Digite um ângulo entre -90 e 90 graus: ");
		scanf("%f", &angulo[0]);
		scanf("%f", &angulo[1]);
		scanf("%f", &angulo[2]);
		
		digitalWrite(pin_led, HIGH);
		usleep(3000000);
		digitalWrite(pin_led, LOW);
		
		for(i=0; i<3; i++)
		{
			pid_filho = fork();
			if(pid_filho==0)
				control_servo(angulo_anterior[i], angulo[i], pin_servo[i]);
			if(pid_filho != 0)
				angulo_anterior[i] = angulo[i];
				//kill(pid_filho, SIGKILL);
		}
	}
	return 0;
}
