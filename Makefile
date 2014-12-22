all:
	rm -f graf1.run
	g++ -lGL -lglut -lGLU -Wall graf_1.cpp -o graf1.run
	./graf1.run

# i3 conf
# for_window [title="^Grafika hazi feladat$"] floating enable
