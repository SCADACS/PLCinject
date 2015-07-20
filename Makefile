
CC = cc 
CFLAGS = -std=gnu99
SNAP7LIB = lib/libsnap7.so
SNAP7INCLUDE = snap7/release/Wrappers/c-cpp
INCLUDE = -I$(SNAP7INCLUDE)

EXECUTABLE = plcinject


all: $(EXECUTABLE)

$(EXECUTABLE): main.o
	$(CC) -o $@ $^ $(SNAP7LIB)

main.o: main.c
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

clean:
	rm main.o plcinject
