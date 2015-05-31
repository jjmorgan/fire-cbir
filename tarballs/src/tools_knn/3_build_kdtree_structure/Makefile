# Compilador usado
CC=gcc

# Opciones para el compilador
LOCAL_HEADERS_PATHS= -I../../libknn/include/ 
CFLAGS= -O6 ${LOCAL_HEADERS_PATHS} -I/usr/include
#CFLAGS= -Wall -g -pg ${LOCAL_HEADERS_PATHS} -I/usr/include

# Opciones para el linker
LOCAL_LIB_PATHS= -L../../libknn/lib
SYSTEM_LIB_PATHS= -L/usr/lib -L/usr/local/lib

LIBS=  -lm -lknn
LDFLAGS= ${LOCAL_LIB_PATHS} ${SYSTEM_LIB_PATHS} ${LIBS}   


#-----------------------------------------------------------------------#
# Objetos requeridos:
LOCALOBJ= build_kdtree_structure.o

# Rules:
%.o: %.c %.h 
	${CC} ${CFLAGS} ${CFLAGS2} -c  $<

#-----------------------------------------------------------------------#
# Objetivo principal                                                    #
#  Fichero ejecutable-> build_kdtree_structure                            #
#-----------------------------------------------------------------------#
all: build_kdtree_structure

tool_build_kdtree_structure: build_kdtree_structure

build_kdtree_structure: $(LOCALOBJ)
	@echo Linking... ${TOOL}
	${CC} ${CFLAGS} ${CFLAGS2} -o $@ ${LOCALOBJ} ${LDFLAGS}


#-----------------------------------------------------------------------#
clean: 
	rm -f *.o build_kdtree_structure
#-----------------------------------------------------------------------#
#-----------------------------------------------------------------------#
