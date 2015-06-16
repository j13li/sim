all: sim

sim: 
	g++ main.cpp functions.cpp -o sim -O3
sim2:
	g++ main2.cpp functions.cpp -o sim2 -O3
debug:
	g++ main.cpp functions.cpp -o sim -O3 -DDEBUG
prof:
	g++ main.cpp functions.cpp -o sim -O3 -pg
clean:
	rm -f sim
