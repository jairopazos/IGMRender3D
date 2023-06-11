todo: spinningcube_withlight

LDLIBS=-lGL -lGLEW -lglfw -lm 

spinningcube_withlight: spinningcube_withlight.cpp textfile.c
	g++ -o spinningcube_withlight spinningcube_withlight.cpp textfile.c -lGL -lGLEW -lglfw -lm

clean:
	rm -f *.o *~

cleanall: clean
	rm -f spinningcube_withlight
