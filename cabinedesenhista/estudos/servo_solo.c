#include <wiringPi.h>
#include <stdio.h>
//~ #include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <sstream>
#include <fstream>
// #include "coordenadas.h"

float INNER_ARM = 14.5;
float OUTER_ARM = 14.5;
float X = 20.0;
float Y = 27.0;
float IX = 100.0;
float IY = 135.0;
float OFFSET = 0.5;

typedef struct {
	float arr[3];
}coord;

float degrees(float radians){
	float degree = radians*180.0/M_PI;
	return degree;
}



coord pixels_to_polar(coord pixel_coord){
	coord polar_coord;
	float x = pixel_coord.arr[0] * X / IX + 2*OFFSET;
	//~ printf("%f\n", X);
	//~ printf("%f\n", IX);
	//~ printf("%f\n", pixel_coord.arr[0]);
	printf("ponto: %f\n", x);
	float y = pixel_coord.arr[1] * Y / IY + OFFSET;
	//~ printf("%f\n", Y);
	//~ printf("%f\n", IY);
	//~ printf("%f\n", pixel_coord.arr[1]);
	//~ printf("ponto: %f\n", y);
	float hypotenuse = sqrt(pow(x,2) + pow(y,2));
	float hypotenuse_angle = asin(x/hypotenuse);
	//~ printf("%f\n", hypotenuse_angle);
	float inner_angle = acos(
    (pow(hypotenuse,2) + pow(INNER_ARM,2) - pow(OUTER_ARM,2)) / (2 * hypotenuse * INNER_ARM)
	);
	float shoulder_motor_angle = hypotenuse_angle - inner_angle;
	//~ printf("%f\n", shoulder_motor_angle);
	float outer_angle = acos(
    (pow(INNER_ARM,2) + pow(OUTER_ARM,2) - pow(hypotenuse,2)) / (2 * INNER_ARM * OUTER_ARM)
	);
	float elbow_motor_angle = M_PI/2 - outer_angle;
	//~ printf("%f\n", elbow_motor_angle);
	polar_coord.arr[0] = degrees(shoulder_motor_angle) + 10*OFFSET;
	polar_coord.arr[1] = degrees(elbow_motor_angle);
	polar_coord.arr[2] = pixel_coord.arr[2];
	if (pixel_coord.arr[2] == 1) {
		polar_coord.arr[2] = 90.0;
	} else if (pixel_coord.arr[2] == 0) {
		polar_coord.arr[2] = 0.0;
	} else if (pixel_coord.arr[2] > 1) {
		polar_coord.arr[2] = -90.0;
	}
	printf("%f\n", polar_coord.arr[0]);
	printf("%f\n", polar_coord.arr[1]);
	return polar_coord;
}

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
	coord pixel;
	//~ coord angulo;
	coord angulo_anterior = {0.0, 0.0, 0.0};
	int pin_led = 4;
	int pin_btn = 7;
	pid_t pid_filho = 0;
	wiringPiSetup();
	pinMode(pin_servo[0], OUTPUT);
	pinMode(pin_servo[1], OUTPUT);
	pinMode(pin_servo[2], OUTPUT);
	pinMode(pin_led, OUTPUT);
	pinMode(pin_btn, INPUT);
	int i, j, k;
	//~ pthread_t thId;
	
	while(1)
	{
		//~ printf("Digite um ângulo entre -90 e 90 graus: ");
		//~ scanf("%f", &angulo[0]);
		//~ scanf("%f", &angulo[1]);
		//~ scanf("%f", &angulo[2]);
		
		//~ printf("Digite um ângulo entre -90 e 90 graus: ");
		//~ scanf("%f", &pixel.arr[0]);
		//~ scanf("%f", &pixel.arr[1]);
		//~ scanf("%f", &pixel.arr[2]);
		
		int matrix[76][100];
		//~ float polar[76000][3];
		std::ifstream file("cartesianas.csv");

		for(int row = 0; row < 76; ++row)
		{
			std::string line;
			std::getline(file, line);
			if ( !file.good() )
				break;

			std::stringstream iss(line);

			for (int col = 0; col < 100; ++col)
			{
				std::string val;
				std::getline(iss, val, ',');
				if ( !iss.good() )
					break;

				std::stringstream convertor(val);
				convertor >> matrix[row][col];
			}
		}
		
		for (j=0; j<76; j++) {
			for (k=0; k<100; k++) {
				//~ polar[][0] = 
				//~ polar[][0] = 
				//~ polar[][0] = 
			
				pixel.arr[0] = j;
				pixel.arr[1] = k;
				pixel.arr[2] = matrix[j][k];
				coord angulo = pixels_to_polar(pixel);
				
				
				digitalWrite(pin_led, HIGH);
				usleep(300000);
				digitalWrite(pin_led, LOW);
				
				//~ thId = pthread_self();
				//~ pthread_attr_t thAttr;
				//~ int policy = 0;
				//~ int max_prio_for_policy = 0;

				//~ pthread_attr_init(&thAttr);
				//~ pthread_attr_getschedpolicy(&thAttr, &policy);
				//~ max_prio_for_policy = sched_get_priority_max(policy);


				//~ pthread_setschedprio(thId, max_prio_for_policy);
				//~ pthread_attr_destroy(&thAttr);
				for(i=2; i>=0; i--)
				{
					pid_filho = fork();
					if(pid_filho==0)
						control_servo(angulo_anterior.arr[i], angulo.arr[i], pin_servo[i]);
					if(pid_filho != 0)
						angulo_anterior.arr[i] = angulo.arr[i];
						//kill(pid_filho, SIGKILL);
				}
			}
		}
	}
	return 0;
}
