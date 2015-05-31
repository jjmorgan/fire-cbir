# Compilador usado
CC=gcc
#CC=ccmalloc g++

# Opciones para el compilador
LOCAL_HEADERS_PATHS= -I../../libknn/include/
CFLAGS= -O6 ${LOCAL_HEADERS_PATHS} -I/usr/include
#CFLAGS= -Wall -g ${LOCAL_HEADERS_PATHS} -I/usr/include
#CFLAGS= -Wall -g -pg ${LOCAL_HEADERS_PATHS} -I/usr/include

# Opciones para el linker
LOCAL_LIB_PATHS= -L../../libknn/lib
SYSTEM_LIB_PATHS= -L/usr/lib -L/usr/local/lib

LIBS= -lm -lknn
#LIBS= -lm -lknn -lefence
LDFLAGS= ${LOCAL_LIB_PATHS} ${SYSTEM_LIB_PATHS} ${LIBS}   


#-----------------------------------------------------------------------#
# Objetos requeridos:
LOCALOBJ= test_knn_kdtree.o

# Rules:
%.o: %.c %.h 
	${CC} ${CFLAGS} ${CFLAGS2} -c  $<

#-----------------------------------------------------------------------#
# Objetivo principal                                                    #
#  Fichero ejecutable-> test_knn_kdtree                                 #
#-----------------------------------------------------------------------#
all: test_knn_kdtree

tool_test_knn_kdtree: test_knn_kdtree

test_knn_kdtree: $(LOCALOBJ)
	@echo Linking... ${TOOL}
	${CC} ${CFLAGS} ${CFLAGS2} -o $@ ${LOCALOBJ} ${LDFLAGS}

#-----------------------------------------------------------------------#
clean: 
	rm -f *.o test_knn_kdtree
#-----------------------------------------------------------------------#
#-----------------------------------------------------------------------#
