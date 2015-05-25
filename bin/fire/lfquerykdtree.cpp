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
#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include "localfeatures.hpp"
#include "getpot.hpp"
#ifdef HAVE_KDTREE_LIBRARY
#include "knn_api.h"
#else
#warning "KDTREE LIBRARY NOT AVAILABLE. This program will not make any sense."
#endif

using namespace std;

void USAGE() {
  cout << "USAGE: " << endl
       << "   lfquerykdtree --features <file with local features> --kdtree <treefile>" << endl
       << "   load local features and query the given kdtree" << endl
       << "   Available options:" << endl
       << "     -h, --help       show this help and exit" << endl
       << "     --binary         to load the tree from a binary file" << endl
       << "     --k <k>          to specify the k for the nearest neighbor" << endl
       << "     --epsilon <epsilon> to specify the epsilon for the approximate nn" << endl
       << endl;
}


int main(int argc, char **argv) {
  GetPot cl(argc, argv);
  
  if(!cl.search("--features") || !cl.search("--kdtree") || cl.search(2,"-h","--help")) {USAGE(); exit(20);}

  string kdtreefilename=cl.follow("tree.kdt","--kdtree");
  string localfeaturesfilename=cl.follow("lf.lf.gz","--features");

  int binary=cl.search("--binary")?1:0;
  uint k=cl.follow(10,"--k");
  double epsilon=cl.follow(0.1,"--epsilon");
  
  DBG(10) << "Loading kdtree from file " << kdtreefilename << endl;
  t_knn_kdtree *kdt=(t_knn_kdtree *)malloc(1*sizeof(t_knn_kdtree)); 
  char *fn=new char[kdtreefilename.size()+1];
  strcpy(fn,kdtreefilename.c_str());
  knn_kdtree_read(fn,binary,kdt);
  delete[] fn;
  
  DBG(10) << "Loading local features from file " << localfeaturesfilename << endl;
  LocalFeatures lf;
  lf.load(localfeaturesfilename);

  //initializatioin for the search
  map<string,uint> hits;
  float *point=new float[lf.dim()];
  t_knn neighbors[k];
  for (uint i=0; i<k; ++i) {
    neighbors[i].featvec=(float *)malloc(lf.dim()*sizeof(float));
    neighbors[i].labelvec=(char *)calloc(LIBKNN_LABEL_SIZE,sizeof(char));
  }

  DBG(10) << "Now querying the kdtree with the local features loaded" << endl;
  for(uint i=0;i<lf.numberOfFeatures();++i) {
    for(uint j=0;j<lf.dim();++j) {
      point[j]=lf[i][j];
    }
    knn_kdtree_search_idc(kdt,point,k,epsilon,neighbors);
    
    for(uint i=0;i<k;++i) {
      hits[string(neighbors[i].labelvec)]++;
    }
  }
  
  vector< pair<uint, string> > toSort;
  
  for(map<string,uint>::const_iterator i=hits.begin();i!=hits.end();++i) {
    toSort.push_back(pair<uint,string>(i->second, i->first));
  }
  sort(toSort.rbegin(),toSort.rend());

  for(vector< pair<uint, string> >::const_iterator i=toSort.begin();i!=toSort.end();++i) {
    cout << i->first << " " << i->second << endl;
  }

  knn_kdtree_free(kdt);
  free(kdt);
  for(uint i=0;i<k;++i) {
    free(neighbors[i].featvec); free(neighbors[i].labelvec);
  }
  delete[] point;
  
  DBG(10) << "cmdline was: " ; printCmdline(argc,argv);
}
