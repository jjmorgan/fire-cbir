/*
 *  <libknn> is a library that implements different data structures
 *  and search methods for fast approximate nearest neighbour queries
 *  Copyright (C) 2003 J.Cano, J.C.Perez, ...
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */

/***************************************************************************
 *
 * Librería para la implementación de la Búsqueda de los K-Vecinos Más Cercanos
 * Desarrollado por el grupo de PRHLT del Instituto Tecnológico de Informática
 * Universidad Politécnica de Valencia.                         
 *
 * Fichero: knn_file.cpp
 * 
 * Fecha: 2003-09
 *
 * Descripción: 
 *   Common functions to access data files
 *   
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "knn_api.h"

int DLLAPI knn_data_read(char *filename, t_fvec *train_vec) {

  char           *ck;
  char            pszLine[LIBKNN_MAX_LONG_LINE], pszLinea[LIBKNN_MAX_LONG_LINE], *pszToken;
  FILE           *f;
  int             i,j, dim, len, nvec=0;
  float           comp;

  // /////////////////////////////////////////////////////////////////////////////////////
  // Alloc data structure mem
  //train_vec = (t_fvec *)malloc(sizeof(t_fvec));
  // /////////////////////////////////////////////////////////////////////////////////////


  // /////////////////////////////////////////////////////////////////////////////////////
  // LEER VECTORES
  // open file
  if ( ( f = fopen ( filename, "r" ) ) == NULL ) {
    fprintf(stderr,"Error abriendo fichero de vectores: %s\n", filename);
    exit (-1);
  }

  // read vector dimension
  fgets(pszLinea, LIBKNN_MAX_LONG_LINE, f );
  strcpy (pszLine, pszLinea);
  if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {
    if ( !strcmp ( pszToken, "DIM" ) ) { // lee dimension de los vectores
        if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((train_vec->dim = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	   ) {
	  fprintf(stderr, "Error, value of parameter DIM incorrect");
	  //	  return -1;
	}
    }
  }
  // read number of vectors
  fgets(pszLinea, LIBKNN_MAX_LONG_LINE, f );
  strcpy (pszLine, pszLinea);
  if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {
    if ( !strcmp ( pszToken, "NVEC" ) ) { // lee nº de vectores
        if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((train_vec->nvectors = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	   ) {
	  fprintf(stderr, "Error, value of parameter NVEC incorrect");
	  //	  return -1;
	}
    }
  }

  // read VECTORS
  train_vec->labelvec=(char **)malloc(train_vec->nvectors*sizeof(char *));
  for (i=0; i<train_vec->nvectors; i++){
    train_vec->labelvec[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
    for (j=0; j<LIBKNN_LABEL_SIZE; j++)
      train_vec->labelvec[i][j]='\0';
  }
  train_vec->featvec=(float **)malloc(train_vec->nvectors*sizeof(float *));
  for (i=0; i<train_vec->nvectors; i++)
    train_vec->featvec[i]=(float *)malloc(train_vec->dim*sizeof(float));

  nvec=0;
  while ( fgets ( pszLine, LIBKNN_MAX_LONG_LINE, f ) ) {
    strcpy (pszLinea, pszLine);
    if ( (pszToken = strtok( pszLinea, " " )) != NULL ){
      comp=strtod(pszToken, &ck);
      train_vec->featvec[nvec][0]=comp;
    }     
    for (dim=1; dim<train_vec->dim; dim++)
      if ( (pszToken = strtok( NULL, " " )) != NULL ) {	    
	comp=strtod(pszToken, &ck);
	train_vec->featvec[nvec][dim]=comp;
      }
    if ( (pszToken = strtok( NULL, " " )) != NULL ){
      len=strlen(pszToken);
      strncpy(train_vec->labelvec[nvec],pszToken,len-1);
      train_vec->labelvec[nvec][len]='\0';
    }     
    nvec++;
  } // while ( fgets 
  // close file
  fclose(f);

  if (nvec!=train_vec->nvectors) {
    fprintf(stderr,"nº de vectores no coincide con especificación de la cabecera\n");
    exit(-1);
  }

  return(ERROR_KNN_OK);
}                          

int DLLAPI knn_data_free(t_fvec *train_vec){
  int i;

  for (i=0; i<train_vec->nvectors; i++){
    free(train_vec->featvec[i]);
    free(train_vec->labelvec[i]);
  }

  free(train_vec->labelvec);
  free(train_vec->featvec);

 return(ERROR_KNN_OK);
}
