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
 * Fichero: knn_kdtree.cpp
 * 
 * Fecha: 2003-05
 *
 * Descripción: 
 *   APPROXIMATE search on a KD-TREE data structure
 *   
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include<time.h>
#include <ctype.h>
#include "knn_api.h"

#define DEBUG_TREE_SPLITDIMPIVOT   0
#define DEBUG_BAD_SET              0
#define DEBUG_TREE_READ            0

#define DEBUG_CREATE_PROGRESS      0

int g_read_node_counter;


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int find_min_max_dim(t_fvec *ptrain_vec, int dim, unsigned char *lr_point,
		     float *min_val, float *max_val){
  int i;

  *min_val=LIBKNN_HUGE_POSITIVE;
  *max_val=LIBKNN_HUGE_NEGATIVE;
  for (i=0; i<ptrain_vec->nvectors; i++){
    if (lr_point[i]==LIBKNN_TRUE) {
      if (DEBUG_TREE_SPLITDIMPIVOT) 
	fprintf(stderr,"ptrain_vec->featvec[i][dim] = %.2f\n", ptrain_vec->featvec[i][dim]);
      if (ptrain_vec->featvec[i][dim]<(*min_val))
	*min_val=ptrain_vec->featvec[i][dim];
      if (ptrain_vec->featvec[i][dim]>(*max_val))
	*max_val=ptrain_vec->featvec[i][dim];
    }
  }

  return(ERROR_KNN_OK);
}

int compute_variance(t_fvec *ptrain_vec, int *lr_point, int nvectors, int dim,
		     float *variance){
  float avg, var, dif;
  int   i;

  avg=0.0;
  for (i=0; i<nvectors; i++){
    avg+=ptrain_vec->featvec[lr_point[i]][dim];
  }  
  avg/=(float)nvectors;
  if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"avg = %.4f\n", avg);
  
  var=0.0;
  for (i=0; i<nvectors; i++){
    dif=ptrain_vec->featvec[lr_point[i]][dim]-avg;
    var+=(dif*dif);
  }  
  var/=(float)nvectors;
  if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"var = %.4f\n", var);

  *variance=var;

  return(ERROR_KNN_OK);
}


//int compare_floats (const float *a, const float *b) {
int compare_floats (const void *a, const void *b) {
  //  float temp = *a - *b;
  float temp = *(float *)a - *(float *)b;
  if (temp > 0)
    return 1;
  else if (temp < 0)
    return -1;
  else
    return 0;
}

