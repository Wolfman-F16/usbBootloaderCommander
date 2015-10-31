LIBUSB_CONFIG   = libusb-config
CFLAGS+=-g -Wall -pedantic `$(LIBUSB_CONFIG) --cflags`
CFLAGS+=-L./
LFLAGS+=`$(LIBUSB_CONFIG) --libs` -lusb-1.0

all:
	make avrusbboot

clean:
	rm *.o
	rm avrusbboot

avrusbboot: main.cpp cflashmem.o cpage.o cbootloader.o
	g++ $(CFLAGS) main.cpp cflashmem.o cpage.o cbootloader.o -o avrusbboot $(LFLAGS)
