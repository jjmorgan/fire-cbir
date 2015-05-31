#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "knn_api.h"

#define MAX_LONG_LINE   4096

int main(int argc, char *argv[]) {

  char           *psz_vec, *psz_str;
  t_fvec         *train_vec;
  t_knn_kdtree   *train_str;
  int             i, max_bucket_size=1, binary=0;
  int		  RC=0;

  if (argc < 7 || argc > 7 || !strcmp(argv[1], "-h"))
    {
      fprintf(stderr, "\nUso: %s [-h] bucket_size [1,2,...n] binary [0,1] train_red.vec kdtreename.str\n\n", argv[0]);
      fprintf(stderr, "             -h: help\n");
      return -1;
    }

  max_bucket_size = atoi(argv[2]);
  binary = atoi(argv[4]);
  psz_vec = argv[5];
  psz_str = argv[6];

  // leer vectores
  train_vec = (t_fvec *)malloc(sizeof(t_fvec));
  knn_data_read(psz_vec, train_vec);
  fprintf(stderr,"Read %d %d-dimensional vectors\n",train_vec->nvectors,train_vec->dim);

  // Construir estructura de kdtree . . . 
  train_str = (t_knn_kdtree *)malloc(1*sizeof(t_knn_kdtree));  
  RC=knn_kdtree_create(train_vec, max_bucket_size,
		       train_str);
  // . . . y salvarla en disco
  RC=knn_kdtree_save(train_str, max_bucket_size, binary,
		     psz_str);

  // free fvecs & kdtree
  knn_kdtree_free(train_str);
  knn_data_free(train_vec);
  
  return(0);
}                          
