# This file is part of the FIRE -- Flexible Image Retrieval System
# 
# FIRE is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# FIRE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with FIRE; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

OBJ=objects/$(ARCH)
BIN=bin/$(ARCH)
LIB=lib/$(ARCH)

CXX=g++
CC=g++

COMPILEFLAGS=-O3 -DDEBUG_LEVEL=10
#COMPILEFLAGS=-O3 -DDEBUG_LEVEL=10
#COMPILEFLAGS=-pg -DDEBUG_LEVEL=10
#COMPILEFLAGS=-g -DDEBUG_LEVEL=15
#COMPILEFLAGS=-g -DDEBUG_LEVEL=50

CXXFLAGS=-c $(COMPILEFLAGS) -W -Wall  `Magick++-config --cxxflags --cppflags` $(INTERPOL_FLAGS) $(KDTREE_FLAGS) $(FFT_FLAGS)
#CXXFLAGS=-c $(COMPILEFLAGS) -W -Wall  $(INTERPOL_FLAGS) 
CFLAGS=$(CXXFLAGS)


#----------------------------------------------------------------------
# configure additional libraries
#----------------------------------------------------------------------

#-image magick --------------------------------------------------------
# this is necessary for many things, probably not compilable without
# this library
LDLIBS=-ldl -lz -lf77blas -lcblas -latlas  -llapack -lg2c $(KDTREE_LDLIBS) $(FFT_LIB_LDLIBS) `Magick++-config --libs` 
LDFLAGS=$(KDTREE_LDFLAGS)  `Magick++-config --ldflags` 
#----------------------------------------------------------------------

#-interpolation library------------------------------------------------
# this is necessary for extraction of invariant feature histograms
# downloaded from Philippe Thevenaz
# 
# if this library is not available, bilinear interpolation with
# wraparound is used
INTERPOL_FLAGS=-DHAVE_INTERPOL_LIBRARY -Ilib/$(ARCH)/interpol
INTERPOL_COEFF_H=lib/$(ARCH)/interpol/coeff.h
INTERPOL_INTERPOL_H=lib/$(ARCH)/interpol/interpol.h
INTERPOL_OBJECTS=lib/$(ARCH)/interpol/interpol.o lib/$(ARCH)/interpol/coeff.o
#---------------------------------------------------------------------- 

#-kdtree library-------------------------------------------------------
# this library is necessary for local feature based image retrieval
# the library used here was written by Javi Cano (jcano@iti.upv.es)
# 
#KDTREE_FLAGS=-DHAVE_KDTREE_LIBRARY -I lib/$(ARCH)/knn_kdtrees/src/libknn/include
#KDTREE_KNN_H=knn_api.h
#KDTREE_LDFLAGS=-Llib/$(ARCH)/knn_kdtrees/src/libknn/lib
#KDTREE_LDLIBS=-lknn
#---------------------------------------------------------------------- 

#-fft library----------------------------------------------------------
# this library is needed for gabor feature extraction
# if not available, the corresponding classes wont do anything useful
#
#FFT_FLAGS=-DHAVE_FFT_LIBRARY 
#FFT_LIB_LDLIBS=-lfftw 
#----------------------------------------------------------------------

APPS=$(BIN)/fire 
EXTRACTFEATURES=$(BIN)/extractcolorhistogram $(BIN)/colororgray				 \
    $(BIN)/extracttamuratexturefeature $(BIN)/extractinvariantfeaturehistogram \
    $(BIN)/extractlocalfeatures $(BIN)/createlfkdtree			 \
	$(BIN)/extractgaborfeaturevector $(BIN)/extractglobaltexturefeature		 \
	$(BIN)/extractaspectratio  
PROGS=$(APPS) $(EXTRACTFEATURES)

sinclude .depend 
.depend: 
	@($(CXX) $(CXXFLAGS) -MM *.cpp | sed 's-^\(.*\.o\)-$(OBJ)/\1-'> .depend)
	@(echo "[CXX] .depend")

all: $(PROGS)

