#include <wiringPi.h>
#include <stdio.h>
//~ #include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
// #include "coordenadas.h"

float INNER_ARM = 14.5;
float OUTER_ARM = 14.5;
float X = 17.0;
float Y = 24.0;
float IX = 100.0;
float IY = 135.0;
int COLS = 100;
int ROWS = 76;
float OFFSET = 0.5;

typedef struct {
	float arr[3];
}coord;

coord pixel;
coord polar[7600];
coord polar_sorted[13500];
coord angulo;

float degrees(float radians){
	float degree = radians*180.0/M_PI;
	return degree;
}

coord pixels_to_polar(coord pixel_coord){
	coord polar_coord;
	float x = pixel_coord.arr[0] * X / IX + 5.0*OFFSET;
	//~ printf("%f\n", X);
	//~ printf("%f\n", IX);
	//~ printf("%f\n", pixel_coord.arr[0]);
	printf("x: %f\n", x);
	float y = pixel_coord.arr[1] * Y / IY + 4.0*OFFSET;
	//~ printf("%f\n", Y);
	//~ printf("%f\n", IY);
	//~ printf("%f\n", pixel_coord.arr[1]);
	printf("y: %f\n", y);
	float hypotenuse = sqrt(pow(x,2.0) + pow(y,2.0));
	float hypotenuse_angle = asin(x/hypotenuse);
	//~ printf("%f\n", hypotenuse_angle);
	float inner_angle = acos(
    (pow(hypotenuse,2.0) + pow(INNER_ARM,2.0) - pow(OUTER_ARM,2.0)) / (2.0 * hypotenuse * INNER_ARM)
	);
	float shoulder_motor_angle = hypotenuse_angle - inner_angle;
	//~ printf("%f\n", shoulder_motor_angle);
	float outer_angle = acos(
    (pow(INNER_ARM,2.0) + pow(OUTER_ARM,2.0) - pow(hypotenuse,2.0)) / (2.0 * INNER_ARM * OUTER_ARM)
	);
	float elbow_motor_angle = M_PI/2.0 - outer_angle;
	//~ printf("%f\n", elbow_motor_angle);
	polar_coord.arr[0] = degrees(shoulder_motor_angle) - 1.0*OFFSET;
	polar_coord.arr[1] = degrees(elbow_motor_angle) + 0.0*OFFSET;
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
	printf("%f\n", polar_coord.arr[2]);
	return polar_coord;
}

void control_servo(float angulo_anterior, float angulo, int pin_out)
{
	int w = 0;
	float t = 0;
	float subtarefa = 10.0;
	float distancia = angulo - angulo_anterior;
	if (pin_out == 8) {
		subtarefa = 1.0;
		distancia = distancia;
	}
	float subangulo;
	float subtempo = distancia/subtarefa;
	int high_us;
	int low_us;
	//~ high_us = (int)(-angulo*100.0/9.0 + 1500.0);
	//~ low_us  = 20000 - high_us;
	//~ while(t<fabs(distancia))
	//~ {
		//~ digitalWrite(pin_out, HIGH);
		//~ usleep(high_us);
		//~ digitalWrite(pin_out, LOW);
		//~ usleep(low_us);
		//~ t = t + 1.0;
	//~ }
	while(w<subtarefa)
	{
		subangulo = angulo_anterior + t*distancia/subtarefa;
		high_us = (int)(-subangulo*100.0/9.0 + 1500.0);
		low_us  = 20000 - high_us;
		while(t<fabs(subtempo))
		{
			digitalWrite(pin_out, HIGH);
			usleep(high_us);
			digitalWrite(pin_out, LOW);
			usleep(low_us);
			t = t + 1.0;
		} w++;
	}
	printf("Caiu!!!");
	kill(getpid(), SIGKILL);
}

