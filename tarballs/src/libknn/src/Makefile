#-----------------------------------------------------------------------------
# Makefile for core library
#
# libknn: K-Nearest Neighbours library
#-----------------------------------------------------------------------------
INCDIR       = -I../include -I../../libitipnm/include
LIBDIR       = ../lib
SOURCES      = knn_common.c knn_file.c \
	       knn_brute.c \
	       knn_kdtree.c knn_kdtree_search.c knn_kdtree_pqueue.c
OBJECTS      = $(SOURCES:.c=.o)
CC           = g++

#-----------------------------------------------------------------------------
# Rules
#-----------------------------------------------------------------------------
.c.o:
	$(CC) -c $(INCDIR) $(INCDIR_EXTRA) $(CFLAGS) $<

#-----------------------------------------------------------------------------
# Make the library
#-----------------------------------------------------------------------------
default:
	@echo "Specify a target configuration"

targets: $(LIBDIR)/$(DR_LIB)

# crea la libreria estática y la copia a lib/
$(LIBDIR)/$(DR_LIB): $(OBJECTS)
	$(MAKELIB) $(DR_LIB) $(OBJECTS)
	$(RANLIB) $(DR_LIB)
	mv $(DR_LIB) $(LIBDIR)

#-----------------------------------------------------------------------------
# Configuration definitions
#-----------------------------------------------------------------------------
include ../Make-config

#-----------------------------------------------------------------------------
# Cleaning
#-----------------------------------------------------------------------------
clean:
	-rm -f *.o core
