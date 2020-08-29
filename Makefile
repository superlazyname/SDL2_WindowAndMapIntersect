WindowMapIntersect: WindowMapIntersect.cc
	gcc -o WindowMapIntersect WindowMapIntersect.cc -lSDL2 -lSDL2_image  -g

clean:
	rm WindowMapIntersect