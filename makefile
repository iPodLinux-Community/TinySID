# Last updated: Jun 29, 2008
# ~Keripo
#
# TinySID makefile
# Modified for iPodLinux
#
DEFS =
CFLAGS = $(DEFS) -O
LDFLAGS = -elf2flt
LIBS = -lpthread
CC = arm-uclinux-elf-gcc

OBJECTS_A = tinysid.o sidengine.o soundcard.o ipod_volume_control.o

ARCH_FILES = tinysid.c sidengine.c soundcard.c ipod_volume_control.c makefile defines.h

tinysid: $(OBJECTS_A)
		$(CC) -o tinysid $(LDFLAGS) $(OBJECTS_A) $(LIBS)

sidengine.o: sidengine.c
		$(CC) -c $(CFLAGS) sidengine.c

soundcard.o: soundcard.c
		$(CC) -c $(CFLAGS) soundcard.c

ipod_volume_control.o: ipod_volume_control.c
		$(CC) -c $(CFLAGS) ipod_volume_control.c
		
clean:
	echo Cleaning up...
	rm *.o
	rm tinysid tinysid.gdb
	echo OK.
