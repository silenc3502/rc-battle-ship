TARGET = quad_kal
SRCS = this_kalman.c quad_kal.c
OBJS = $(SRCS:.c=.o)
LDFLAGS = -lm

all:	$(TARGET)

$(TARGET):	$(OBJS)
	gcc	-o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(TARGET)
