all: 3d_modeling

3d_modeling: 3d_modeling.c
	gcc -fopenmp -o 3d_modeling 3d_modeling.c  -lglut -lGLU -lGL `pkg-config opencv --cflags` `pkg-config opencv --libs`
