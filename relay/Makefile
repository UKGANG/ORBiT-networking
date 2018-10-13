

#.DEFAULT_GOAL := BUILD

#compiler
CC := g++
#compiler arguments
CA := -std=c++11 -pthread
#compiler limit errors argument
CL := -fmax-errors=5
#output file
OF := output

BUILD:
	clear
	${CC} ${CA} *.cpp -o ${OF}

BUILD_LIMIT:
	clear
	${CC} ${CA} ${CL} *.cpp -o ${OF}

BUILD_CLEAN: CLEAN BUILD

CLEAN:
