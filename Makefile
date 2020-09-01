WindowMapIntersect: WindowMapIntersect.cc
	g++ -o WindowMapIntersect WindowMapIntersect.cc -lSDL2 -lSDL2_image  -g

clean:
	rm WindowMapIntersect