void polaroide () {
	int j, k;
	int matrix[COLS][ROWS];
	std::ifstream file("cartesianas.csv");

	for(int row = 0; row < ROWS; ++row) {
		std::string line;
		std::getline(file, line);
		if ( !file.good() )
			break;

		std::stringstream iss(line);

		for (int col = 0; col < COLS; ++col)
		{
			std::string val;
			std::getline(iss, val, ',');
			if ( !iss.good() )
				break;

			std::stringstream convertor(val);
			convertor >> matrix[col][row];
		}
	}
	for (j=0; j<ROWS; j++) {
		for (k=0; k<COLS; k++) {
				pixel.arr[0] = k;
				pixel.arr[1] = j;
				pixel.arr[2] = matrix[k][j];
				printf("Coord = %d,%d \t", k, j);
				printf("Valor = %d\n", pixel.arr[2]);
				//~ pixel.arr[0] = 50;
				//~ pixel.arr[1] = 67.5;
				//~ pixel.arr[2] = 0.0;
				angulo = pixels_to_polar(pixel);
				
				polar[j*COLS+k] = angulo;
		}
	}
}

void sortido(bool insideout) {
	int a, b;
	if (insideout == true) {
		a = 0;
		b = 1;
	} else {
		a = 1;
		b = 0;
	}
	
	for(int i=0;i<ROWS*COLS;i++) {
		for(int j=0; j<ROWS*COLS; j++) {
			if(polar[j+1].arr[a] < polar[j].arr[a])
			{
				std::swap(polar[j+1].arr[0], polar[j].arr[0]);
				std::swap(polar[j+1].arr[1], polar[j].arr[1]);
				std::swap(polar[j+1].arr[2], polar[j].arr[2]);
			} else if ((polar[j+1].arr[a] == polar[j].arr[a]) && (polar[j+1].arr[b] < polar[j].arr[b])) {
				std::swap(polar[j+1].arr[0], polar[j].arr[0]);
				std::swap(polar[j+1].arr[1], polar[j].arr[1]);
				std::swap(polar[j+1].arr[2], polar[j].arr[2]);
			}
		}
    }
    int s = 0;
	int flag = 0;
		for (int k=0; k<ROWS*COLS; k++) {
			if (polar[k].arr[2] == 0) {
					flag++;
				} else {
					flag = 0;
				}
			if (flag <= 1) {
				polar_sorted[s] = polar[k];
				printf("Coord = %d,%d \t", polar[k].arr[0], polar[k].arr[1]);
				printf("Valor = %d\n", polar[k].arr[2]);
				s++;
		}
	}
}

// void take_picture(){
//     cv::VideoCapture camera(0);
//     cv::Mat frame;
// }
void set_realtime_priority() {
     int ret;
 
     // We'll operate on the currently running thread.
     pthread_t this_thread = pthread_self();
     printf("%d", this_thread);
     // struct sched_param is used to store the scheduling priority
     struct sched_param params;
 
     // We'll set the priority to the maximum.
     params.sched_priority = sched_get_priority_max(SCHED_FIFO);
     std::cout << "Trying to set thread realtime prio = " << params.sched_priority << std::endl;
 
     // Attempt to set thread real-time priority to the SCHED_FIFO policy
     ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
     if (ret != 0) {
         // Print the error
         std::cout << "Unsuccessful in setting thread realtime prio" << std::endl;
         return;     
     }
     // Now verify the change in thread priority
     int policy = 0;
     ret = pthread_getschedparam(this_thread, &policy, &params);
     if (ret != 0) {
         std::cout << "Couldn't retrieve real-time scheduling paramers" << std::endl;
         return;
     }
 
     // Check the correct policy was applied
     if(policy != SCHED_FIFO) {
         std::cout << "Scheduling is NOT SCHED_FIFO!" << std::endl;
     } else {
         std::cout << "SCHED_FIFO OK" << std::endl;
     }
 
     // Print thread scheduling priority
     std::cout << "Thread priority is " << params.sched_priority << std::endl; 
}

