all: hpgl-dxf

hpgl-dxf: hpgl-dxf.c
	gcc -Wall hpgl-dxf.c -o hpgl-dxf

install: hpgl-dxf
	cp hpgl-dxf /usr/local/bin

clean:
	rm hpgl-dxf *.o

