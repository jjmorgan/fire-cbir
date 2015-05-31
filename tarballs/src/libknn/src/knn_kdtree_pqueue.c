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
 * Fichero: knn_kdtree_pqueue.cpp
 * 
 * Fecha: 2003-05
 *
 * Descripción: 
 *   Primitives for the use Priority Queues
 *   
 ****************************************************************************/

#include "knn_api.h"


// /////////////////////////////////////////////////////////////////////////////////////////////////////////
// PQ implementation with a Sorted List -> PQ_Extract is O(1), but PQ_Insert is O(N)

// LAST COMPONENT of pq "dist_array" must host the HIGHEST PRIORITY !!!
void PQ_SortedList_Save(register float *dist_array, register t_kdtree_node **node_array, register int n) {
  float          dist;
  t_kdtree_node *node;
  
  n--;  // index to the last element
  dist = dist_array[n];  // distance of the new element
  node = node_array[n];  // node index of the new element
  // run through the list in reverse direction & shift list
  while ((dist_array[n-1] <= dist) && (n>0)) {  
    dist_array[n] = dist_array[n-1];      // swap distance values
    node_array[n] = node_array[n-1];      // swap node index values 
    n--;                                  // next element
  }
  // insert new element
  dist_array[n] = dist;  // insert distance
  node_array[n] = node;  // insert node index
}

// insert node & distance into both priority queues
// "n" is the number of elements in the queue
int DLLAPI PQ_SortedList_Insert(t_kdtree_node  *node,       float dist,
				t_kdtree_node **node_array, float *dist_array,
				int *n){ 
 
  (*n)++;                     // update queue size  
  dist_array[*n - 1] = dist;  // insert new value at the end of distance queue
  node_array[*n - 1] = node;  // insert new value at the end of node queue
  // sort the queues by (reverse) distance order; lower distances at the end of the queue
  PQ_SortedList_Save(dist_array, node_array, *n);
  
  return(ERROR_KNN_OK);
}

// retrieve least distance node
// "n" is the number of elements in the queue
int DLLAPI PQ_SortedList_Extract(t_kdtree_node **node_array, float *dist_array,
				 t_kdtree_node **node,       float *dist,
				 int *n){  

  *dist = dist_array[*n - 1];                 // get the value of the last element
  *node = node_array[*n - 1];                 // get the value of the last element 
  dist_array[*n - 1]=(float)LIBKNN_MAX_DIST;  // reset queue value
  node_array[*n - 1]=NULL;                    // reset queue value  
  (*n)--;                                     // update queue size
 
  return(ERROR_KNN_OK);
}




// /////////////////////////////////////////////////////////////////////////////////////////////////////////
// PQ implementation with a Heap  -> PQ_Extract is O(Log N) and PQ_Insert is also O(Log N)

// A heap can be used to keep the queue elements in a partially-sorted
// order. Insertions and extractions can then be done in log(N) time,
// with virtually no extra memory overhead.

// A heap has the following key characteristics:

//    * The elements of a heap are stored in a contiguous array. If
//    the heap holds N elements, they will be stored at locations 1
//    through N of the array. 

//    * The elements in the array are also part of an implicit binary
//    tree. Every element X (except the root node) has a parent at
//    location X/2.

//    * Each node has a priority that is greater than or equal to both
//    of its children.

// The important thing to note about a heap is that is not a
// completely ordered tree. Even though parent nodes are always
// greater than children, there is no ordering among siblings.

// Even though a heap is not completely sorted, it has one very useful
// characteristic: the node with the highest priority will always be
// at the top of the tree. This makes it suitable for a priority
// queue, since the pop() operation only requires the highest
// element. Combining this characteristic with the negligible overhead
// and fast insertions and removals makes the heap an ideal data
// structure for implementation of a priority queue.
// ////////////////////////////////////////////////////////////////////

