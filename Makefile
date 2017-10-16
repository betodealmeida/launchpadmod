CFLAGS = -Wall -I../../../include

all: launchpadmod.so

launchpadmod.so: launchpadmod.o
	$(LD) launchpadmod.o -o launchpadmod.so -shared

clean:
	rm -f *.o launchpadmod.so
