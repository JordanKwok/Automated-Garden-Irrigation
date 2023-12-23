all:
	arm-linux-gnueabihf-gcc -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -pthread -Werror -Wshadow  pump.c 8x8led.c -o pump
	
	cp pump $(HOME)/cmpt433/public/myApps/public/

8x8led:
	arm-linux-gnueabihf-gcc -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -Wshadow 8x8led.c -o 8x8led
	
