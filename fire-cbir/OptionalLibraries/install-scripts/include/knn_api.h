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
 * Librería para la implementación de la Búsqueda de los K-Vecinos Más Cercanos (KNN)
 * Desarrollado por el grupo de PRHLT del Instituto Tecnológico de Informática.    
 * Universidad Politecnica de Valencia.                         
 *
 * Fichero: knn_api.h
 * 
 * Fecha: 2003-11
 *
 * Descripción: 
 *   API de la librería 
 *   
 ****************************************************************************/

#ifndef __KNN_API_H__
#define __KNN_API_H__


#include <stdio.h>

//// Exportar llamadas en windows /////////////////////////////////////
#ifdef _WINDOWS

#include "windows.h"
#ifndef DLLAPI
#define DLLAPI _stdcall

#endif
#else

#ifndef DLLAPI
#define DLLAPI
#endif

#endif


//// Verbose levels ////////////////////////////////////////////////////
#define VERBOSE_KNN_ERROR         0

#define VERBOSE_KNN_KDTREE_CREATE 0
#define VERBOSE_KNN_KDTREE_TABLE  0

//// Códigos de error //////////////////////////////////////////////////
#define ERROR_KNN_OK     0
#define ERROR_KNN_MEM    1
#define ERROR_KNN_FILE   2
#define ERROR_KNN_PARAM  3

#define ERROR_KNN_REGISTRO_VACIO 4



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// K-Nearest Neighbours /////////////////////////////////////////////////
typedef struct {
  float          dist;
  int            idx;
  float         *featvec;
  char          *labelvec;
} t_knn;

typedef struct {
  char          *labelvec;
  float          conf;
} t_resknn;

typedef float    *t_point;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// LIST ////////////////////////////////////////////////////////////////
typedef struct {
  int             dim;
  int             nvectors;
  float         **featvec;
  char          **labelvec;
} t_fvec;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// K-Dimensional-TREE (Arya-Mount) /////////////////////////////////////
struct kdtree_node {

  int                    node_idx;

  int                    bucket_size; // number of points in a LEAF node 
  int                   *bucket;

  int                    split_dim;
  float                  split_hplane;

  float                  low_limit;
  float                  high_limit;

  struct kdtree_node    *left_kdtree;
  struct kdtree_node    *right_kdtree;
  struct kdtree_node    *up_kdtree;
};
typedef struct kdtree_node t_kdtree_node;

typedef struct {
  //  t_fvec               *prototypes;      // pointer to prototypes (vectors, dim and size)
  int                   dim;       // dimension of the points
  int                   nvectors;  // number of points
  float               **featvec;   // multidimensional points
  char                **labelvec;  // point labels

  float                *partial_dists;  // auxiliary search vector: temporal 
                                        // distances for each dimension

  struct kdtree_node   *root;      // pointer to the root node of the kd-tree data structure
  int                   nnodes;    // number of nodes (nodes+leafs) of the kd-tree
  int                   max_bucket_size; // max number of prototypes in a bucket
  struct kdtree_node   *kdtree_vector;  // all the tree in a linear vector
} t_knn_kdtree;

typedef struct {
  int   node;

  int   bucket_size;
  int  *bucket;

  int   split_dim;
  float split_hplane;

  float low_limit;
  float high_limit;

  int   node_idx;
  int   left_idx;
  int   right_idx;
  int   up_idx;
} t_kdtree_table;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


#define LIBKNN_TRUE  1
#define LIBKNN_FALSE 0

#define LIBKNN_LABEL_SIZE  255

#define LIBKNN_MAX_DIST 10000000000000000.0

#define LIBKNN_HUGE_POSITIVE +1000000000.0
#define LIBKNN_HUGE_NEGATIVE -1000000000.0

#define LIBKNN_TREE_NODE 1
#define LIBKNN_LEAF_NODE 2


//#define LIBKNN_MAX_LONG_LINE 8192
#define LIBKNN_MAX_LONG_LINE 300000 // linea de ORDERED_SET en las PROJB de 44951 palabras, 258597 carac ...



// Macros
#define min_libknn(x,y) ( (x) < (y) ? (x) : (y) )



////////////////////////////////////////////////////////////////////////
// exported functions //////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// KNN common

int DLLAPI knn_rule_simple(t_knn *knn, int k,
			   t_resknn *res_knn, int *n_dif);

int DLLAPI knn_rule(t_knn *knn, int k,
		    t_resknn *res_knn, int *n_dif);

////////////////////////////////////////////////////////////////////////
// approximate KNN with KDTREE data structure

int DLLAPI knn_kdtree_create(t_fvec *ptrain_vec, int max_bucket_size,
			     t_knn_kdtree *ptrain_str);

int DLLAPI knn_kdtree_save(t_knn_kdtree *ptrain_str, int max_bucket_size, int binary,
			   char *str_filename);

int DLLAPI knn_kdtree_read(char *filename, int binary,
			   t_knn_kdtree *train_str);

int DLLAPI knn_kdtree_free(t_knn_kdtree *ptrain_str);

int DLLAPI knn_kdtree_search_idc  (t_knn_kdtree *train_str, float *point, int k, float eps, 
				   t_knn *knn);

int DLLAPI knn_kdtree_search_idc_pq (t_knn_kdtree *train_str, float *point, int k, float eps,
				     t_knn *knn);

// Priority Queue Ops

int DLLAPI PQ_SortedList_Insert(t_kdtree_node  *node,       float dist,
				t_kdtree_node **node_array, float *dist_array,
				int *n);
int DLLAPI PQ_SortedList_Extract(t_kdtree_node **node_array, float *dist_array,
				 t_kdtree_node **node,       float *dist,
				 int *n);

int DLLAPI PQ_Heap_Insert(t_kdtree_node  *node,       float dist,
			  t_kdtree_node **node_array, float *dist_array,
			  int *n);
int DLLAPI PQ_Heap_Extract(t_kdtree_node **node_array, float *dist_array,
			   t_kdtree_node **node,       float *dist,
			   int *n);

// ///////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////
// functions for managing data

int DLLAPI knn_data_read(char *filename, 
			 t_fvec *train_vec);

int DLLAPI knn_data_free(t_fvec *train_vec);

#endif

