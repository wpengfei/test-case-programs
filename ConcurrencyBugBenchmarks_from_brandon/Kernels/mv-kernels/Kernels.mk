CFLAGS=-g -O0
CXXFLAGS=-g -O0
LDFLAGS=-lpthread -lm

OBJS = $(KERNEL).o

default: $(KERNEL)

%.o : %.cpp %.c
	$(CC) $(CXXFLAGS) $(CFLAGS) -c -o $@ $<

$(KERNEL): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(KERNEL) $(OBJS)

clean:
	rm -f *.o $(KERNEL)

include ../../Running.mk
