CC=gcc
TARGET=review
SRCS=producer_consumer.c
LIBS=-lpthread
$(TARGET):$(SRCS)
	$(CC) $(SRCS) -o $(TARGET) $(LIBS)
clean:
	rm -f $(TARGET)
	