PROGRAM = V3DTest
OBJS = main.o Buffer.o Triangle.o
CXXFLAGS = -Og -ggdb3 -std=c++23 -fno-exceptions -fno-omit-frame-pointer
LDFLAGS = -lgcc_s -lcore -lcoreminimal -lcorebasic -lgui -lgfx -lmain

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm $(OBJS) $(PROGRAM)

run:
	./$(PROGRAM)
