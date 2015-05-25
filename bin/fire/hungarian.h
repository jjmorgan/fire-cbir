#ifndef __hungarian_h__
#define __hungarian_h__

extern "C" {
  void solveAssignmentProblemDoubleRect(double **Array, int **Result, int m, int n);
  void printstate();
  void solveMinWeightEdgeCover(double **Array, int **Result, int size1, int size2);
}

#endif