int knn_choose_pivot_maxdispersion(t_fvec *ptrain_vec, unsigned char *lr_point,
				   int *index_lr_point, int nvectors,
				   float *split_hplane, int *split_dim, float *min_val, float *max_val){
  int    i_dim,count;
  float *variance;
  float  max_var;
  int    i, j;
  float *list_val;

  variance = (float *)malloc(ptrain_vec->dim*sizeof(float));
  for (i_dim=0; i_dim<ptrain_vec->dim; i_dim++)
    variance[i_dim]=0.0;
  // compute variance for every dimension (in the set of points still not inserted in the kdtree -> lr_point)
  for (i_dim=0; i_dim<ptrain_vec->dim; i_dim++) {
    compute_variance(ptrain_vec, index_lr_point, nvectors, i_dim,
		     &variance[i_dim]);
    if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"variance[%d] = %.4f\n", i_dim, variance[i_dim]);
  }
  // CHOOSE the SPLIT DIMENSION which maximizes variance . . . 
  max_var=variance[0];
  *split_dim=0;
  for (i_dim=1; i_dim<ptrain_vec->dim; i_dim++) {
    if (variance[i_dim]>max_var) {
      max_var=variance[i_dim];
      *split_dim=i_dim;
    }
  }
  if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"split_dim = %d\n", *split_dim);
  if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"variance[%d] = %.2f\n", *split_dim, variance[*split_dim]);
  free(variance);

  // FIND MIN & MAX VALUES AT THE SPLIT DIMENSION
  find_min_max_dim(ptrain_vec, *split_dim, lr_point,
		   min_val, max_val);
  if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"min = %.2f\n", *min_val);
  if (DEBUG_TREE_SPLITDIMPIVOT) fprintf(stderr,"max = %.2f\n", *max_val);

  // CHOOSE the median as the SPLIT HPLANE
  // copy points to a local list
  count=0;
  for (i=0; i<ptrain_vec->nvectors; i++){
    if (lr_point[i]==LIBKNN_TRUE) 
      count++;
  }
  list_val=(float *)malloc(count*sizeof(float));
  j=0;
  for (i=0; i<ptrain_vec->nvectors; i++){
    if (lr_point[i]==LIBKNN_TRUE) {
      list_val[j]=ptrain_vec->featvec[i][*split_dim];
      j++;
    }      
  }
  for (i=0; i<count; i++)
    if (DEBUG_TREE_SPLITDIMPIVOT) 
      fprintf(stderr,"list_val[%d]=%.2f\n",i,list_val[i]);


  // sort the list
  //  bubblesort(list_val,0,count-1);
  //  quicksort(list_val,0,count-1);
  qsort(list_val, count, sizeof(float),
	compare_floats);  

  if (DEBUG_TREE_SPLITDIMPIVOT) 
    for (i=0; i<count; i++)
      fprintf(stderr,"list_val[%d]=%.2f\n",i,list_val[i]);

  // choose median
  *split_hplane =list_val[(int)(count/2)];
  if (DEBUG_TREE_SPLITDIMPIVOT) 
    fprintf(stderr,"median: split_hplane = %.2f (count/2=%d)\n",*split_hplane,(int)(count/2));
  
  free(list_val);

  return(ERROR_KNN_OK);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int knn_kdtree_create_rec(t_fvec *ptrain_vec, unsigned char *lr_point, int *node_count, int max_bucket_size,
			  t_kdtree_node *this_node){

  int            i,j,count,RC;
  //  int            point_idx[LIBKNN_MAX_BUCKET_SIZE];
  int            *point_idx;
  int            split_dim;
  unsigned char *marked_left_point, *marked_right_point;
  float          split_hplane,point_pos;
  float          min_val,max_val;
  int            n_left,n_right;

  t_kdtree_node *left_node;
  t_kdtree_node *right_node;


  //  point_idx = (int *)malloc(max_bucket_size*sizeof(int));
  point_idx = (int *)malloc(ptrain_vec->nvectors*sizeof(int));


  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"---------------------------------------------------------------------\n");
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"knn_kdtree_create_rec: node_count=%d\n",*node_count);

  // Stop recursivity when there is only LIBKNN_MAX_BUCKET_SIZE or less points in the local list -> LEAF    
  count=0;
  for (i=0; i<ptrain_vec->nvectors; i++)
    if (lr_point[i]==LIBKNN_TRUE) {
      //      if (count<max_bucket_size)
      point_idx[count]=i;
      count++;
    }
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"knn_kdtree_create_rec: count=%d\n",count);
  //  fprintf(stderr,"knn_kdtree_create_rec: count=%d\n",count);

  if (count>max_bucket_size) {

    // CHOOSE a PIVOT point &
    // the SPLIT DIMENSION
    RC = knn_choose_pivot_maxdispersion(ptrain_vec, lr_point,
					point_idx, count,
					&split_hplane, &split_dim, &min_val, &max_val);    
  
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. SPLIT_DIM=%d\n", split_dim);
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. SPLIT_HPLANE=%.4f\n\n", split_hplane);
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. MIN_VAL=%.4f\n\n", min_val);
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. MAX_VAL=%.4f\n\n", max_val);

    // divide LIBKNN_NOT_SELECTED points into 'left' or 'right' points
    marked_left_point=(unsigned char *)malloc(ptrain_vec->nvectors*sizeof(unsigned char));
    marked_right_point=(unsigned char *)malloc(ptrain_vec->nvectors*sizeof(unsigned char));
    for (i=0; i<ptrain_vec->nvectors; i++) {
      marked_left_point[i]=LIBKNN_FALSE;
      marked_right_point[i]=LIBKNN_FALSE;
    }
    for (i=0; i<ptrain_vec->nvectors; i++) {
      if (lr_point[i]==LIBKNN_TRUE) {
	point_pos = ptrain_vec->featvec[i][split_dim];
	if (point_pos<split_hplane)
	  marked_left_point[i]=LIBKNN_TRUE;
	else
	  marked_right_point[i]=LIBKNN_TRUE;
      }
    }

    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"LEFT CHILDREN:\n");
    n_left=0;
    for (i=0; i<ptrain_vec->nvectors; i++)
      if (marked_left_point[i]) {
	if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"(%d, %.2f)\n", i, ptrain_vec->featvec[i][split_dim]);
	n_left++;
      }
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"\n");
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"RIGHT CHILDREN:\n");
    n_right=0;
    for (i=0; i<ptrain_vec->nvectors; i++)
      if (marked_right_point[i]) {
	if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"(%d, %.2f)\n", i, ptrain_vec->featvec[i][split_dim]);
	n_right++;
      }
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"\n");
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. TREE NODE\n");
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"---------------------------------------------------------------------\n");

    // fill node fields
    this_node->node_idx      = *node_count;
    (*node_count)++;
    if (DEBUG_CREATE_PROGRESS)
      if (*node_count%1000==0)
	fprintf(stderr,".(%d)",*node_count);
    this_node->bucket_size   = 0;

    this_node->split_dim     = split_dim;
    this_node->split_hplane  = split_hplane;
    this_node->low_limit     = min_val;
    this_node->high_limit    = max_val;

    if ((n_left+n_right)<=1) {
      fprintf(stderr,"knn_kdtree_create_rec: Error (n_left+n_right)<=1 AND (count>1) !! ?? !!\n");
      return(ERROR_KNN_PARAM);

    } else if ((n_left>0) && (n_right>0)) {  // (recursively construct kdtree)
      // left chidren
      left_node = (t_kdtree_node *)malloc(1*sizeof(t_kdtree_node));
      left_node->up_kdtree = this_node;
      RC = knn_kdtree_create_rec(ptrain_vec, marked_left_point, node_count, max_bucket_size,
				 left_node);
      this_node->left_kdtree   = left_node;

      // right chidren
      right_node = (t_kdtree_node *)malloc(1*sizeof(t_kdtree_node));
      right_node->up_kdtree = this_node;
      RC = knn_kdtree_create_rec(ptrain_vec, marked_right_point, node_count, max_bucket_size,
				 right_node);
      this_node->right_kdtree  = right_node;

    } else { // split_hplane and all point_pos have the same hplane value
      // leave this set of points . . .  (no elegant solution, but statistically acceptable ¿??)
      right_node = NULL;
      this_node->right_kdtree  = right_node;
      left_node = NULL;
      this_node->left_kdtree   = left_node;

      if (DEBUG_BAD_SET) fprintf(stderr,"knn_kdtree_create_rec: BAD_SET (n_left=%d n_right=%d, this_node->node_idx=%d)\n", n_left, n_right, this_node->node_idx);
      for (i=0; i<ptrain_vec->nvectors; i++) {
	if (DEBUG_BAD_SET) fprintf(stderr,"knn_kdtree_create_rec: BAD_SET: (split_dim=%d)\n", split_dim);
	if (marked_left_point[i]==LIBKNN_TRUE) {
	  if (DEBUG_BAD_SET) fprintf(stderr,"knn_kdtree_create_rec: BAD_SET: fvec_idx=%d vector-left: ", i);
	  for (j=0; j<ptrain_vec->dim; j++) {
	    if (DEBUG_BAD_SET) fprintf(stderr,"%.2f ",ptrain_vec->featvec[i][j]);
	  }
	  if (DEBUG_BAD_SET) fprintf(stderr,"\n");
	}
	if (marked_right_point[i]==LIBKNN_TRUE) {
	  if (DEBUG_BAD_SET) fprintf(stderr,"knn_kdtree_create_rec: BAD_SET: fvec_idx=%d vector-right: ", i);
	  for (j=0; j<ptrain_vec->dim; j++) {
	    if (DEBUG_BAD_SET) fprintf(stderr,"%.2f ",ptrain_vec->featvec[i][j]);
	  }
	  if (DEBUG_BAD_SET) fprintf(stderr,"\n");
	}
      }
      
      
    }

    if (VERBOSE_KNN_KDTREE_TABLE) 
      fprintf(stderr,"knn_kdtree_create_rec: TREE_NODE %d %d  %d %.2f  %.2f %.2f  %d %d  -\n", 
	      this_node->node_idx, this_node->bucket_size, 
	      this_node->split_dim, this_node->split_hplane, 
	      this_node->low_limit, this_node->high_limit, 	      
	      this_node->left_kdtree->node_idx, this_node->right_kdtree->node_idx);   

    // free mem
    free(marked_left_point);
    free(marked_right_point);

    //  } else if ((count<=LIBKNN_MAX_BUCKET_SIZE) && (count>=1)) {
  } else if ((count<=max_bucket_size) && (count>=1)) {
    this_node->node_idx     = *node_count;
    (*node_count)++;
    if (DEBUG_CREATE_PROGRESS)
      if (*node_count%1000==0)
	fprintf(stderr,".(%d)",*node_count);

    this_node->bucket_size  = count;
    this_node->bucket       = (int *)malloc(count*sizeof(int));
    for (i=0; i<count; i++)
      this_node->bucket[i]  = point_idx[i];    

    this_node->split_dim    = -1; // ..... LEAF node
    this_node->split_hplane = 0.0; // ..... LEAF node
    this_node->low_limit    = 0.0; // ..... LEAF node
    this_node->high_limit   = 0.0; // ..... LEAF node
    this_node->left_kdtree  = NULL;
    this_node->right_kdtree = NULL;    
    if (VERBOSE_KNN_KDTREE_TABLE) {
      fprintf(stderr,"knn_kdtree_create_rec: LEAF_NODE %d %d ", 
	      this_node->node_idx, this_node->bucket_size);
      fprintf(stderr," (fvec_idx: "); 
      for (i=0; i<this_node->bucket_size; i++)
	fprintf(stderr," %d ", this_node->bucket[i]); 
      fprintf(stderr,")"); 
      fprintf(stderr," %d %.2f  %.2f %.2f  %d %d  %d\n", 
	      this_node->split_dim, this_node->split_hplane, 
	      this_node->low_limit, this_node->high_limit, 	      
	      -1, -1, 
	      this_node->up_kdtree->node_idx);   
    }
    
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. BUCKET_SIZE=%d\n", this_node->bucket_size);
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"rec. LEAF NODE\n");
    if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"---------------------------------------------------------------------\n");

  } else { //  (count==0)
    fprintf(stderr,"knn_kdtree_create_rec: Error: Empty Leaf !!");
    return(ERROR_KNN_PARAM);
  }  


  free(point_idx);

  return(ERROR_KNN_OK);
}

