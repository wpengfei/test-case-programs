CC=gcc
CXX=g++


SIM=-D'NOOP(x)=x'
REAL=-D'NOOP(x)={}'

#To turn the simulation bug detection hooks into noops, change REAL to SIM
CFLAGS=-g -O0 $(REAL)
CXXFLAGS=-g -O0 $(REAL)
LDFLAGS=-lpthread -lm

COBJS = $(SRCS:%.c=%.o) 
CXXOBJS = $(SRCS:%.cpp=%.o) 
OBJS = $(COBJS) $(CXXOBJS)

%.o : %.c
	$(CC) $(CXXFLAGS) $(CFLAGS) -c -o $@ $<

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(CFLAGS) -c -o $@ $<

plain: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(KERNEL) $<


test:
	BUGGYFILE=test.$(KERNEL) ./$(KERNEL)

clean:
	-rm -f *.o $(KERNEL)
