OBJS=$(SRCS:%.c=%.o)
OBJS=$(SRCS:%.cpp=%.o)

$(KERNEL): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.o $(KERNEL)