int DLLAPI knn_kdtree_create(t_fvec   *ptrain_vec, int max_bucket_size,
			     t_knn_kdtree *ptrain_str){
  int            i,j,RC;
  int            split_dim;
  float          split_hplane,point_pos;
  float          min_val, max_val;

  unsigned char *marked_lr;
  unsigned char *marked_left_point;
  unsigned char *marked_right_point;

  t_kdtree_node *left_node;
  t_kdtree_node *right_node;
  t_kdtree_node *root_node;

  int            node_count;


  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"*********************************************************************\n");
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"*********************************************************************\n");


  if (ptrain_vec->nvectors<=0) {
    fprintf(stderr,"libknn error: no vectors supplied ...\n");
    return (ERROR_KNN_PARAM);
  }

  // alloc & initialize mem
  marked_lr=(unsigned char *)malloc(ptrain_vec->nvectors*sizeof(unsigned char));
  for (i=0; i<ptrain_vec->nvectors; i++)
    marked_lr[i]=LIBKNN_TRUE;

  int *point_idx = (int *)malloc(ptrain_vec->nvectors*sizeof(int));
  for (i=0; i<ptrain_vec->nvectors; i++)
    point_idx[i]=i;
  
  // 1 - CHOOSE a PIVOT point from the whole prototype reference set &
  // the SPLITTING DIMENSION
  // initialization
  RC = knn_choose_pivot_maxdispersion(ptrain_vec, marked_lr,
				      point_idx, ptrain_vec->nvectors,
				      &split_hplane, &split_dim, &min_val, &max_val);  
  free(point_idx);

  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"LOW_LIMIT=%.2f\n", min_val);
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"HIGH_LIMIT=%.2f\n", max_val);
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"SPLIT_DIM=%d\n", split_dim);
  // 2 - divide NOT_SELECTED points into 'left' or 'right' points
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"SPLIT_HPLANE=%.4f\n", split_hplane);
  // divide points
  marked_left_point=(unsigned char *)malloc(ptrain_vec->nvectors*sizeof(unsigned char));
  marked_right_point=(unsigned char *)malloc(ptrain_vec->nvectors*sizeof(unsigned char));
  for (i=0; i<ptrain_vec->nvectors; i++) {
    marked_left_point[i]=LIBKNN_FALSE;
    marked_right_point[i]=LIBKNN_FALSE;
    point_pos = ptrain_vec->featvec[i][split_dim];
    if (point_pos<split_hplane)
      marked_left_point[i]=LIBKNN_TRUE;
    else
      marked_right_point[i]=LIBKNN_TRUE;
  }

  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"\n");
  for (i=0; i<ptrain_vec->nvectors; i++)
    if (marked_left_point[i])
      if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"left[%d]=%.2f\n", i, ptrain_vec->featvec[i][split_dim]);
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"\n");
  for (i=0; i<ptrain_vec->nvectors; i++)
    if (marked_right_point[i])
      if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"right[%d]=%.2f\n", i, ptrain_vec->featvec[i][split_dim]);
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"\n");
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr," - ROOT NODE - \n");
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"---------------------------------------------------------------------\n");


  // fill root node fields
  root_node = (t_kdtree_node *)malloc(1*sizeof(t_kdtree_node));
  node_count=0;
  root_node->node_idx      = node_count;
  node_count++;
  root_node->up_kdtree     = NULL;


  // 3a - recursively build 'left' kdtree
  left_node = (t_kdtree_node *)malloc(1*sizeof(t_kdtree_node));
  left_node->up_kdtree     = root_node;
  RC = knn_kdtree_create_rec(ptrain_vec, marked_left_point, &node_count, max_bucket_size,
			     left_node);

  // 3b - recursively build 'right' kdtree
  right_node = (t_kdtree_node *)malloc(1*sizeof(t_kdtree_node));
  right_node->up_kdtree    = root_node;
  RC = knn_kdtree_create_rec(ptrain_vec, marked_right_point, &node_count, max_bucket_size,
			     right_node);

  // 4 - fill root node fields
  root_node->bucket_size   = 0;
  root_node->split_dim     = split_dim;
  root_node->split_hplane  = split_hplane;
  root_node->low_limit     = min_val;
  root_node->high_limit    = max_val;
  root_node->left_kdtree   = left_node;
  root_node->right_kdtree  = right_node;
  if (VERBOSE_KNN_KDTREE_TABLE) 
    fprintf(stderr,"knn_kdtree_create_rec: ROOT_NODE %d %d  %d %.2f %.2f %.2f  %d %d  -\n", 
	    root_node->node_idx, root_node->bucket_size, 
	    root_node->split_dim, root_node->split_hplane, root_node->low_limit, root_node->high_limit,
	    root_node->left_kdtree->node_idx, root_node->right_kdtree->node_idx);     

  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"**************************************\n");
  if (VERBOSE_KNN_KDTREE_CREATE) fprintf(stderr,"**************************************\n");

  // 5 - return whole kdtree
  // alloc mem
  ptrain_str->featvec=(float **)malloc(ptrain_vec->nvectors*sizeof(float *));
  ptrain_str->labelvec=(char **)malloc(ptrain_vec->nvectors*sizeof(char *));
  for (i=0; i<ptrain_vec->nvectors; i++){
    ptrain_str->featvec[i]=(float *)malloc(ptrain_vec->dim*sizeof(float));
    ptrain_str->labelvec[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
    for (j=0; j<LIBKNN_LABEL_SIZE; j++)
      ptrain_str->labelvec[i][j]='\0';
  }
  // fill fields
  ptrain_str->dim             = ptrain_vec->dim;
  ptrain_str->nvectors        = ptrain_vec->nvectors;
  ptrain_str->nnodes          = node_count;
  ptrain_str->max_bucket_size = max_bucket_size;
  for (i=0; i<ptrain_vec->nvectors; i++) {
    for (j=0; j<ptrain_vec->dim; j++)
      ptrain_str->featvec[i][j]=ptrain_vec->featvec[i][j];
    strncpy(ptrain_str->labelvec[i],ptrain_vec->labelvec[i],LIBKNN_LABEL_SIZE);
  }
  ptrain_str->root           = root_node;    

  // printar estructura
//    fprintf(stderr,"ptrain_str->dim=%d\n", ptrain_str->dim);
//    fprintf(stderr,"ptrain_str->nvectors=%d\n", ptrain_str->nvectors);
//    for (i=0; i<ptrain_vec->nvectors; i++) {
//      fprintf(stderr,"ptrain_str->featvec[i] = ");
//      for (j=0; j<ptrain_vec->dim; j++)
//        fprintf(stderr,"%.2f ", ptrain_str->featvec[i][j]);
//      fprintf(stderr,"\n");
//      fprintf(stderr,"ptrain_str->labelvec[i]=%s\n", ptrain_str->labelvec[i]);
//    }
//    fprintf(stderr,"kdtree\n\n");


  ptrain_str->kdtree_vector = (t_kdtree_node *)malloc(ptrain_str->nnodes*sizeof(t_kdtree_node));  

  ptrain_str->partial_dists = (float *)malloc(ptrain_vec->dim*sizeof(float));
  
  // free mem
  free(marked_left_point);
  free(marked_right_point);

  return(ERROR_KNN_OK);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int knn_kdtree_save_node(FILE *f, t_knn_kdtree *kdtree, t_kdtree_node *node, int binary){
  int node_type, bucket_size, node_idx, split_dim, lnode, rnode, unode;
  float split_hplane, low_limit, high_limit;
  char str_node_type[10];
  int  i;
    
  if (node->bucket_size==0) {

    node_type    = LIBKNN_TREE_NODE;
    strncpy(str_node_type,"TREE_NODE\0",10);

    node_idx     = node->node_idx;
    bucket_size  = 0;
    split_dim    = node->split_dim;
    split_hplane = node->split_hplane;
    low_limit    = node->low_limit;
    high_limit   = node->high_limit;      

    if (node->left_kdtree==NULL) 
      lnode = -1;
    else
      lnode = node->left_kdtree->node_idx;

    if (node->right_kdtree==NULL) 
      rnode = -1;
    else
      rnode = node->right_kdtree->node_idx;

    if (node->up_kdtree==NULL) 
      unode = -1;
    else
      unode = node->up_kdtree->node_idx;

  } else {

    node_type    = LIBKNN_LEAF_NODE;
    strncpy(str_node_type,"LEAF_NODE\0",10);

    node_idx     = node->node_idx;
    bucket_size  = node->bucket_size;
    split_dim    = -1;
    split_hplane = 0.0;
    low_limit    = 0.0;
    high_limit   = 0.0;      
    lnode        = -1;
    rnode        = -1;

    if (node->up_kdtree==NULL) 
      unode = -1;
    else
      unode = node->up_kdtree->node_idx;
  }

  // /////////////////////////////////////////////////////

  if (binary) {
    fwrite ( &node_type, sizeof(int), 1, f );

    fwrite ( &node_idx, sizeof(int), 1, f );   
    fwrite ( &bucket_size, sizeof(int), 1, f );   
    for (i=0; i<bucket_size; i++)
      fwrite ( &(node->bucket[i]), sizeof(int), 1, f );   

    fwrite ( &split_dim, sizeof(int), 1, f );   
    fwrite ( &split_hplane, sizeof(float), 1, f );   
    
    fwrite ( &low_limit, sizeof(float), 1, f );   
    fwrite ( &high_limit, sizeof(float), 1, f );   
    
    fwrite ( &lnode, sizeof(int), 1, f );   
    fwrite ( &rnode, sizeof(int), 1, f );   
    fwrite ( &unode, sizeof(int), 1, f );   
   
  } else {    
    fprintf(f,"%s  %d %d ", 
	    str_node_type, 
	    node_idx, bucket_size);
    for (i=0; i<bucket_size; i++)
      fprintf(f," %d ", node->bucket[i]);
    fprintf(f," %d %.2f  %.2f %.2f  %d %d %d\n", 
	    split_dim, node->split_hplane, 
	    node->low_limit, node->high_limit, 
	    lnode, rnode, unode);
  }

  // save descendant nodes, if any ...
  if (node->bucket_size==0) {
    if (lnode!=-1) knn_kdtree_save_node(f, kdtree, node->left_kdtree, binary);
    if (rnode!=-1) knn_kdtree_save_node(f, kdtree, node->right_kdtree, binary);
  }

  return (ERROR_KNN_OK);
}

int DLLAPI knn_kdtree_save(t_knn_kdtree *kdtree, int max_bucket_size, int binary,
			   char *str_filename){

  int            i,j,label_len;
  t_kdtree_node *root_node;
  FILE          *f;

  ////////////////////////////////////////////////////////////////
  if (binary) { // BINARY

    // open file
    if ( ( f = fopen ( str_filename, "wb" ) ) == NULL ) {
      fprintf(stderr,"Error creando fichero: %s\n", str_filename);
      return(ERROR_KNN_FILE);
    }  

    // save data points . . .
    fwrite ( &(kdtree->dim), sizeof(int), 1, f );
    fwrite ( &(kdtree->nvectors), sizeof(int), 1, f );
    for (i=0; i<kdtree->nvectors; i++) {
      for (j=0; j<kdtree->dim; j++)
	fwrite ( &(kdtree->featvec[i][j]), sizeof(float), 1, f );
      label_len = strlen(kdtree->labelvec[i])+1;
      fwrite (&label_len, sizeof(unsigned int), 1, f);
      fwrite (kdtree->labelvec[i], sizeof(char), label_len, f);

    }
    // save kdtree structure 
    fwrite ( &(kdtree->nnodes), sizeof(int), 1, f );   
    fwrite ( &(kdtree->max_bucket_size), sizeof(int), 1, f );   
    root_node = kdtree->root;    
    // . . . recursive tree exploration
    knn_kdtree_save_node(f, kdtree, root_node, binary);

  ////////////////////////////////////////////////////////////////
  } else { // ASCII

    // open file
    if ( ( f = fopen ( str_filename, "w" ) ) == NULL ) {
      fprintf(stderr,"Error creando fichero: %s\n", str_filename);
      return(ERROR_KNN_FILE);
    }  

    // print data points . . .
    fprintf(f,"DIM %d\n", kdtree->dim);
    fprintf(f,"NVEC %d\n", kdtree->nvectors);
    for (i=0; i<kdtree->nvectors; i++) {
      for (j=0; j<kdtree->dim; j++)
	fprintf(f,"%.15g ", kdtree->featvec[i][j]);
      fprintf(f,"%s\n", kdtree->labelvec[i]);
    }
    // print kdtree structure . . .
    fprintf(f,"KDTREE (NODE_IDX BUCKET_SIZE <BUCKET_POINTS> SPLIT_DIM SPLIT_HPLANE LOW_LIMIT HIGH_LIMIT LEFT_NODE_IDX RIGHT_NODE_IDX UP_NODE_IDX)\n");
    fprintf(f,"NNODES %d\n", kdtree->nnodes);
    fprintf(f,"MAX_BUCKET_SIZE %d\n", kdtree->max_bucket_size);
    root_node = kdtree->root;    
    // . . . recursive tree exploration
    knn_kdtree_save_node(f, kdtree, root_node, binary);

  }


  fclose(f);

  return(ERROR_KNN_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int knn_kdtree_free_node(t_kdtree_node *node){

  if (node->bucket_size==0) { // recursive exploration if TREE NODE
    if (node->left_kdtree!=NULL)
      knn_kdtree_free_node(node->left_kdtree);
    if (node->right_kdtree!=NULL) 
      knn_kdtree_free_node(node->right_kdtree);
    //    free(node);

  } else { // LEAF NODE
    free(node->bucket);
    //    free(node);    
  }

  return (ERROR_KNN_OK);
}

int DLLAPI knn_kdtree_free(t_knn_kdtree *kdtree){
  int i;

  // free k-d-tree nodes . . . recursive tree exploration
  knn_kdtree_free_node(kdtree->root);
  free(kdtree->kdtree_vector);    


  // free feature vectors
  for (i=0; i<kdtree->nvectors; i++)
    free(kdtree->featvec[i]);
  free(kdtree->featvec);

  // free feature vector labels
  for (i=0; i<kdtree->nvectors; i++)
    free(kdtree->labelvec[i]);
  free(kdtree->labelvec);

  // free auxiliary search vectors
  free(kdtree->partial_dists);  

  return(ERROR_KNN_OK);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int knn_kdtree_read_nodes(t_kdtree_node *kdtree_vector, 
			  t_kdtree_table *kdtree_table, int nnodes){
  int i,j;
  t_kdtree_node *node;

  for (i=0; i<nnodes; i++) {
    node = &(kdtree_vector[i]);  
    node->node_idx = kdtree_table[i].node_idx;
    if (kdtree_table[i].node==LIBKNN_TREE_NODE) { // TREE_NODE
      node->bucket_size  = 0;
      node->split_dim    = kdtree_table[i].split_dim;
      node->split_hplane = kdtree_table[i].split_hplane;
      node->low_limit    = kdtree_table[i].low_limit;
      node->high_limit   = kdtree_table[i].high_limit;
      // recursive left exploration
      if (kdtree_table[i].left_idx==-1) {  // branch end
	node->left_kdtree            = NULL;
      } else { // left branch
	node->left_kdtree            = &(kdtree_vector[kdtree_table[i].left_idx]);	
	node->left_kdtree->up_kdtree = node; // set backward link
      }
      // recursive right exploration
      if (kdtree_table[i].right_idx==-1) {  // branch end
	node->right_kdtree = NULL;
      } else { // right branch
	node->right_kdtree            = &(kdtree_vector[kdtree_table[i].right_idx]);
	node->right_kdtree->up_kdtree = node; // set backward link
      }
    } else { // LEAF_NODE
      node->bucket_size  = kdtree_table[i].bucket_size;
      node->bucket       = (int *)malloc(node->bucket_size*sizeof(int));
      for (j=0; j<node->bucket_size; j++)
	node->bucket[j]  = kdtree_table[i].bucket[j];
      node->split_dim    = -1;
      node->split_hplane = 0.0;
      node->low_limit    = 0.0;
      node->high_limit   = 0.0;
      node->right_kdtree = NULL;
      node->left_kdtree  = NULL;      
    }
  } // for (i=0; i<nnodes; i++) {
  // /////////////////////////////////////////////////////////////////////////////

  return(ERROR_KNN_OK);
}


int DLLAPI knn_kdtree_read(char *filename, int binary,
			   t_knn_kdtree *train_str){

  char           *ck;
  char            pszLine[LIBKNN_MAX_LONG_LINE], pszLinea[LIBKNN_MAX_LONG_LINE], *pszToken;
  FILE           *f;

  int             i,j,nvec,dim,label_len;
  int             len;
  float           comp;

  t_kdtree_table *kdtree_table;

  int             node_type;

  int             k_v_idx;
  int            *kdtree_vector_idx;
  t_kdtree_node  *kdtree_vector;  


  ////////////////////////////////////////////////////////////////
  if (binary) { // BINARY

    if (DEBUG_TREE_READ) fprintf(stderr, " 1 ");

    // open file
    if ( ( f = fopen ( filename, "rb" ) ) == NULL ) {
      fprintf(stderr,"Error abriendo fichero %s\n", filename);
      return(ERROR_KNN_FILE);
    }

    if (DEBUG_TREE_READ) fprintf(stderr, " 2 ");

    // read vector dimension
    fread ( &(train_str->dim), sizeof(int), 1, f );
    // read number of vectors
    fread ( &(train_str->nvectors), sizeof(int), 1, f );


    // read VECTORS
    //  fprintf(stderr, "reading POINTS  . . . \n");
    train_str->labelvec=(char **)malloc(train_str->nvectors*sizeof(char *));
    for (i=0; i<train_str->nvectors; i++){
      train_str->labelvec[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
      for (j=0; j<LIBKNN_LABEL_SIZE; j++)
	train_str->labelvec[i][j]='\0';
    }
    train_str->featvec=(float **)malloc(train_str->nvectors*sizeof(float *));
    for (i=0; i<train_str->nvectors; i++)
      train_str->featvec[i]=(float *)malloc(train_str->dim*sizeof(float));

    for (i=0; i<train_str->nvectors; i++) {

      for (j=0; j<train_str->dim; j++)
        fread( &(train_str->featvec[i][j]), sizeof(float), 1, f);
      //      fread( train_str->featvec[i], sizeof(float), train_str->dim, f);

      fread( &label_len, sizeof(int), 1, f);
      fread( train_str->labelvec[i], sizeof(char), label_len, f );
    } 
    //    fprintf(stderr,"DIM %d\n", train_str->dim);
    //    fprintf(stderr,"NVEC %d\n", train_str->nvectors);
    //    for (i=0; i<train_str->nvectors; i++) {
    //      for (j=0; j<train_str->dim; j++)
    //        fprintf(stderr,"%.16f ", train_str->featvec[i][j]);
    //      fprintf(stderr,"%s\n", train_str->labelvec[i]);
    //    }

    if (DEBUG_TREE_READ) fprintf(stderr, " 3 ");



    // read KDTREE structure
    fread( &(train_str->nnodes), sizeof(int), 1, f );
    fread( &(train_str->max_bucket_size), sizeof(int), 1, f );

    // LEER LA TABLA DEL KDTREE
    // (NODE_TYPE NODE_IDX BUCKET_SIZE <BUCKET_POINTS> SPLIT_DIM SPLIT_HPLANE LOW_LIMIT HIGH_LIMIT 
    //  LEFT_NODE_IDX RIGHT_NODE_IDX UP_NODE_IDX)
    kdtree_table = (t_kdtree_table *)malloc(train_str->nnodes*sizeof(t_kdtree_table));
    for (i=0; i<train_str->nnodes; i++) {
      fread( &node_type, sizeof(int), 1, f );
      if (node_type==LIBKNN_TREE_NODE)
	kdtree_table[i].node = LIBKNN_TREE_NODE;
      else
	kdtree_table[i].node = LIBKNN_LEAF_NODE;

      fread( &(kdtree_table[i].node_idx), sizeof(int), 1, f );

      fread( &(kdtree_table[i].bucket_size), sizeof(int), 1, f );
/*        //      if (kdtree_table[i].bucket_size>LIBKNN_MAX_BUCKET_SIZE) { */
/*        if (kdtree_table[i].bucket_size>max_bucket_size) { */
/*  	fprintf(stderr,"libknn: knn_kdtree_read: Error! se ha intentado leer un kdtree con un BUCKET_SIZE (nodo %d, bucket_size=%d) mayor del definido en la librería (LIBKNN_MAX_BUCKET_SIZE=%d)\n", i, kdtree_table[i].bucket_size, LIBKNN_MAX_BUCKET_SIZE); */
/*  	return(ERROR_KNN_PARAM); */
/*        } */
      if (kdtree_table[i].bucket_size) {
	kdtree_table[i].bucket=(int *)malloc(kdtree_table[i].bucket_size*sizeof(int));
	for (j=0; j<kdtree_table[i].bucket_size; j++)
	  fread( &(kdtree_table[i].bucket[j]), sizeof(int), 1, f );
      }      

      fread( &(kdtree_table[i].split_dim), sizeof(int), 1, f );
      fread( &(kdtree_table[i].split_hplane), sizeof(float), 1, f );

      fread( &(kdtree_table[i].low_limit), sizeof(float), 1, f );
      fread( &(kdtree_table[i].high_limit), sizeof(float), 1, f );
      
      fread( &(kdtree_table[i].left_idx), sizeof(int), 1, f );   
      fread( &(kdtree_table[i].right_idx), sizeof(int), 1, f );   
      fread( &(kdtree_table[i].up_idx), sizeof(int), 1, f );   
    }

    // close file
    fclose(f);

    ////////////////////////////////////////////////////////////////
  } else { // ASCII

    if (DEBUG_TREE_READ) fprintf(stderr, " 1 ");

    // open file
    if ( ( f = fopen ( filename, "r" ) ) == NULL ) {
      fprintf(stderr,"Error abriendo fichero %s\n", filename);
      return(ERROR_KNN_FILE);
    }

    if (DEBUG_TREE_READ) fprintf(stderr, " 2 ");

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
    //  fprintf(stderr, "reading POINTS  . . . \n");
    train_str->labelvec=(char **)malloc(train_str->nvectors*sizeof(char *));
    for (i=0; i<train_str->nvectors; i++){
      train_str->labelvec[i]=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
      for (j=0; j<LIBKNN_LABEL_SIZE; j++)
	train_str->labelvec[i][j]='\0';
    }
    train_str->featvec=(float **)malloc(train_str->nvectors*sizeof(float *));
    for (i=0; i<train_str->nvectors; i++)
      train_str->featvec[i]=(float *)malloc(train_str->dim*sizeof(float));

    //  while ( fgets ( pszLine, LIBKNN_MAX_LONG_LINE, f ) ) {
    nvec=0;
    for (i=0; i<train_str->nvectors; i++) {
      fgets(pszLine, LIBKNN_MAX_LONG_LINE, f );
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
	len=strlen(pszToken);
	strncpy(train_str->labelvec[nvec],pszToken,LIBKNN_LABEL_SIZE);
	train_str->labelvec[nvec][len-1]='\0';
      }     
      nvec++;
    } // while ( fgets 
    if (nvec!=train_str->nvectors) {
      fprintf(stderr,"nº de vectores no coincide con especificación de la cabecera\n");
      exit(-1);
    }
    //    fprintf(stderr,"DIM %d\n", train_str->dim);
    //    fprintf(stderr,"NVEC %d\n", train_str->nvectors);
    //    for (i=0; i<train_str->nvectors; i++) {
    //      for (j=0; j<train_str->dim; j++)
    //        fprintf(stderr,"%.16f ", train_str->featvec[i][j]);
    //      fprintf(stderr,"%s\n", train_str->labelvec[i]);
    //    }
  
    if (DEBUG_TREE_READ) fprintf(stderr, " 3 ");

    // read KDTREE structure
    fgets(pszLinea, LIBKNN_MAX_LONG_LINE, f );
    strcpy (pszLine, pszLinea);
    if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {
    
      if ( !strcmp ( pszToken, "KDTREE" ) ) { // lee etiqueta de comienzo del kdtree
	//      fprintf(stderr, "reading KDTREE  . . . \n");
      } else {
	fprintf(stderr, "Error, label KDTREE expected but not found . . . \n");
	return (ERROR_KNN_PARAM);
      }
    }

    // LEER LA TABLA DEL KDTREE
    // (NODE_TYPE NODE_IDX BUCKET_SIZE <BUCKET_POINTS> SPLIT_DIM SPLIT_HPLANE LOW_LIMIT HIGH_LIMIT 
    //  LEFT_NODE_IDX RIGHT_NODE_IDX UP_NODE_IDX)
    // read number of nodes
    fgets(pszLinea, LIBKNN_MAX_LONG_LINE, f );
    strcpy (pszLine, pszLinea);
    if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {
      if ( !strcmp ( pszToken, "NNODES" ) ) { // lee nº de vectores
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((train_str->nnodes = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter NVEC incorrect");
	  return -1;
	}
      }
    }
    fgets(pszLinea, LIBKNN_MAX_LONG_LINE, f );
    strcpy (pszLine, pszLinea);
    if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {
      if ( !strcmp ( pszToken, "MAX_BUCKET_SIZE" ) ) { // lee tamaño máximo del bucket
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((train_str->max_bucket_size = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter MAX_BUCKET_SIZE incorrect");
	  return -1;
	}
      }
    }
    kdtree_table = (t_kdtree_table *)malloc(train_str->nnodes*sizeof(t_kdtree_table));
    for (i=0; i<train_str->nnodes; i++) {
      fgets(pszLine, LIBKNN_MAX_LONG_LINE, f );
      strcpy (pszLinea, pszLine);

      if ( (pszToken = strtok( pszLine, " \t\n" )) != NULL ) {

	// Leaf or Node? ...
	if ( !strcmp ( pszToken, "TREE_NODE" ) ) { // lee dimension de los vectores	
	  // NODE/LEAF
	  kdtree_table[i].node = LIBKNN_TREE_NODE;       
	} else if ( !strcmp ( pszToken, "LEAF_NODE" ) ) { // lee dimension de los vectores	
	  // NODE/LEAF
	  kdtree_table[i].node = LIBKNN_LEAF_NODE;       
	} else {
	  fprintf(stderr, "Error, expected NODE label but none found\n");
	  return ERROR_KNN_PARAM;
	}

	// fill fields ...
	// NODE INDEX
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].node_idx = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter ROOT NODE_IDX incorrect");
	  return ERROR_KNN_PARAM;
	}	
	// BUCKET SIZE
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].bucket_size = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter ROOT BUCKET_SIZE incorrect");
	  return ERROR_KNN_PARAM;
	}	

/*  	if (kdtree_table[i].bucket_size>LIBKNN_MAX_BUCKET_SIZE) { */
/*  	  fprintf(stderr,"libknn: knn_kdtree_read: Error! se ha intentado leer un kdtree con un BUCKET_SIZE (nodo %d, bucket_size=%d) mayor del definido en la librería (LIBKNN_MAX_BUCKET_SIZE=%d)\n", i, kdtree_table[i].bucket_size, LIBKNN_MAX_BUCKET_SIZE); */
/*  	  return(ERROR_KNN_PARAM); */
/*  	} */
	
	if (kdtree_table[i].bucket_size) {
	  kdtree_table[i].bucket=(int *)malloc(kdtree_table[i].bucket_size*sizeof(int));
	  for (j=0; j<kdtree_table[i].bucket_size; j++)
	    // BUCKET [i]
	    if ( 
		((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
		((kdtree_table[i].bucket[j] = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
		) {
	      fprintf(stderr, "Error, value of parameter ROOT BUCKET[j] incorrect");
	      return ERROR_KNN_PARAM;
	    }	
	}

	// SPLIT DIMENSION
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].split_dim = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter ROOT SPLIT_DIM incorrect");
	  return ERROR_KNN_PARAM;
	}	
	// SPLIT HPLANE
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].split_hplane = (float)strtod(pszToken, &ck)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter ROOT SPLIT_HPLANE incorrect");
	  return ERROR_KNN_PARAM;
	}	
	// LOW LIMIT 
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].low_limit = (float)strtod(pszToken, &ck)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter ROOT LOW_LIMIT incorrect");
	  return ERROR_KNN_PARAM;
	}	
	// HIGH LIMIT 
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].high_limit = (float)strtod(pszToken, &ck)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter ROOT HIGH_LIMIT incorrect");
	  return ERROR_KNN_PARAM;
	}	
	// LEFT NODE
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].left_idx = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter INDEX LEFT NODE incorrect");
	  return ERROR_KNN_PARAM;
	}	
	// RIGHT NODE
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].right_idx = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter INDEX RIGHT NODE incorrect");
	  return ERROR_KNN_PARAM;
	}
	// UP NODE
	if ( 
	    ((pszToken = strtok ( NULL, " \t\n" )) == NULL) || 
	    ((kdtree_table[i].up_idx = strtol(pszToken, &ck, 10)) && (*ck != 0)) 
	    ) {
	  fprintf(stderr, "Error, value of parameter INDEX UP NODE incorrect");
	  return ERROR_KNN_PARAM;
	}
      
      }
    } // for (i=0; i<train_str->nnodes
    // close file
    fclose(f);

  } // } else { // ASCII


  if (DEBUG_TREE_READ) fprintf(stderr, " 4 ");

  // printar kdtree_table
