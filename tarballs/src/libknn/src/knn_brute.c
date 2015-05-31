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
 * Fichero: knn_brute.cpp
 * 
 * Fecha: 2003-05
 *
 * Descripción: 
 *   BRUTE FORCE search on a simple LIST data structure
 *   
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "knn_api.h"


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Squared Euclidean distance: (x1-y1)²+(x2-y2)²+...+(xD-yD)²
static int distance_l2(float *point1, float *point2, int dim, float dist_min, 
                       float *d){
  register float   sum;
  register float   dif;
  register int     i;
  register float  *p1;
  register float  *p2;

  sum=0.0;
  p1 = &(point1[0]);
  p2 = &(point2[0]);
  for (i=0; i<dim; i++){
    dif=(( *p1 - *p2 ) * ( *p1 - *p2 ));
    p1++;
    p2++;
    sum+=dif;
    if (sum>dist_min) {
      *d=LIBKNN_MAX_DIST;
      return(ERROR_KNN_OK);
    }
  }
  *d=sum;

  return(ERROR_KNN_OK);
}

static int distance_l2_eps(float *point1, float *point2, int dim, 
			   float *d, float *eps_ndim){
  register float   sum;
  register float   dif;
  register int     i;
  register float  *p1;
  register float  *p2;

  sum=0.0;
  p1 = &(point1[0]);
  p2 = &(point2[0]);
  for (i=0; i<dim; i++){
    dif=(( *p1 - *p2 ) * ( *p1 - *p2 ));
    p1++;
    p2++;
    sum+=dif;
    eps_ndim[i]=dif;
  }
  *d=sum;

  return(ERROR_KNN_OK);
}



