TARGET := send_h264file_rtp 

.PHONY : clean all

all: $(TARGET)

send_h264file_rtp: send_h264file_rtp.o llist_i386.o
	gcc $^ -o $@ -Wall

send_h264file_rtp.o: send_h264file_rtp.c
	gcc -c $< -o $@ -Wall

llist_i386.o: llist.c
	gcc -c $< -o $@ -Wall

clean:
	@rm -f $(TARGET) *.o