//    for (i=0; i<train_str->nnodes; i++) {
//      if (kdtree_table[i].node==LIBKNN_TREE_NODE) {
//        fprintf(stderr,"TREE_NODE %d %d  %d %.2f  %d %d %d\n", 
//      	      kdtree_table[i].node_idx, kdtree_table[i].bucket_size, 
//      	      kdtree_table[i].split_dim, kdtree_table[i].split_hplane, 
//      	      kdtree_table[i].left_idx, kdtree_table[i].right_idx, kdtree_table[i].up_idx);
//      } else {
//        fprintf(stderr,"LEAF_NODE %d %d ", 
//      	      kdtree_table[i].node_idx, kdtree_table[i].bucket_size);
//        fprintf(stderr,"( ");
//        for (j=0; j<kdtree_table[i].bucket_size; j++)
//  	fprintf(stderr, " %d ", kdtree_table[i].bucket[j]);
//        fprintf(stderr," )");
//        fprintf(stderr," %d %.2f  %d %d %d\n", 
//      	      kdtree_table[i].split_dim, kdtree_table[i].split_hplane, 
//      	      kdtree_table[i].left_idx, kdtree_table[i].right_idx, kdtree_table[i].up_idx);
//      }    
//    }
  

  // tree creation from table . . .
  // create & fill root node

  if (DEBUG_TREE_READ) fprintf(stderr, " 5 ");

  train_str->kdtree_vector = (t_kdtree_node *)malloc(train_str->nnodes*sizeof(t_kdtree_node));  
  kdtree_vector = train_str->kdtree_vector;
  k_v_idx = 0;
  kdtree_vector_idx = &k_v_idx;

  g_read_node_counter=0;

  if (DEBUG_TREE_READ) fprintf(stderr, " 6 iter ");
  
  train_str->root = &(kdtree_vector[*kdtree_vector_idx]);
  knn_kdtree_read_nodes(kdtree_vector, 
			kdtree_table, train_str->nnodes);
  
  if (DEBUG_TREE_READ) fprintf(stderr, " 7 ");


  for (i=0; i<train_str->nnodes; i++)
    if (kdtree_table[i].bucket_size>0)
      free(kdtree_table[i].bucket);
  free(kdtree_table);



  train_str->partial_dists=(float *)malloc(train_str->dim*sizeof(float));
  


  return(ERROR_KNN_OK);
}