$(BIN)/colororgray: $(OBJ)/colororgray.o $(OBJ)/imagefeature.o $(OBJ)/diag.o $(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/basefeature.o 

$(BIN)/fire: $(OBJ)/fire.o $(OBJ)/diag.o $(OBJ)/server.o $(OBJ)/net.o $(OBJ)/retriever.o $(OBJ)/imagecontainer.o		   \
		$(OBJ)/database.o $(OBJ)/gzstream.o										   \
		$(OBJ)/featureloader.o $(OBJ)/vectorfeature.o $(OBJ)/distancemaker.o $(OBJ)/imagefeature.o \
		$(OBJ)/imagecomparator.o											   \
		$(OBJ)/histogramfeature.o $(OBJ)/imagelib.o $(OBJ)/dist_idm.o $(OBJ)/interpolatingimage.o  \
		$(OBJ)/localfeatures.o												   \
		$(INTERPOL_OBJECTS) $(OBJ)/dist_globallocalfeaturedistance.o		   \
		$(OBJ)/runprogram.o $(OBJ)/largefeaturefile.o $(OBJ)/basefeature.o				   \
		$(OBJ)/dist_mpeg7.o $(OBJ)/mpeg7feature.o $(OBJ)/getscoring.o $(OBJ)/maxentscoring.o	   \
		$(OBJ)/sparsehistogramfeature.o $(OBJ)/maxentscoringsecondorder.o			   \
		$(OBJ)/maxentscoringfirstandsecondorder.o $(OBJ)/hungarian.o $(OBJ)/dist_textfeature.o


$(BIN)/extractcolorhistogram: $(OBJ)/extractcolorhistogram.o $(OBJ)/imagefeature.o $(OBJ)/histogramfeature.o $(OBJ)/diag.o  \
						$(OBJ)/vectorfeature.o	 $(OBJ)/gzstream.o $(OBJ)/basefeature.o

$(BIN)/extractsparsegaborhistogram: $(OBJ)/extractsparsegaborhistogram.o $(OBJ)/createsparsehisto.o $(OBJ)/basefeature.o $(OBJ)/sparsehistogramfeature.o $(OBJ)/gzstream.o $(OBJ)/diag.o $(OBJ)/gabor.o $(OBJ)/jflib.o $(OBJ)/imagefeature.o $(OBJ)/vectorfeature.o $(OBJ)/imagelib.o $(OBJ)/colorhsv.o $(OBJ)/interpolatingimage.o $(INTERPOL_OBJECTS)

$(BIN)/extractgaborfeaturevector: $(OBJ)/extractgaborfeaturevector.o $(OBJ)/gabor.o $(OBJ)/imagefeature.o $(OBJ)/imagelib.o $(OBJ)/vectorfeature.o $(OBJ)/basefeature.o $(OBJ)/gzstream.o $(OBJ)/colorhsv.o $(OBJ)/interpolatingimage.o \
	$(INTERPOL_OBJECTS) $(OBJ)/diag.o $(OBJ)/jflib.o

$(BIN)/extractglobaltexturefeature: $(OBJ)/extractglobaltexturefeature.o $(OBJ)/imagefeature.o $(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/diag.o \
		$(OBJ)/basefeature.o $(OBJ)/imagelib.o $(INTERPOL_OBJECTS) $(OBJ)/interpolatingimage.o $(OBJ)/globalfeatureextraction.o

$(BIN)/extracttamuratexturefeature: $(OBJ)/extracttamuratexturefeature.o $(OBJ)/tamurafeature.o $(OBJ)/imagefeature.o $(OBJ)/histogramfeature.o $(OBJ)/diag.o \
		$(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/imagelib.o $(OBJ)/interpolatingimage.o $(INTERPOL_OBJECTS) $(OBJ)/basefeature.o

$(BIN)/extractinvariantfeaturehistogram: $(OBJ)/extractinvariantfeaturehistogram.o $(OBJ)/imagefeature.o $(OBJ)/histogramfeature.o $(OBJ)/diag.o \
        $(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/invariantfeaturehistogram.o $(OBJ)/interpolatingimage.o $(INTERPOL_OBJECTS) $(OBJ)/basefeature.o \
        $(OBJ)/kernelfunctionmaker.o $(OBJ)/imagelib.o $(OBJ)/basefeature.o

$(BIN)/extractrelationalfeaturehistogram: $(OBJ)/extractrelationalfeaturehistogram.o $(OBJ)/imagefeature.o $(OBJ)/histogramfeature.o $(OBJ)/diag.o \
        $(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/relationalfeaturehistogram.o $(OBJ)/interpolatingimage.o $(INTERPOL_OBJECTS) $(OBJ)/basefeature.o \
        $(OBJ)/kernelfunctionmaker.o $(OBJ)/imagelib.o $(OBJ)/vectorfeature.o

$(BIN)/extractlocalfeatures: $(OBJ)/extractlocalfeatures.o $(OBJ)/imagefeature.o $(OBJ)/diag.o $(OBJ)/localfeatures.o $(OBJ)/gzstream.o $(OBJ)/imagelib.o \
		$(INTERPOL_OBJECTS) $(OBJ)/interpolatingimage.o $(OBJ)/vectorfeature.o $(OBJ)/pca.o $(OBJ)/svd.o $(OBJ)/Lapack.o $(OBJ)/salientpoints.o $(OBJ)/wavelet.o $(OBJ)/basefeature.o $(OBJ)/sift.o $(OBJ)/colorhsv.o

$(BIN)/extractaspectratio: $(OBJ)/extractaspectratio.o $(OBJ)/imagefeature.o $(OBJ)/diag.o $(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/basefeature.o

$(BIN)/createlfkdtree: $(OBJ)/createlfkdtree.o $(OBJ)/localfeatures.o $(OBJ)/imagefeature.o $(OBJ)/diag.o $(OBJ)/gzstream.o $(OBJ)/vectorfeature.o $(OBJ)/basefeature.o

$(BIN)/salientpoints: $(OBJ)/imagefeature.o $(OBJ)/imagelib.o $(INTERPOL_OBJECTS) $(OBJ)/vectorfeature.o $(OBJ)/gzstream.o $(OBJ)/wavelet.o $(OBJ)/salientpoints.o\
	$(OBJ)/calcsalientpoints.o $(OBJ)/interpolatingimage.o $(OBJ)/diag.o $(OBJ)/basefeature.o

$(BIN)/db2lff: $(OBJ)/db2lff.o $(OBJ)/database.o $(OBJ)/gzstream.o $(OBJ)/imagefeature.o $(OBJ)/imagecontainer.o $(OBJ)/vectorfeature.o $(OBJ)/diag.o $(OBJ)/localfeatures.o $(OBJ)/featureloader.o $(OBJ)/histogramfeature.o $(OBJ)/jflib.o $(OBJ)/largefeaturefile.o $(OBJ)/basefeature.o $(OBJ)/sparsehistogramfeature.o $(OBJ)/mpeg7feature.o

$(OBJ)/%.o: %.cpp Makefile
	@(echo "  [CXX $(COMPILEFLAGS)] $@ ")
	@($(CXX) -o $@ $(CXXFLAGS) -c $<)

$(OBJ)/hungarian.o: 
	gcc -o $@ $(CXXFLAGS) -c hungarian.c

hungarian.cpp: hungarian.c
	cp hungarian.c hungarian.cpp


$(BIN)/%: $(OBJ)/%.o
	@(echo "  [LD] $@: $^")
	@($(CXX)  $(LDFLAGS)  -o $@ $^ $(LDLIBS))


clean: 
	rm -f .depend $(OBJ)/*.o *~ $(APPS) $(EXTRACTFEATURES) 

newdepend:
	rm  .depend
	make .depend

directories:
	mkdir -p $(OBJ)
	mkdir -p $(BIN)
	mkdir -p $(LIB)
info:
	@(echo $(ARCH))
	@(echo $(OBJ))
	@(echo $(BIN))
