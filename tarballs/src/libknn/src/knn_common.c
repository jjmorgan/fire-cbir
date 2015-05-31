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
 *   Common functions to different K Nearest Neighbours search techniques
 *   
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "knn_api.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// regla de los k-vecinos: la clase más votada es la ganadora ...
//
int DLLAPI knn_rule_simple(t_knn *knn, int k,
			   t_resknn *res_knn, int *n_dif){
  float  sum_inv_dists, ldist;
  int    i,j,found,n_dif_classes;

  char **l_dif_classes;
  int   *l_dif_votes;
  
  float  aux_conf;
  char   aux_label[LIBKNN_LABEL_SIZE];
  int    max_idx;


  // alloc local mem
  l_dif_classes=(char **)malloc(k*sizeof(char*));
  for (i=0; i<k; i++)
    l_dif_classes[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));

  l_dif_votes=(int *)malloc(k*sizeof(float));
  for (i=0; i<k; i++)
    l_dif_votes[i]=0;

  // Recuento de clases diferentes en los k-vecinos y sumatorio de distancias por clase
  n_dif_classes=0;
  for (i=0; i<k; i++) {
    found=0;
    j=-1;
    // buscar si la clase ya ha aparecido antes o no
    while ((j<n_dif_classes)&&(!found)) {
      j++;
      if (!strncmp(knn[i].labelvec,l_dif_classes[j],LIBKNN_LABEL_SIZE))
	found=1;	
    }    
    if (!found) {  // la clase NO había aparecido . . . añadirla e inicializar votos
      strncpy(l_dif_classes[n_dif_classes],knn[i].labelvec, LIBKNN_LABEL_SIZE);
      l_dif_votes[n_dif_classes]++;
      n_dif_classes++;
    } else { // la clase SI había aparecido . . . actualizar votos
      l_dif_votes[j]++;
    }
  }

  // fiabilidades calculadas respecto al conjunto de los k-vecinos encontrados
  for (i=0; i<n_dif_classes; i++ ) {
    strncpy(res_knn[i].labelvec, l_dif_classes[i], LIBKNN_LABEL_SIZE);
    res_knn[i].conf = (float)((float)l_dif_votes[i]/(float)k);
  }

  // ordenar resultados por orden decreciente de fiabilidad
  for (i=0; i<n_dif_classes; i++ ) {
    max_idx=i;
    for (j=i+1; j<n_dif_classes; j++)
      if (res_knn[j].conf>res_knn[max_idx].conf)
	max_idx=j;
    if (i!=max_idx) {
      // swap label
      strncpy(aux_label, res_knn[i].labelvec, LIBKNN_LABEL_SIZE);
      strncpy(res_knn[i].labelvec, res_knn[max_idx].labelvec, LIBKNN_LABEL_SIZE);
      strncpy(res_knn[max_idx].labelvec, aux_label, LIBKNN_LABEL_SIZE);
      // swap conf      
      aux_conf                     = res_knn[i].conf;
      res_knn[i].conf              = res_knn[max_idx].conf;
      res_knn[max_idx].conf        = aux_conf;
    }
  }
  *n_dif=n_dif_classes;
  
  // free local mem
  for (i=0; i<k; i++)
    free(l_dif_classes[i]);
  free(l_dif_classes);
  free(l_dif_votes);
  
  return(ERROR_KNN_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// regla de los k-vecinos modificada para tener en cuenta las distancias:
//
// Suma de todas las inversas de las distancias de los K vecinos encontrados.
// medida de fiabilidad F-2 en el PFC de jarlandis. 
//
//   (SumWm/Sum - 1/K) * ( 1 / (1 - 1/K) ) donde
//   SumWm = Sumatorio(j=wm) (1 / Distancia(j)) para j = clases más votadas
//   Sum = Sumatorio(j->[1,k]) (1 / Distancia(j)) para todo j

int DLLAPI knn_rule(t_knn *knn, int k,
		    t_resknn *res_knn, int *n_dif){
  float  sum_inv_dists, ldist;
  int    i,j,found,n_dif_classes;

  char **l_dif_classes;
  float *l_dif_acc;
  
  float  aux_conf;
  char   aux_label[LIBKNN_LABEL_SIZE];
  int    max_idx;


  // alloc local mem
  l_dif_classes=(char **)malloc(k*sizeof(char*));
  for (i=0; i<k; i++)
    l_dif_classes[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));

  l_dif_acc=(float *)malloc(k*sizeof(float));

  // Recuento de clases diferentes en los k-vecinos y sumatorio de distancias por clase
  n_dif_classes=0;
  for (i=0; i<k; i++) {
    found=0;
    j=-1;
    // buscar si la clase ya ha aparecido antes o no
    while ((j<n_dif_classes)&&(!found)) {
      j++;
      if (!strncmp(knn[i].labelvec,l_dif_classes[j],LIBKNN_LABEL_SIZE)) {
	found=1;	
      }
    }    
    if (!found) {  // la clase NO había aparecido . . . añadirla, inicializar votos y distancias
      strncpy(l_dif_classes[n_dif_classes],knn[i].labelvec, LIBKNN_LABEL_SIZE);
      l_dif_acc[n_dif_classes]=(float)(1.0/knn[i].dist);
      n_dif_classes++;
    } else { // la clase SI había aparecido . . . actualizar votos y sumatorio de distancias
      l_dif_acc[j]+=(float)(1.0/knn[i].dist);
    }
  }

  // Sumar las distancias de los K VECINOS
  sum_inv_dists = 0;
  for (i=0; i<k; i++) {
    if (knn[i].dist > LIBKNN_MAX_DIST)
      ldist = (float)LIBKNN_MAX_DIST;
    else
      ldist = knn[i].dist;
    sum_inv_dists += (float)(1.0/ldist);	
  }
 
  // fiabilidades calculadas respecto al conjunto de los k-vecinos encontrados
  for (i=0; i<n_dif_classes; i++ ) {
    strncpy(res_knn[i].labelvec, l_dif_classes[i], LIBKNN_LABEL_SIZE);
    res_knn[i].conf        = l_dif_acc[i]/sum_inv_dists;
  }
  // practica:
  // La Frel no esta normalizada ya que:
  // fiabilidad = (SumWm/Sum), y NO (SumWm/Sum - 1/K) * ( 1 / (1 - 1/K) )
  // por lo que el resultado estará en el rango: [1/k,1]


  // ordenar resultados por orden decreciente de fiabilidad
  for (i=0; i<n_dif_classes; i++ ) {
    max_idx=i;
    for (j=i+1; j<n_dif_classes; j++)
      if (res_knn[j].conf>res_knn[max_idx].conf)
	max_idx=j;
    if (i!=max_idx) {
      // swap label
      strncpy(aux_label, res_knn[i].labelvec, LIBKNN_LABEL_SIZE);
      strncpy(res_knn[i].labelvec, res_knn[max_idx].labelvec, LIBKNN_LABEL_SIZE);
      strncpy(res_knn[max_idx].labelvec, aux_label, LIBKNN_LABEL_SIZE);
      // swap conf      
      aux_conf                     = res_knn[i].conf;
      res_knn[i].conf              = res_knn[max_idx].conf;
      res_knn[max_idx].conf        = aux_conf;
    }
  }  

  *n_dif=n_dif_classes;

  // free local mem
  for (i=0; i<k; i++)
    free(l_dif_classes[i]);
  free(l_dif_classes);
  free(l_dif_acc);
  
  return(ERROR_KNN_OK);
}

