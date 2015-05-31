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
 * Fichero: knn_kdtree_search.cpp
 * 
 * Fecha: 2003-09
 *
 * Descripción: 
 *   APPROXIMATE & EXACT search on a KD-TREE data structure
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
static int distance_l2_opt(float *point1, float *point2, int dim, float dist_min, 
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

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Squared Euclidean distance: (x1-y1)²+(x2-y2)²+...+(xD-yD)²
static int distance_l2(float *point1, float *point2, int dim,
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
  }
  *d=sum;

  return(ERROR_KNN_OK);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Incremental Distance Calculation + Priority Queue SEARCH ///////

#define DEBUG_PQ 0

int idc_pq_search(t_knn_kdtree *train_str, t_kdtree_node *node, float *point, float *partial_dists, float eps_param, int k, 
		  //
		  int *nn_fvec_idx, int *nn_node_idx, float *nn_dist, int *nn_bucket_idx, int *nn_bucket_size){

  int             i, j, pos, queue_size, max_queue_size, more_nodes;
  float           hyperplane, pointpos, new_dist, old_dist, incremental_dist, new_incremental_dist, eps_dist;
  float          *l_dist;
  float          *dist_queue;
  //  float         **dist_queue;
  t_kdtree_node **node_queue;

  // alloc memory for the vector of distances
  l_dist=(float *)malloc(train_str->max_bucket_size*sizeof(float));

  // alloc & initialize memory for the queues
  max_queue_size = train_str->nvectors * 4; // NNODES = (aprox.) NVECTORS*2
  node_queue = (t_kdtree_node **)malloc(max_queue_size*sizeof(t_kdtree_node *));
  dist_queue =          (float *)malloc(max_queue_size*sizeof(float));
  for (i=0; i<max_queue_size; i++) {
    node_queue[i]=NULL;
    dist_queue[i]=(float)LIBKNN_MAX_DIST;
  }  
  // initialize queues with root node
  queue_size = 0;
  //PQ_SortedList_Insert(
  PQ_Heap_Insert(
		 node, 0.0,
		 node_queue, dist_queue,
		 &queue_size);
  if (DEBUG_PQ) {
    fprintf(stderr,"Insert (%d):",queue_size);
    for (i=0; i<queue_size; i++)
      fprintf(stderr,"\t%.2f", dist_queue[i]);
    fprintf(stderr,"\n");
  }
  // ////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
  // //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // iterative search
  while (queue_size>0) {

    // get the first element of the queue
    //PQ_SortedList_Extract(
    PQ_Heap_Extract(
		    node_queue, dist_queue,
		    &node, &incremental_dist,
		    &queue_size);
    if (DEBUG_PQ) {
      fprintf(stderr,"Extract (%d):",queue_size);
      for (i=0; i<queue_size; i++)
	fprintf(stderr,"\t%.2f", dist_queue[i]);
      fprintf(stderr,"\n");
    }    
    eps_dist = (float)( nn_dist[k-1] / ((1.0+eps_param)*(1.0+eps_param)) ); // search optimization: prune against k-th NN => kNN (real-knn)

    // if the partial distance is bigger than the current full
    // distance, search has finished (the other elements of the queue
    // have bigger distances
    if (incremental_dist>eps_dist)
      break;

    // descend tree
    more_nodes=1;
    while ((node->bucket_size==0) && (more_nodes)) {  // TREE NODE   ??? bucket_size==0?
      //    while (more_nodes) {  // TREE NODE

      if (max_queue_size<queue_size) {
	fprintf(stderr,"libknn:  max_queue_size exceeded !!!\n");
	fflush(stdout);
	exit (-1);
      }

      hyperplane  = node->split_hplane;
      pointpos    = point[node->split_dim];
      new_dist    = pointpos - hyperplane;      

      if (new_dist<0) {  // query point to the "left" of the cutting hyperplane
	if (node->right_kdtree!=NULL) {  // insert new element (right subtree)
	  old_dist = pointpos - node->low_limit;
	  if (old_dist>0) // point inside hyperrectangle area at the cutting dimension
	    old_dist=0;	  
	  // new minimum distance (from query point to subregion) in the current dimension
	  new_incremental_dist = incremental_dist - old_dist*old_dist + new_dist*new_dist;
	  //PQ_SortedList_Insert(
	  PQ_Heap_Insert(
			 node->right_kdtree, new_incremental_dist,
			 node_queue, dist_queue,
			 &queue_size);
	  if (DEBUG_PQ) {
	    fprintf(stderr,"Insert (%d):",queue_size);
	    for (i=0; i<queue_size; i++)
	      fprintf(stderr,"\t%.2f", dist_queue[i]);
	    fprintf(stderr,"\n");
	  }    
	}	
	if (node->left_kdtree!=NULL)  // explore (left subtree)
	  node = node->left_kdtree;
	else
	  more_nodes=0;

      } else {  // query point to the "right"	
	if (node->left_kdtree!=NULL) {  // insert new element
	  old_dist = node->high_limit - pointpos;
	  if (old_dist>0)
	    old_dist=0;
	  new_incremental_dist = incremental_dist - old_dist*old_dist + new_dist*new_dist;
	  //PQ_SortedList_Insert(
	  PQ_Heap_Insert(
			 node->left_kdtree, new_incremental_dist,
			 node_queue, dist_queue,
			 &queue_size);
	  if (DEBUG_PQ) {
	    fprintf(stderr,"Insert (%d):",queue_size);
	    for (i=0; i<queue_size; i++)
	      fprintf(stderr,"\t%.2f", dist_queue[i]);
	    fprintf(stderr,"\n");
	  }
	}		
	if (node->right_kdtree!=NULL)  // explore right subtree
	  node = node->right_kdtree;
	else
	  more_nodes=0;

      } // if (new_dist<0) } else { ...
    } // while ((node->bucket_size==0) && (more_nodes)) {  // TREE NODE

    // check leaf
    if (node->bucket_size>0) { // LEAF NODE, update distance if closer point      

      // //////////////////////////////////////////////////////////////////////////////////////
      for (i=0; i<node->bucket_size; i++) { // compute distances to bucket points
	distance_l2(point, train_str->featvec[node->bucket[i]], train_str->dim,
		    &l_dist[i]);
	
	if (l_dist[i]<nn_dist[k-1]) {	  
	  pos=k-1;
	  for (j=k-2; j>=0; j--)
	    if (l_dist[i]<nn_dist[j])
	      pos=j;
	  
	  for (j=k-1; j>pos; j--){
	    nn_node_idx[j]     = nn_node_idx[j-1];    // node index
	    nn_dist[j]         = nn_dist[j-1];        // minimum distance
	    nn_fvec_idx[j]     = nn_fvec_idx[j-1];    // index to feature vector list
	    nn_bucket_idx[j]   = nn_bucket_idx[j-1];  // index to bucket list
	    nn_bucket_size[j]  = nn_bucket_size[j-1]; // number of points in this bucket
	  }	  	  
	  nn_node_idx[pos]     = node->node_idx;    // node index
	  nn_dist[pos]         = l_dist[i];         // minimum distance
	  nn_fvec_idx[pos]     = node->bucket[i];   // index to feature vector list
	  nn_bucket_idx[pos]   = i;                 // index to bucket list
	  nn_bucket_size[pos]  = node->bucket_size; // number of points in this bucket	  
	}
      }
      // //////////////////////////////////////////////////////////////////////////////////////      

    } // if (node->bucket_size>0) { // LEAF NODE, update distance if closer point 

  } // while (queue_size>0) ...
  // iterative search
  // //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  free(node_queue);
  free(dist_queue);

  free(l_dist);

  //  exit(-1);
  return(ERROR_KNN_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Incremental Distance Calculation + Priority Queue SEARCH
int DLLAPI knn_kdtree_search_idc_pq (t_knn_kdtree *train_str, float *point, int k, float eps,
				     t_knn *knn){
  int            i;
  int            len;
  float         *partial_dists;
  int           *nn_fvec_idx, *nn_node_idx, *nn_bucket_idx, *nn_bucket_size;
  float         *nn_dist;

  nn_fvec_idx = (int *)malloc(k*sizeof(int));
  nn_node_idx = (int *)malloc(k*sizeof(int));
  nn_bucket_idx = (int *)malloc(k*sizeof(int));
  nn_bucket_size = (int *)malloc(k*sizeof(int));
  nn_dist = (float *)malloc(k*sizeof(float));

  partial_dists = train_str->partial_dists;
  // initialize
  for (i=0; i<train_str->dim; i++)
    partial_dists[i]=0.0;

  // select the set of k nearest neighbours
  for (i=0; i<k; i++) {  
    nn_fvec_idx[i]=-1;
    nn_node_idx[i]=-1;
    nn_dist[i]=(float)LIBKNN_MAX_DIST;
    nn_bucket_idx[i]=-1;
    nn_bucket_size[i]=-1;  
  }

  // incremental distance calculation + priority queue
  idc_pq_search(train_str, train_str->root, point, partial_dists, eps, k, 
		nn_fvec_idx, nn_node_idx, nn_dist, nn_bucket_idx, nn_bucket_size);
  // return Nearest Neighbour found
  for (i=0; i<k; i++) {
    len=strlen(train_str->labelvec[nn_fvec_idx[i]]);
    strncpy(knn[i].labelvec,train_str->labelvec[nn_fvec_idx[i]],len);
    knn[i].labelvec[len]='\0';
    knn[i].idx=nn_fvec_idx[i];
    //      for (j=0; j<train_str->dim; j++)
    //        knn[i].featvec[j]=train_str->featvec[nn_fvec_idx[i]][j];
    knn[i].dist=nn_dist[i];
  }

  free(nn_fvec_idx);
  free(nn_node_idx);
  free(nn_bucket_idx);
  free(nn_bucket_size);
  free(nn_dist);

  return(ERROR_KNN_OK);
}




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// RECURSIVE Incremental Distance Calculation SEARCH ////////////////////////
int recursive_idc_search(t_knn_kdtree *train_str, t_kdtree_node *node, 
			 float *point,
			 float *partial_dists, float incremental_dist,
			 float eps_param, int k,
			 //
			 int *nn_fvec_idx, int *nn_node_idx, float *nn_dist, 
			 int *nn_bucket_idx, int *nn_bucket_size,
			 int *prototypes_visited){  

  float eps_dist, pre_dist, new_dist;
  int   i, j, pos;
  float *l_dist;
  

  l_dist=(float *)malloc(train_str->max_bucket_size*sizeof(float));

  if (node->bucket_size>0) { // LEAF, compute distances to bucket points and update nn if necessary
    //    (*leaf_visited)++;

    // //////////////////////////////////////////////////////////////////////////////////////
    for (i=0; i<node->bucket_size; i++) { // compute distances to bucket points

      (*prototypes_visited)++;

      distance_l2(point, train_str->featvec[node->bucket[i]], train_str->dim,
		  &l_dist[i]);

      
      if (l_dist[i]<nn_dist[k-1]) { // save a set of k neighbours
	  	  
	pos=k-1;
	for (j=k-2; j>=0; j--)
	  if (l_dist[i]<nn_dist[j])
	    pos=j;
	  
	for (j=k-1; j>pos; j--){
	  nn_node_idx[j]     = nn_node_idx[j-1];    // node index
	  nn_dist[j]         = nn_dist[j-1];        // minimum distance
	  nn_fvec_idx[j]     = nn_fvec_idx[j-1];    // index to feature vector list
	  nn_bucket_idx[j]   = nn_bucket_idx[j-1];  // index to bucket list
	  nn_bucket_size[j]  = nn_bucket_size[j-1]; // number of points in this bucket
	}	  	  
	nn_node_idx[pos]     = node->node_idx;    // node index
	nn_dist[pos]         = l_dist[i];         // minimum distance
	nn_fvec_idx[pos]     = node->bucket[i];   // index to feature vector list
	nn_bucket_idx[pos]   = i;                 // index to bucket list
	nn_bucket_size[pos]  = node->bucket_size; // number of points in this bucket	
  
      }      

    }      
    // //////////////////////////////////////////////////////////////////////////////////////
   
    /* prueba para la búsqueda exhaustiva ... jcano 2004/06
    int   *idx_min;
    float *dists, dist_min;
    dists=(float *)malloc(train_str->nvectors*sizeof(float));
    idx_min=(int *)malloc(k*sizeof(int));
    dist_min=(float)LIBKNN_MAX_DIST;
    for (i=0; i<node->bucket_size; i++) { // compute distances to bucket points
      distance_l2(point, train_str->featvec[node->bucket[i]], train_str->dim,
		  &l_dist[i]);
      if (l_dist[i]<dist_min) {
	dist_min = l_dist[i];
	idx_min[0] = node->bucket[i];
      }      
    }    
    dist_min=(float)LIBKNN_MAX_DIST;
    for (i=0; i<train_str->nvectors; i++) {
      distance_l2_opt(train_str->featvec[i], point, train_str->dim, dist_min,
		      &dists[i]);    
      if (dists[i]<dist_min) { 
	dist_min = dists[i];
	idx_min[0]=i;
      }    
    }    
    nn_dist[0]         = dist_min;         // minimum distance
    nn_fvec_idx[0]     = idx_min[0];   // index to feature vector list
    free(dists);
    free(idx_min);    
    return(ERROR_KNN_OK);
*/


  } else { // NODE, update incremental distance and explore subtrees

    eps_dist = (float)( nn_dist[k-1] / ((1.0+eps_param)*(1.0+eps_param)) ); // prune against k-th NN => kNN (real-knn)
    //    eps_dist = (float)( nn_dist[0] / ((1.0+eps_param)*(1.0+eps_param)) ); // prune against NN => kNSN (pseudo-knn, IbPRIA'2003, pg. 590)
    pre_dist = partial_dists[node->split_dim];               // IDC
    new_dist = point[node->split_dim] - node->split_hplane;  // IDC

    if (new_dist<0) {  // query point left to the cutting hyperplane
      if (node->left_kdtree!=NULL) {
	recursive_idc_search(train_str, node->left_kdtree, 
			     point,
			     partial_dists, incremental_dist,
			     eps_param, k,
			     nn_fvec_idx, nn_node_idx, nn_dist, 
			     nn_bucket_idx, nn_bucket_size, prototypes_visited);
      }
	/*
      } else {
	fprintf(stderr,"Kagada\n");
      }
      */
      incremental_dist = incremental_dist - pre_dist*pre_dist + new_dist*new_dist;
      if (incremental_dist<eps_dist) {       
	partial_dists[node->split_dim] = new_dist;
	if (node->right_kdtree!=NULL) {
	  recursive_idc_search(train_str, node->right_kdtree, 
			       point,
			       partial_dists, incremental_dist,
			       eps_param, k,
			       nn_fvec_idx, nn_node_idx, nn_dist, 
			       nn_bucket_idx, nn_bucket_size, prototypes_visited);
	}	
	partial_dists[node->split_dim] = pre_dist;
      }
      /**/

    } else { // query point right to the cutting hyperplane
      if (node->right_kdtree!=NULL) {
	recursive_idc_search(train_str, node->right_kdtree, 
			     point, 
			     partial_dists, incremental_dist,
			     eps_param, k,
			     nn_fvec_idx, nn_node_idx, 
			     nn_dist, nn_bucket_idx, nn_bucket_size, prototypes_visited);
      }
	/*
      } else {
	fprintf(stderr,"Kagada\n");
      }
      */
      incremental_dist = incremental_dist - pre_dist*pre_dist + new_dist*new_dist;
      if (incremental_dist<eps_dist) {       
	partial_dists[node->split_dim] = new_dist;
	if (node->left_kdtree!=NULL) {
	  recursive_idc_search(train_str, node->left_kdtree, 
			       point,
			       partial_dists, incremental_dist,
			       eps_param, k,
			       nn_fvec_idx, nn_node_idx, 
			       nn_dist, nn_bucket_idx, nn_bucket_size, prototypes_visited);
	}	
	partial_dists[node->split_dim] = pre_dist;
      }
      /**/

    } // else { // query point right to the cutting hyperplane

  } // else {  // NODE, update incremental distance and explore subtrees

  free(l_dist);

  return(ERROR_KNN_OK);
}

// Incremental Distance Calculation
int DLLAPI knn_kdtree_search_idc (t_knn_kdtree *train_str, float *point, int k, float eps,
				  t_knn *knn){
  int            i;
  float         *partial_dists;
  int            len;
  int            nn_fvec_idx[k], nn_node_idx[k], nn_bucket_idx[k], nn_bucket_size[k];
  float          nn_dist[k];
  float          ini_incremental_dist;

  partial_dists = train_str->partial_dists;

  // initialize
  for (i=0; i<train_str->dim; i++)
    partial_dists[i]=0.0;
  ini_incremental_dist=0.0;

  int prototypes_visited;

  for (i=0; i<k; i++) {  
    nn_fvec_idx[i]=-1;
    nn_node_idx[i]=-1;
    nn_dist[i]=(float)LIBKNN_MAX_DIST;
    nn_bucket_idx[i]=-1;
    nn_bucket_size[i]=-1;  
  }
  // incremental distance calculation
  prototypes_visited=0;
  recursive_idc_search(train_str, train_str->root, 
		       point, 
		       partial_dists, ini_incremental_dist,
		       eps, k, 
		       nn_fvec_idx, nn_node_idx, nn_dist, 
		       nn_bucket_idx, nn_bucket_size,
		       &prototypes_visited);
  //  fprintf(stdout,"libknn: prototypes_visited= %d\n", prototypes_visited);
  //  fprintf(stdout,"(%d) ", prototypes_visited);
  /*
  // parche para evitar "algunos" de los casos en los que el usuario
  // pide una cantidad de vecinos (k) superior al número de hojas
  // visitadas
  if (k>prototypes_visited) {
    fprintf(stderr,"libknn: knn_kdtree_search_idc: Aviso! se reduce el epsilon a 0.0 para conseguir más vecinos!\n");
    prototypes_visited=0;
    eps=0.0;
    recursive_idc_search(train_str, train_str->root, 
			 point, 
			 partial_dists, ini_incremental_dist,
			 eps, k, 
			 nn_fvec_idx, nn_node_idx, nn_dist, 
			 nn_bucket_idx, nn_bucket_size,
			 &prototypes_visited);
  }
  // si aún así no se consiguen los vecinos requeridos, devolver error ...
  if (k>prototypes_visited) {
    fprintf(stderr,"libknn: knn_kdtree_search_idc: Error! se piden más vecinos(%d) que prototipos son visitados(%d)!\n", k, prototypes_visited);
    return(ERROR_KNN_PARAM);
  }
  */

  // return Nearest Neighbour found
  for (i=0; i<k; i++) {
    len=strlen(train_str->labelvec[nn_fvec_idx[i]]);
    strncpy(knn[i].labelvec,train_str->labelvec[nn_fvec_idx[i]],len);
    knn[i].labelvec[len]='\0';
    knn[i].idx=nn_fvec_idx[i];
    knn[i].dist=nn_dist[i];
  }

  return(ERROR_KNN_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