// Insert a new element in the Heap: O(Log N)
// FIRST !!! COMPONENT of pq "dist_array" must host the HIGHEST PRIORITY !!!
int DLLAPI PQ_Heap_Insert(t_kdtree_node  *node,       float dist,
			  t_kdtree_node **node_array, float *dist_array, 
			  int *n){ 
  int parent_node, i;
  float temp_dist;
  t_kdtree_node *temp_node;
  
  // Set a new element at the end of the heap
  (*n)++;                     // update queue size  
  dist_array[*n - 1] = dist;  // insert new values at the end of distance queue
  node_array[*n - 1] = node;  // insert new values at the end of node queue

  // And sort of the heap by repeatedly swapping the new node with its
  // parent node until it reaches a position where its parent is
  // greater than it.
  i=*n;
  while ((i-1)>0) {
    parent_node = i/2;
    if (dist_array[parent_node-1]>dist_array[i-1]) {
      // swap dist heap
      temp_dist                 = dist_array[parent_node-1];
      dist_array[parent_node-1] = dist_array[i-1];
      dist_array[i-1]           = temp_dist;
      // swap node heap
      temp_node                 = node_array[parent_node-1];
      node_array[parent_node-1] = node_array[i-1];
      node_array[i-1]           = temp_node;
      // next element
      i=parent_node;
    } else {
      break;
    }
  }

  return(ERROR_KNN_OK);
}

// Sort of the heap after an extract operation: the new root node is
// moved down the tree, swapping the greater of its two children,
// until it is greater than both of its children
//
// This code is slightly more complicated than that of
// PQ_Heap_Sort_After_Insert, but it can clearly be seen that it is
// performing the reverse operation. The least element has been moved
// to the root of the heap, so it loops while checking to see if the
// root node is less than one of its children. If it is, the node is
// swapped with its largest child, and the process repeats.

// FIRST !!! COMPONENT of pq "dist_array" must host the HIGHEST PRIORITY !!!
int PQ_Heap_Sort_After_Extract( int heap_size,
				 t_kdtree_node **node_array, float *dist_array ) {
  int father_node,child_node;     // range [0, n-1]
  int first_child,second_child;   // range [1,   n]
  float temp_dist;
  t_kdtree_node *temp_node;
 
  father_node=0; // start with the new root node
  first_child = (father_node+1)*2;

  while (first_child<=heap_size){

    second_child=first_child+1;
    if (second_child>heap_size) // we are at the end of a branch with a single leaf
      child_node=first_child-1;
    else {                      // else, select the child with the highest priority
      if (dist_array[first_child-1]<dist_array[second_child-1])
	child_node=first_child-1;
      else
	child_node=second_child-1;
    }

    // FIRST !!! COMPONENT of pq "dist_array" must host the HIGHEST PRIORITY !!!
    if (dist_array[father_node]>dist_array[child_node]) {  // if child with lower value: swap nodes & update father node
      // swap dist heap
      temp_dist               = dist_array[father_node];
      dist_array[father_node] = dist_array[child_node];
      dist_array[child_node]  = temp_dist;
      // swap node heap
      temp_node               = node_array[father_node];
      node_array[father_node] = node_array[child_node];
      node_array[child_node]  = temp_node;
      // next element      
      father_node = child_node;
      first_child = (father_node+1)*2;
    } else {  // if child with lower value, finish (heap is sorted)
      return(ERROR_KNN_OK);
    }	     
  }

}

// Extract the first element of the Heap: O(Log N)
int DLLAPI PQ_Heap_Extract(t_kdtree_node **node_array, float *dist_array,
			   t_kdtree_node **node,       float *dist, 
			   int *n){  
  float temp_dist;
  t_kdtree_node *temp_node;

  // Swap first(root) and last elemente of the heap
  // swap dist
  temp_dist        = dist_array[0];
  dist_array[0]    = dist_array[*n-1];
  dist_array[*n-1] = temp_dist;
  // swap node
  temp_node        = node_array[0];
  node_array[0]    = node_array[*n-1];
  node_array[*n-1] = temp_node;
  
  // get the new last element(root) and update the heap size
  *dist = dist_array[*n - 1];                 // get the value of the last element
  *node = node_array[*n - 1];                 // get the value of the last element 
  dist_array[*n - 1]=(float)LIBKNN_MAX_DIST;  // reset queue value
  node_array[*n - 1]=NULL;                    // reset queue value  
  (*n)--;                                     // update queue size

  // ReSortHeap
  PQ_Heap_Sort_After_Extract(*n, 
			     node_array, dist_array);  
  return(ERROR_KNN_OK);
}

