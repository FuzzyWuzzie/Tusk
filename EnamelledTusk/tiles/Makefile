CC=g++
CFLAGS=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi
LDFLAGS=-L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -L../libs/ilclient
INCLUDES=-I$(SDKSTAGE)/opt/vc/include/ -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I./ -I../libs/ilclient
DEBUG=
PROFILE=

bin/tiles: o/tiles.o o/eglManager.o o/oglManager.o o/lodepng.o
	$(CC) -o bin/tiles -Wl,--whole-archive $(DEBUG) $(PROFILE) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic o/oglManager.o o/eglManager.o o/lodepng.o o/tiles.o

o/tiles.o: src/tiles.cc
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(PROFILE) -Wno-deprecated-declarations -c -o o/tiles.o src/tiles.cc

o/eglManager.o: src/eglManager.h src/eglManager.cc
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(PROFILE) -Wno-deprecated-declarations -c -o o/eglManager.o src/eglManager.cc
	
o/oglManager.o: src/oglManager.h src/oglManager.cc
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(PROFILE) -Wno-deprecated-declarations -c -o o/oglManager.o src/oglManager.cc
	
o/lodepng.o: src/lodepng.h src/lodepng.cc
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(PROFILE) -Wno-deprecated-declarations -c -o o/lodepng.o src/lodepng.cc
	
.PHONY: run
run:
	./bin/tiles
	
.PHONY: clean
clean:
	rm -v o/* bin/*