int inserta_min(int new_idx, int k, float *dists, 
		int *idx_min) {
  int i, pos;
  float dist, new_dist;

  new_dist=dists[new_idx];

  // busca la posición que le corresponde
  pos=0;
  if (idx_min[pos]==-1)
    dist=(float)LIBKNN_MAX_DIST;
  else 
    dist=dists[idx_min[pos]];  
  while ((pos<k-1) && (dist<new_dist)) {
    pos++;
    if (idx_min[pos]==-1)
      dist=(float)LIBKNN_MAX_DIST;
    else 
      dist=dists[idx_min[pos]];    
  }

  // desplaza los vecinos actuales a partir de esa posicion
  for (i=k-1; i>pos; i--)
    idx_min[i]=idx_min[i-1];
  // inserta el nuevo vecino
  idx_min[pos]=new_idx;

  return(ERROR_KNN_OK);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


//  optimización trivial de la búsqueda exhaustiva: 
//  ordenar los prototipos por sus distancias a la media de los prototipos.

int DLLAPI knn_list_create(t_fvec  *ptrain_vec, 
			   t_fvec  *ptrain_str){
  float *max, *min, *mean;;
  int    i,j;


  max = (float *)malloc(ptrain_vec->dim*sizeof(float));
  min = (float *)malloc(ptrain_vec->dim*sizeof(float));
  mean = (float *)malloc(ptrain_vec->dim*sizeof(float));

  for (j=0; j<ptrain_vec->dim; j++) {
    max[j] = -LIBKNN_MAX_DIST;
    min[j] = +LIBKNN_MAX_DIST;
  }

  for (i=0; i<ptrain_vec->nvectors; i++)
    for (j=0; j<ptrain_vec->dim; j++) {
      if (ptrain_vec->featvec[i][j]>max[j]) max[j]=ptrain_vec->featvec[i][j];
      if (ptrain_vec->featvec[i][j]<min[j]) min[j]=ptrain_vec->featvec[i][j];
      mean[j] += ptrain_vec->featvec[i][j];
    }

  for (j=0; j<ptrain_vec->dim; j++)
    mean[j] /= ptrain_vec->nvectors;

  
  for (j=0; j<ptrain_vec->dim; j++) 
    fprintf(stderr,"dim %d:\t[%.2f,%.2f]\t%.2f\n",j,min[j],max[j],(max[j]-min[j]));
    

  fprintf(stderr,"mean vector: ");
  for (j=0; j<ptrain_vec->dim; j++) 
    fprintf(stderr,"%.2f ", mean[j]);
  fprintf(stderr,"\n");
    



  float *dists;
  int *local_idx;
  dists=(float *)malloc(ptrain_vec->nvectors*sizeof(float));
  local_idx=(int *)malloc(ptrain_vec->nvectors*sizeof(int));

  // calcular la distancia del punto "mean" a todos los puntos del train
  fprintf(stderr,"\n");
  float dist_min=(float)LIBKNN_MAX_DIST;
  for (i=0; i<ptrain_vec->nvectors; i++) {
    distance_l2(ptrain_vec->featvec[i], mean, ptrain_vec->dim, dist_min,
		&dists[i]);    
    local_idx[i]     = i;
    if (i%1000==0) fprintf(stderr,".");
  }
  fprintf(stderr,"\n");

  // bubble sort
  int aux_int;
  float aux_float;
  for (i=0; i<ptrain_vec->nvectors; i++) {
    for (j=i+1; j<ptrain_vec->nvectors; j++)
      if (dists[i]>dists[j]) {       
	// swap value
	aux_float = dists[i];
	dists[i]  = dists[j];
	dists[j]  = aux_float;
	// swap index
	aux_int      = local_idx[i];
	local_idx[i] = local_idx[j];
	local_idx[j] = aux_int;
      }
    if (i%1000==0) fprintf(stderr,".");
  }
  fprintf(stderr,"\n");

  // mostrar lista de pares (indice,distancia)
/*    fprintf(stderr,"distancias ordenadas: "); */
/*    for (i=0; i<ptrain_vec->nvectors; i++) */
/*      fprintf(stderr,"(%d,\t%.2f)\n", local_idx[i], dists[i]); */

  for (i=0; i<ptrain_vec->nvectors; i++) {
    for (j=0; j<ptrain_vec->dim; j++)
      ptrain_str->featvec[i][j] = ptrain_vec->featvec[local_idx[i]][j];
    strncpy(ptrain_str->labelvec[i], ptrain_vec->labelvec[local_idx[i]], LIBKNN_LABEL_SIZE);
  }




  free(mean);
  free(max);
  free(min);

  return(ERROR_KNN_OK);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int DLLAPI knn_list_save(t_fvec *ptrain_str, 
			 char *str_filename){

  FILE *f;
  int nvec,dim;
  
  // open file
  if ( ( f = fopen ( str_filename, "w" ) ) == NULL ) {
    fprintf(stderr,"Error creando fichero: %s\n", str_filename);
    return(ERROR_KNN_FILE);
  }

  // volcar dimensión y número vectores
  fprintf(f,"DIM %d\n", ptrain_str->dim);
  fprintf(f,"NVEC %d\n", ptrain_str->nvectors);
  for (nvec=0; nvec<ptrain_str->nvectors; nvec++) {
    for (dim=0; dim<ptrain_str->dim; dim++)
      fprintf(f,"%.15g ", ptrain_str->featvec[nvec][dim]);
    fprintf(f,"%s", ptrain_str->labelvec[nvec]);
    fprintf(f,"\n");
  }

  // close file
  fclose(f);

  return(ERROR_KNN_OK);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int DLLAPI knn_list_read(char *filename,
			 t_fvec *train_str){
  char           *ck;
  char            pszLine[LIBKNN_MAX_LONG_LINE], pszLinea[LIBKNN_MAX_LONG_LINE], *pszToken;
  FILE           *f;

  int             i,j,nvec,dim;
  float           comp;

  // open file
  if ( ( f = fopen ( filename, "r" ) ) == NULL ) {
    fprintf(stderr,"Error abriendo fichero %s\n", filename);
    return(ERROR_KNN_FILE);
  }
  // read vector dimension
  fgets(pszLinea, LIBKNN_MAX_LONG_LINE, f );
  strcpy (pszLine, pszLinea);
  if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {
    if ( !strcmp ( pszToken, "DIM" ) ) { // lee dimension de los vectores
        if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((train_str->dim = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	   ) {
	  fprintf(stderr, "Error, value of parameter DIM incorrect");
	  return -1;
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
	    ((train_str->nvectors = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	   ) {
	  fprintf(stderr, "Error, value of parameter NVEC incorrect");
	  return -1;
	}
    }
  }
  // read VECTORS
  train_str->labelvec=(char **)malloc(train_str->nvectors*sizeof(char *));
  for (i=0; i<train_str->nvectors; i++){
    train_str->labelvec[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
    for (j=0; j<LIBKNN_LABEL_SIZE; j++)
      train_str->labelvec[i][j]='\0';
  }
  train_str->featvec=(float **)malloc(train_str->nvectors*sizeof(float *));
  for (i=0; i<train_str->nvectors; i++)
    train_str->featvec[i]=(float *)malloc(train_str->dim*sizeof(float));
  nvec=0;
  while ( fgets ( pszLine, LIBKNN_MAX_LONG_LINE, f ) ) {
    strcpy (pszLinea, pszLine);
    if ( (pszToken = strtok( pszLinea, " " )) != NULL ){
      comp=(float)strtod(pszToken, &ck);
      train_str->featvec[nvec][0]=(float)comp;
    }     
    for (dim=1; dim<train_str->dim; dim++)
      if ( (pszToken = strtok( NULL, " " )) != NULL ) {	    
	comp=(float)strtod(pszToken, &ck);
	train_str->featvec[nvec][dim]=(float)comp;
      }
    if ( (pszToken = strtok( NULL, " " )) != NULL ){
      strncpy(train_str->labelvec[nvec],pszToken,LIBKNN_LABEL_SIZE);
    }     
    nvec++;
  } // while ( fgets 
  if (nvec!=train_str->nvectors) {
    fprintf(stderr,"nº de vectores no coincide con especificación de la cabecera\n");
    exit(-1);
  }
  // close file
  fclose(f);



  return(ERROR_KNN_OK);
}


int DLLAPI knn_list_search(t_fvec *train_str, float *point, int k, float eps,
			   t_knn *knn){
  //			   t_knn *knn, float *point_eps){
  int    i,j,RC;
  int   *idx_min;
  float *dists, dist_min;

  dists=(float *)malloc(train_str->nvectors*sizeof(float));
  idx_min=(int *)malloc(k*sizeof(int));

  // calcular la distancia del punto de test a todos los puntos del train
  dist_min=(float)LIBKNN_MAX_DIST;
  for (i=0; i<train_str->nvectors; i++) {
    RC=distance_l2(train_str->featvec[i], point, train_str->dim, dist_min,
		   &dists[i]);    
    if (RC) { 
      fprintf(stderr,"Error calculando distancia . . .");
      return(ERROR_KNN_PARAM);
    }    
    if (k==1) // para k=1 se optimiza abortando el cálculo de la
      // distancia a un prototipo cuando su distancia parcial ya supera la
      // mejor distancia encontrada hasta el momento
      if (dists[i]<dist_min) { 
	// 	dist_min = dists[i];
	dist_min = (dists[i]/(1.0+eps));
	idx_min[0]=i;
      }    
  }

  // buscar los k más cercanos
  if (k>1) {    // para k=1 se optimiza guardando el indice durante el cálculo de distancias (idx_min[0] ya se ha establecido)
    dist_min=(float)LIBKNN_MAX_DIST;
    for (i=0; i<k; i++)
      idx_min[i]=-1;
    for (i=0; i<train_str->nvectors; i++){
      if (dists[i]<dist_min) {
	//      fprintf(stderr,"inserta %d (dists[i]=%.2f dist_min=%.2f)\n", i, dists[i], dist_min);
	inserta_min(i, k, dists, 
		    idx_min);
	if (idx_min[k-1]!=-1) 
	  dist_min=dists[idx_min[k-1]];
	else 
	  dist_min=(float)LIBKNN_MAX_DIST;
	//        for (j=0; j<k; j++)
	//  	fprintf(stderr,"idx_min[%d]=%d  dists[idx_min[%d]]=%.2f\n", j, idx_min[j], j, dists[idx_min[j]]);

      }
    }    
  }


  // guardar los k puntos mas cercanos
  for (i=0; i<k; i++){
    knn[i].idx=idx_min[i];
    knn[i].dist=dists[idx_min[i]];
    for (j=0; j<train_str->dim; j++)
      knn[i].featvec[j]=train_str->featvec[idx_min[i]][j];   
    strncpy(knn[i].labelvec,train_str->labelvec[idx_min[i]],LIBKNN_LABEL_SIZE);   
    //    fprintf(stderr,"K%d: dist=%.2f idx_min[%d]=%d\n", i, knn[i].dist, i, idx_min[i]);
  }


  /*    RC=distance_l2_eps(knn[k-1].featvec, point, train_str->dim, */
  /*  		     &dists[0], point_eps); */



  // free local mem
  free(dists);
  free(idx_min);

  return(ERROR_KNN_OK);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/*
int DLLAPI knn_list_search_old(t_fvec *train_str, float *point, int k,
			       t_knn *knn){
  int    i,j,idx_min,RC;
  float *dists, dist_min;

  // alloc local mem
  dists=(float *)malloc(train_str->nvectors*sizeof(float));

  // calcular la distancia del punto de test a todos los puntos del train
  for (i=0; i<train_str->nvectors; i++) {
    RC=distance_l2(train_str->featvec[i], point, train_str->dim,
		   &dists[i]);
    if (RC) { 
      fprintf(stderr,"Error calculando distancia . . .");
      return(ERROR_KNN_PARAM);
    }
  }

  // seleccionar los k puntos mas cercanos
  for (i=0; i<k; i++){

    // buscar el más cercano
    dist_min=(float)LIBKNN_MAX_DIST;
    idx_min=-1;
    for (j=0; j<train_str->nvectors; j++){
      if (dists[j]<dist_min) {
	dist_min=dists[j];
	idx_min=j;
      }
    }

    // guardarlo
    knn[i].dist=dists[idx_min];
    dists[idx_min]=(float)LIBKNN_MAX_DIST;
    for (j=0; j<train_str->dim; j++)
      knn[i].featvec[j]=train_str->featvec[idx_min][j];   
    strncpy(knn[i].labelvec,train_str->labelvec[idx_min],LIBKNN_LABEL_SIZE);   
  }

  // free local mem
  free(dists);

  return(ERROR_KNN_OK);
}
*/
