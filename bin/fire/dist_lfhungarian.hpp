/*
  This file is part of the FIRE -- Flexible Image Retrieval System

  FIRE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  FIRE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FIRE; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __dist_lfhungarian_hpp__
#define __dist_lfhungarian_hpp__

#include "vectorfeature.hpp"
#include "diag.hpp"
#include "basedistance.hpp"
#include "localfeatures.hpp"
#include "hungarian.h"
#include <stdlib.h>
#include <iostream>

class LFHungarianDistance : public BaseDistance {
public:

  virtual double distance(const BaseFeature* queryFeature, const BaseFeature* databaseFeature) {
    double result=0.0;

    const LocalFeatures* db=dynamic_cast<const LocalFeatures*>(databaseFeature);
    const LocalFeatures* query=dynamic_cast<const LocalFeatures*>(queryFeature);
    
    if(db && query) {
      double **DistMatrix;
      int **Result;
      
      DBG(20) << "Allocating memory" << endl;
      uint m=db->numberOfFeatures();
      uint n=query->numberOfFeatures();
      DistMatrix=(double**)calloc(sizeof(double*),m);
      Result=(int**)calloc(sizeof(int*),m);
      for(uint i=0;i<m;++i) {
        DistMatrix[i]=(double*)calloc(sizeof(double),n);
        Result[i]=(int*)calloc(sizeof(int),n);
      }
      
      DBG(20) << "Initializing Distance Matrix" << endl;
      for(uint i=0;i<m;++i) {
        for(uint j=0;j<n;++j) {
          DistMatrix[i][j]=eucdist((*db)[i],(*query)[j]);
        }
      }
      
      DBG(20) << "Hungarian: start()" << endl;
      solveMinWeightEdgeCover(DistMatrix,Result,m,n);
      
      DBG(20) << "summing up the distances: ";
      for(uint i=0;i<m;++i) {
        for(uint j=0;j<n;++j) {
          result+=Result[i][j]*DistMatrix[i][j];
        }
      }
      BLINK(20) << result << ::std::endl;
      
      DBG(20) << "cleaning up" << endl;
      for(uint i=0;i<m;++i) {
        free(DistMatrix[i]);
        free(Result[i]);
      }
      
      free(DistMatrix); 
      free(Result);
      
    } else {
      ERR << "Features not comparable" << ::std::endl;
      result=-1.0;
    }
    return result;
  }
  
  virtual ::std::string name() {return "lfhungarian";}

  virtual void start(const BaseFeature *) {
  }
  virtual void stop(){
  }

private:
  double eucdist(const vector<double> &a, const vector<double>& b) const {
    double  result=0.0;
    double tmp;
    for(uint i=0;i<a.size();++i) {
      tmp=a[i]-b[i];
      result+=tmp*tmp;
    }
    return result;
  }
  
};

#endif
