CFLAGS = -Wall -O2
LDFLAGS = -lpthread

TARGET := send_h264file_rtp \
		  test_tcp_send_nal test_tcp_recv_nal \
		  test_decode_rtp2h264

.PHONY : clean all

all: $(TARGET)

test_tcp_send_nal: test_tcp_send_nal.o

test_tcp_recv_nal: test_tcp_recv_nal.o

test_rtp2h264: test_rtp2h264.o

test_decode_rtp2h264: test_decode_rtp2h264.o

send_h264file_rtp: send_h264file_rtp.o llist_i386.o
	gcc $^ -o $@ -Wall

send_h264file_rtp.o: send_h264file_rtp.c h264tortp.h
	gcc -c $< -o $@ -Wall

llist_i386.o: llist.c
	gcc -c $< -o $@ -Wall

clean:
	@rm -f $(TARGET) *.o

