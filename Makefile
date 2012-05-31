INCS = -I../libcensure/src -I/usr/include/opencv
LIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc -lcensure -L/home/rochus/workspace/censure/libcensure
#CFLAGS = -ggdb

all:
	g++ -o demo main.cpp -std=c++03 ${INCS} ${LIBS} ${CFLAGS}

clean:
	@rm -f demo
