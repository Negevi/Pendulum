LDFLAGS = `pkg-config --libs opencv4`
CFLAGS = `pkg-config --cflags opencv4`

all:
	g++ $(CFLAGS) main.cpp -o video $(LDFLAGS)
clean:
	rm video