int main(void)
{
	int pin_servo[3] = {15, 9, 8};
	coord angulo_anterior = {-45.0, -45.0, -45.0};
	int pin_led = 4;
	int pin_btn = 7;
	pid_t pid_filho = 0;
	wiringPiSetup();
	pinMode(pin_servo[0], OUTPUT);
	pinMode(pin_servo[1], OUTPUT);
	pinMode(pin_servo[2], OUTPUT);
	pinMode(pin_led, OUTPUT);
	pinMode(pin_btn, INPUT);
	int i, h;
	
	set_realtime_priority();
	
	//~ polaroide();
	//~ sortido(true);
	int j, k;
	int ROWS = 0, COLS = 0;
	using vec = std::vector<std::string>;
	using matrix = std::vector<vec>;
	vec linha;
	matrix matriz;
	//~ std::vector<int> > linha;
	//~ std::vector<linha> > matrix;
	std::ifstream file("cartesianas.csv");
	
	std::string line, val;

	//~ while( getline( file, row ) )
	//~ {
		//~ ROWS++;
		//~ std::stringstream ss( row );
		//~ while ( getline ( ss, item, ',' ) ) linha.push_back( item );
		//~ matriz.push_back( linha );
	//~ }

	while( std::getline( file, line ) ) {
		if ( !file.good() )
			break;

		std::stringstream iss(line);

		while ( std::getline ( iss, val, ',' ) )
		{
			if ( !iss.good() )
				break;
			
			//~ std::stringstream convertor(val);
			linha.push_back( val.str() );
		}
		matriz.push_back( linha );
	}
	
	//~ for(int i=0;i<ROWS*COLS;i++) {
		//~ for(int j=0; j<ROWS*COLS; j++) {
			//~ if(polar[j+1].arr[a] < polar[j].arr[a])
			//~ {
				//~ std::swap(matrix[j+1].arr[0], polar[j].arr[0]);
				//~ std::swap(matrix[j+1].arr[1], polar[j].arr[1]);
				//~ std::swap(matrix[j+1].arr[2], polar[j].arr[2]);
			//~ } else if ((polar[j+1].arr[a] == polar[j].arr[a]) && (polar[j+1].arr[b] < polar[j].arr[b])) {
				//~ std::swap(polar[j+1].arr[0], polar[j].arr[0]);
				//~ std::swap(polar[j+1].arr[1], polar[j].arr[1]);
				//~ std::swap(polar[j+1].arr[2], polar[j].arr[2]);
			//~ }
		//~ }
    //~ }
	
	while(1)
	{
		//~ printf("Digite um ângulo entre -90 e 90 graus: ");
		//~ scanf("%f", &angulo[0]);
		//~ scanf("%f", &angulo[1]);
		//~ scanf("%f", &angulo[2]);
		
		//~ pthread_t thId;
		//~ thId = pthread_self();
		//~ pthread_attr_t thAttr;
		//~ int policy = 0;
		//~ int max_prio_for_policy = 0;

		//~ pthread_attr_init(&thAttr);
		//~ pthread_attr_getschedpolicy(&thAttr, &policy);
		//~ max_prio_for_policy = sched_get_priority_max(policy);


		//~ pthread_setschedprio(thId, max_prio_for_policy);
		//~ pthread_attr_destroy(&thAttr);
		//~ for (h=0; h<(sizeof(polar_sorted)/sizeof(coord)); h++) {
			//~ angulo = polar_sorted[h];
			//~ angulo = {h, h, 0};
			//~ printf("%d\n", h);
			//~ angulo = {polar_sorted[h].arr[0], polar_sorted[h].arr[1], 0};
			//~ angulo_anterior = {-45, -45, -45};
		
		for (j=0; j<ROWS; j++) {
			for (k=0; k<COLS; k++) {
				pixel.arr[0] = k;
				pixel.arr[1] = j;
				pixel.arr[2] = matriz[k][j];
				printf("Coord = %d,%d \t", k, j);
				printf("Valor = %d\n", pixel.arr[2]);
				//~ pixel.arr[0] = 50;
				//~ pixel.arr[1] = 67.5;
				//~ pixel.arr[2] = 0.0;
				if (pixel.arr[2] != 0) {
					angulo = pixels_to_polar(pixel);

					digitalWrite(pin_led, HIGH);
					usleep(300000);
					digitalWrite(pin_led, LOW);
				
					
					for(i=2; i>=0; i--)
					{
						pid_filho = fork();
						if(pid_filho==0)
							//~ control_servo(angulo_anterior.arr[i], 0, pin_servo[i]);
							control_servo(angulo_anterior.arr[i], angulo.arr[i], pin_servo[i]);
							printf("motor: %d\tangulo: %.2f\n", i, angulo.arr[i]);
						if(pid_filho != 0)
							angulo_anterior.arr[i] = angulo.arr[i];
							//kill(pid_filho, SIGKILL); 
						}
					}
				}
			}
		}
	return 0;
}