/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int get_class_color(char label,
		    int *r, int *g, int *b){

  switch (label) {
  case 'A': 
    *r=255;
    *g=0;
    *b=0;
    break;
  case 'B': 
    *r=0;
    *g=255;
    *b=0;
    break;
  case 'C': 
    *r=255;
    *g=255;
    *b=0;
    break;
  case 'D': 
    *r=0;
    *g=0;
    *b=255;
    break;
  case 'E': 
    *r=255;
    *g=0;
    *b=255;
    break;
  case 'F': 
    *r=0;
    *g=255;
    *b=255;
    break;

  case 'G': 
    *r=0;
    *g=32;
    *b=64;
    break;
  case 'H': 
    *r=32;
    *g=64;
    *b=96;
    break;
  case 'I': 
    *r=64;
    *g=96;
    *b=128;
    break;
  case 'J': 
    *r=96;
    *g=128;
    *b=160;
    break;
  case 'K': 
    *r=128;
    *g=160;
    *b=192;
    break;
  case 'L': 
    *r=160;
    *g=192;
    *b=224;
    break;
  case 'M': 
    *r=192;
    *g=224;
    *b=255;
    break;
  case 'N': 
    *r=224;
    *g=255;
    *b=224;
    break;
  case 'O': 
    *r=255;
    *g=224;
    *b=192;
    break;
  case 'P': 
    *r=224;
    *g=192;
    *b=160;
    break;
  case 'Q': 
    *r=192;
    *g=160;
    *b=128;
    break;
  case 'R': 
    *r=160;
    *g=128;
    *b=96;
    break;
  case 'S': 
    *r=128;
    *g=96;
    *b=64;
    break;
  case 'T': 
    *r=96;
    *g=64;
    *b=32;
    break;
  case 'U': 
    *r=64;
    *g=32;
    *b=0;
    break;

  case 'V': 
    *r=64;
    *g=255;
    *b=255;
    break;
  case 'W': 
    *r=255;
    *g=64;
    *b=255;
    break;
  case 'X': 
    *r=255;
    *g=255;
    *b=64;
    break;

  case 'Y': 
    *r=255;
    *g=64;
    *b=64;
    break;
  case 'Z': 
    *r=64;
    *g=255;
    *b=64;
    break;

  default:
    fprintf(stderr,"libknn: knn_kdtree: get_class_color: Clase no esperada!!\n");
    *r=128;
    *g=128;
    *b=128;
    break;    
  }

  return(ERROR_KNN_OK);
}


