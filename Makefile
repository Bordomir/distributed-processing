SOURCES=$(wildcard *.cpp)
HEADERS=$(SOURCES:.cpp=.h)
FLAGS=-g

all: main

main: $(SOURCES) $(HEADERS)
	mpic++ $(SOURCES) $(FLAGS) -o main

debug: $(SOURCES) $(HEADERS)
	mpic++ $(SOURCES) $(FLAGS) -DDEBUG -o main

clear: clean

clean:
	rm main

run: main
	mpirun -oversubscribe --allow-run-as-root -np 8 ./main

drun: debug
	mpirun -oversubscribe --allow-run-as-root -np 8 ./main
