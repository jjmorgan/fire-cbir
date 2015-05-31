#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// gettimeofday
#include <sys/time.h>
#include <unistd.h>

#include "knn_api.h"

#define MAX_LONG_LINE   4096

// para calcular el "rank number"
//#define K_NEIGHBOURS       20
//#define EPS                0.0

//#define K_NEIGHBOURS       1
#define K_NEIGHBOURS       4

//#define EPS                0.0
//#define EPS                1.0
#define EPS                2.0
//#define EPS                3.0
//#define EPS                4.0
//#define EPS                6.0
//#define EPS                8.0
//#define EPS               64.0
//#define EPS             2048.0


// TIME ****************************************************************************

int timeval_subtract (struct timeval *result, struct timeval *x, 
		      struct timeval *y)
{
  // Perform the carry for the later subtraction by updating Y. 
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }
     
  // Compute the time remaining to wait.
  //   `tv_usec' is certainly positive. 
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
     
  // Return 1 if result is negative.
  return x->tv_sec < y->tv_sec;
}
// TIME ****************************************************************************

int main(int argc, char *argv[]) {

  char           *psz_train, *psz_test;
  t_knn_kdtree   *train_str;
  t_fvec         *test_vec;
  int             i, j, nvec, dim, len;
  int		  binary, RC=0;

  int             ok,kao;
  t_knn           knn[K_NEIGHBOURS];
  t_point         point;
  char            winner_class[LIBKNN_LABEL_SIZE];
  float           winner_conf;

  int             ndif;
  t_resknn       *res_knn;

  static struct timeval time1, time2, timedif;
  int                   miliseconds;

//    int           *selected;
//    int          **selected_point;
//    float         *partial_dists;


  if (argc < 5 || argc > 5 || !strcmp(argv[1], "-h"))
    {
      fprintf(stderr, "\nUso: %s [-h] binary [0,1] train.str test.vec\n\n", argv[0]);
      fprintf(stderr, "             -h: help\n");
      return -1;
    }
  
  binary = atoi(argv[2]);
  psz_train = argv[3];
  psz_test = argv[4];

  // ////////////////////////////////////////////////////////////////////////////////////////
  // READ TRAIN
  fprintf(stderr, "leyendo train ... ");
  train_str = (t_knn_kdtree *)malloc(1*sizeof(t_knn_kdtree));
  RC=knn_kdtree_read(psz_train, binary,
		     train_str);
  if (RC){
    fprintf(stderr,"Error leyendo la estructura de datos %s ...\n", psz_train); 
    exit (-1); 
  }
  fprintf(stderr, "ok\n");

  // ////////////////////////////////////////////////////////////////////////////////////////
  // READ TEST POINTS  
  test_vec = (t_fvec *)malloc(sizeof(t_fvec));
  knn_data_read(psz_test, test_vec);
  fprintf(stderr,"Read %d %d-dimensional vectors\n",test_vec->nvectors,test_vec->dim);

  // ////////////////////////////////////////////////////////////////////////////////////////
  // CHECK DIMENSION
  if (train_str->dim!=test_vec->dim) {
    fprintf(stderr,"dimensión de los vectores no coincide en train y test\n");
    exit(-1);
  }

  // ////////////////////////////////////////////////////////////////////////////////////////
  // ////////////////////////////////////////////////////////////////////////////////////////
  // ////////////////////////////////////////////////////////////////////////////////////////
  // CLASIFICAR TEST

  res_knn = (t_resknn *)malloc(K_NEIGHBOURS*sizeof(t_resknn));
  for (i=0; i<K_NEIGHBOURS; i++) {
    res_knn[i].labelvec=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
    for (j=0; j<LIBKNN_LABEL_SIZE; j++)
      res_knn[i].labelvec[j]='\0';
  }

  for (i=0; i<K_NEIGHBOURS; i++) {
    knn[i].featvec=(float *)malloc(test_vec->dim*sizeof(float));
    knn[i].labelvec=(char *)malloc(LIBKNN_LABEL_SIZE*sizeof(char));
    for (j=0; j<LIBKNN_LABEL_SIZE; j++)
      knn[i].labelvec[j]='\0';
  }
  point = (t_point)malloc(test_vec->dim*sizeof(float));
  ok = kao = 0;

  // TIME1 ****************************************************************************
#ifndef _WINDOWS
  gettimeofday (&time1, NULL);
#endif


  for (nvec=0; nvec<test_vec->nvectors; nvec++) {

    // seleccionar punto de test a buscar
    for (i=0; i<test_vec->dim; i++)
      point[i]=test_vec->featvec[nvec][i];

    // buscar k vecinos más cercanos
    //      RC = knn_kdtree_search_exact_brute(train_str, point, K_NEIGHBOURS,
    //    knn);
    RC = knn_kdtree_search_idc(train_str, point, K_NEIGHBOURS, EPS, knn);
    //RC = knn_kdtree_search_idc_pq(train_str, point, K_NEIGHBOURS, EPS,
    //				  knn);
    if (RC){
      fprintf(stderr,"Error buscando knn ...\n"); 
      exit (-1); 
    }			   

    fprintf(stdout, "%d %s \t\t", nvec, test_vec->labelvec[nvec]);
    //    fprintf(stdout, "%d (%s)---", nvec, test_vec->labelvec[nvec]);

    // mostrar resultados de la búsqueda
    for (i=0; i<K_NEIGHBOURS; i++) {
      fprintf(stdout, "K%d ", i);
      fprintf(stdout, "%d ", knn[i].idx);
      fprintf(stdout, "%s ", knn[i].labelvec);
      //      fprintf(stdout, "%.2f", knn[i].dist);
      fprintf(stdout, "%.2f", knn[i].dist);
      fprintf(stdout, "\t");
    }

    // //////////////////////////////////////////////////////////////////////////////////////////////
    // seleccionar clase ganadora (knn rule) a partir del conjunto de los k vecinos más cercano
    //    RC = knn_rule_simple(knn, K_NEIGHBOURS,
/*      RC = knn_rule(knn, K_NEIGHBOURS, */
/*  		  res_knn, &ndif); */
/*      if (RC){ */
/*        fprintf(stderr,"Error seleccionando clase ganadora ...\n");  */
/*        exit (-1);  */
/*      }	 */
/*      // seleccionar la clase ganadora -> indice 0 (como máximo habrán K_NEIGHBOURS clases distintas) */
/*      strncpy(winner_class,res_knn[0].labelvec,LIBKNN_LABEL_SIZE); */
/*      winner_conf  = res_knn[0].conf; */
/*      // printar resultado */
/*      fprintf(stdout, "\t"); */
/*      len=strlen(test_vec->labelvec[nvec]); */
/*      if (!strncmp(test_vec->labelvec[nvec],winner_class,len)) { */
/*        ok++; */
/*        fprintf(stdout, "%s %.4f OK\n", winner_class, winner_conf); */
/*      } else { */
/*        kao++; */
/*        fprintf(stdout, "%s %.4f KAO\n", winner_class, winner_conf); */
/*      } */

    fprintf(stdout, "\n");
    //if (nvec==100) exit(0);
    if (nvec%100==0) fprintf(stderr, "%d ", nvec);

  }
  fprintf(stderr, "\n");

#ifndef _WINDOWS
  gettimeofday (&time2, NULL);
  timeval_subtract ( &timedif, &time2, &time1 );
  fprintf(stdout, "%ld%.4f milisegundos\n", timedif.tv_sec, (float)((float)(timedif.tv_usec)/1000.0) );  
/*    miliseconds = timedif.tv_sec*1000 + timedif.tv_usec/1000; */
/*    fprintf(stdout, "%.4f miliseconds/point\n", (float)miliseconds/(float)(kao+ok)); */
#endif

  // TIME2 ****************************************************************************

/*    fprintf(stdout,"Total: %d\n", ok+kao); */
/*    fprintf(stdout," ok:  %d  %.2f%%\n", ok,   100.0*((float)ok/(float)(kao+ok))); */
/*    fprintf(stdout," kao: %d  %.2f%%\n", kao, 100.0*((float)kao/(float)(kao+ok))); */

  // TIME AVG
  // fprintf(stdout, "Avg. time = %.4g\n", (float)time_dif/(float)(kao+ok));




  // free malloc's . . . 

  // res_knn
  for (i=0; i<K_NEIGHBOURS; i++)
    free(res_knn[i].labelvec);
  free(res_knn);
  // point
  free(point);
  // knn
  for (i=0; i<K_NEIGHBOURS; i++) {
    free(knn[i].featvec);
    free(knn[i].labelvec);
  }  
  //  test_vec
  knn_data_free(test_vec);
  //  train_str
  knn_kdtree_free(train_str);
  free(train_str);
  


  return(0);
}                          