int knn_compute_trainset_bounds_dim_k(t_knn_kdtree *kdtree, int dim,
				      float *trainset_lower_bound, float *trainset_upper_bound){
  int   i;
  float val[kdtree->nvectors];  
  
  // copiar a un vector local
  for (i=0; i<kdtree->nvectors; i++)
    val[i]=kdtree->featvec[i][dim];
  qsort(val, kdtree->nvectors, sizeof(float),
	compare_floats);  
  // seleccionar valores de los l&#65533;mites
  *trainset_lower_bound = val[0];
  *trainset_upper_bound = val[kdtree->nvectors-1];
  
  return(ERROR_KNN_OK);
}

int knn_kdtree_draw_node(FILE *f, t_knn_kdtree *kdtree, t_kdtree_node *node, int dim_x, int dim_y, 
			 float min_x, float max_x, float min_y, float max_y, float min_data_x, float min_data_y, float scale){
  int point_x1, point_y1, point_x2, point_y2;

   if (node->bucket_size==0){ 
  
    if (node->split_dim == dim_x){
      point_x1=(int)(((node->split_hplane - min_data_x)*scale)+0.5);
      point_y1=(int)(((min_y - min_data_y)*scale)+0.5);

      point_x2=point_x1;
      point_y2=(int)(((max_y - min_data_y)*scale)+0.5);

      // fprintf(stdout,"dimension x %d\n",node->node_idx);
      
      //x constante
      fprintf(f,"2 1 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 2\n%d %d %d %d\n",point_x1,point_y1,point_x2,point_y2);
    
      //num de la particion
      //fprintf(f,"4 0 0 50 0 16 12 0.0000 4 135 450 %d %d %d\\001\n",point_x2,point_y2-10,node->node_idx);
    }

    if (node->split_dim == dim_y){
      //y constante
      point_y1=(int)(((node->split_hplane - min_data_y)*scale)+0.5);
      point_x1=(int)(((min_x - min_data_x)*scale)+0.5);
      
      point_y2=point_y1;
      point_x2=(int)(((max_x - min_data_x)*scale)+0.5);

      //fprintf(stdout,"dimension y %d\n",node->node_idx);
      
      fprintf(f,"2 1 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 2\n%d %d %d %d\n",point_x1,point_y1,point_x2,point_y2);
    
      //num de la particion
      //      fprintf(f,"4 0 0 50 0 16 12 0.0000 4 135 450 %d %d %d\\001\n",point_x2,point_y2-10,node->node_idx);
    }    
    
    // draw descendant nodes, if any ...
    //if (node->bucket_size==0){
    //por ahora asumo left menor y right mayor

    if (node->split_dim == dim_x){
      if (node->left_kdtree!=NULL) knn_kdtree_draw_node(f, kdtree, node->left_kdtree, dim_x, dim_y, 
                                                        min_x, node->split_hplane, min_y, max_y, 
                                                        min_data_x, min_data_y, scale);
      
      if (node->right_kdtree!=NULL) knn_kdtree_draw_node(f, kdtree, node->right_kdtree, dim_x, dim_y, 
                                                         node->split_hplane, max_x, min_y, max_y, 
                                                         min_data_x, min_data_y, scale);
    } else if (node->split_dim == dim_y){
      if (node->left_kdtree!=NULL) knn_kdtree_draw_node(f, kdtree, node->left_kdtree, dim_x, dim_y, 
                                                        min_x, max_x, min_y, node->split_hplane, 
                                                        min_data_x, min_data_y, scale);
      
      if (node->right_kdtree!=NULL) knn_kdtree_draw_node(f, kdtree, node->right_kdtree, dim_x, dim_y, 
                                                         min_x, max_x,node->split_hplane, max_y, 
                                                         min_data_x, min_data_y, scale);
    } else {
      
      /*     if   ((node->split_dim != dim_x) && (node->split_dim != dim_y)) { //no modificamos valores solo pasamos la pelota */

      if (node->left_kdtree!=NULL) knn_kdtree_draw_node(f, kdtree, node->left_kdtree, dim_x, dim_y,
                                                        min_x, max_x , min_y, max_y,
                                                        min_data_x, min_data_y, scale);
      
      /*      if (node->right_kdtree!=NULL) knn_kdtree_draw_node(f, kdtree, node->right_kdtree, dim_x, dim_y,
                                                         min_y, max_x,min_y, max_y,
                                                         min_data_x, min_data_y, scale);
      */
    }
   }
  
  return (ERROR_KNN_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int DLLAPI knn_kdtree_draw(t_knn_kdtree *kdtree, int dim_x, int dim_y,
			   char *str_filename, float *origin, float *scale){
  
  int            i;
  t_kdtree_node *root_node;
  FILE          *f;
  float min_data[2], max_data[2];
  float range[2], max_range;
  int r_color, g_color, b_color;
  int point_x, point_y, max_size_x, max_size_y;
  int dim[2];
  
  dim[0]=dim_x;
  dim[1]=dim_y;
  
  ////////////////////////////////////////////////////////////////
  
  // open file
  if ( ( f = fopen ( str_filename, "w" ) ) == NULL ) {
    fprintf(stderr,"Error creando fichero: %s\n", str_filename);
    return(ERROR_KNN_FILE);
  }  
  
  //print fig header
  fprintf(f,"#FIG 3.2\nLandscape\nCenter\nMetric\nA4\n200.00\nSingle\n-2\n1200 1\n");
  
  //print color palette
  for(i=toascii('A');i<=toascii('Z');i++){
    //#include <ctype.h>
    get_class_color((char)i,
                    &r_color, &g_color, &b_color);
    fprintf(f,"0 %d #%2.2X%2.2X%2.2X\n",i+32,r_color,g_color,b_color);
  }
  
  //compute trainset bounds for dimx, dimy
  knn_compute_trainset_bounds_dim_k(kdtree, dim[0], &min_data[0], &max_data[0]);
  
  ///min_data_x = min_data_x - (max_data_x-min_data_x)*0.05;
  //  max_data_x = max_data_x + (max_data_x-min_data_x)*0.05;
  range[0] = (max_data[0]-min_data[0]);
  
  knn_compute_trainset_bounds_dim_k(kdtree, dim[1], &min_data[1], &max_data[1]);

  //min_data_y = min_data_y - (max_data_y-min_data_y)*0.05;
  //max_data_y = max_data_y + (max_data_y-min_data_y)*0.05;
  
  range[1] = (max_data[1]-min_data[1]);

  if (range[0] > range[1]) max_range=range[0]; else max_range=range[1];
  
  max_size_x=9000;
  max_size_y=(int)(max_size_x*(range[1]/(float)range[0]));
  
  *scale=max_size_x/(float)max_range; 
  origin[0]=min_data[0];
  origin[1]=min_data[1];

  fprintf(f,"#img#2 5 0 1 0 -1 50 0 -1 0.000 0 0 -1 0 0 5\n");
  fprintf(f,"#img#      0 image.tif\n");
  fprintf(f,"#img#      0 0 %d 0 %d %d 0 %d 0 0\n",max_size_x,max_size_x,max_size_y,max_size_y);

  //draw bounds, not real only for limits
  fprintf(f,"2 1 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 2\n%d %d %d %d\n",0,0,max_size_x,0);
  fprintf(f,"2 1 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 2\n%d %d %d %d\n",0,0,0,max_size_y);
  fprintf(f,"2 1 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 2\n%d %d %d %d\n",max_size_x,0,max_size_x,max_size_y);
  fprintf(f,"2 1 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 2\n%d %d %d %d\n",0,max_size_y,max_size_x,max_size_y);

  root_node = kdtree->root;    
  // . . . recursive tree exploration
  knn_kdtree_draw_node(f, kdtree, root_node, dim[0], dim[1], min_data[0],max_data[0], min_data[1], max_data[1], min_data[0],min_data[1], *scale);

  //int knn_kdtree_draw_node(FILE *f, t_knn_kdtree *kdtree, t_kdtree_node *node, int dim_x, int dim_y, float min_x, float max_x, float min_y, float max_y, float min_data_x, float min_data_y, float scale){

  //int knn_kdtree_draw_node(FILE *f, t_knn_kdtree *kdtree, t_kdtree_node *node, int *dim, float *min, float *max, float *min_data,float scale){

  for (i=0; i<kdtree->nvectors; i++) {
    /*     get_class_color(ptrain_vec->labelvec[this_node->npp[0]][0],&r_color, &g_color, &b_color); */
    point_x=(int)(((kdtree->featvec[i][dim[0]]-min_data[0])/(float)(max_range))*max_size_x+0.5);
    point_y=(int)(((kdtree->featvec[i][dim[1]]-min_data[1])/(float)(max_range))*max_size_x+0.5);
    
    fprintf(f,"2 1 0 5 %d 7 50 0 -1 0.000 0 0 -1 0 0 1\n%d %d\n",(int)toascii(kdtree->labelvec[i][0])+32,point_x,point_y);
  }
  
  fclose(f);
  
  return(ERROR_KNN_OK);
}




