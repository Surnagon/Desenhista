#!/bin/sh

echo 23 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio23/direction
echo 15 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio15/direction
echo 1 > /sys/class/gpio/gpio15/value
 
trap "echo 23 > /sys/class/gpio/unexport; echo 15 > /sys/class/gpio/unexport; exit 0" 2

#~ g++ imagem.cpp -o image_proc `pkg-config --cflags --libs opencv`
#~ g++ servo_solo.c -o servo -lwiringPi

while true
do
    botao=$(cat /sys/class/gpio/gpio23/value)
    if [ "$botao" -eq 1 ];
        then
			echo "Foi"
            echo 0 > /sys/class/gpio/gpio15/value
            sleep 3
            
            echo 1 > /sys/class/gpio/gpio15/value
			#~ ./image_proc
			
			./servo
        else
            echo 1 > /sys/class/gpio/gpio15/value
    fi
sleep 0.5
done
