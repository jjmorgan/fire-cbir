/*
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

#include <iostream>
#include <sstream>
#include "gzstream.hpp"
#include "gaussiandensity.hpp"
#include "localfeatures.hpp"
#include "histogramfeature.hpp"
#include "diag.hpp"
#include "getpot.hpp"

using namespace std;

void USAGE(int argc, char** argv) {
  cout << "USAGE: lftiedmixturedensities [options] --clusterfile <cluster model> --histofiles <histogram file 1> .. <histogram file n> --testfiles <test file 1> .. <test file n>" << endl
       << " Options: " << endl
       << "  -s, --sumrule:     use also sum rule for classification" << endl
       << "  -eq, --eqapriori:  use equal a priori class probabilities" << endl
       << "  -c, --confidence=n: use only patches with confidence >= n" << endl;
    
 DBG(10) << "cmdline was: "; printCmdline(argc,argv);
  exit(1);
}

void loadCluster(const string filename, vector<GaussianDensity>& densities) {
  densities.clear();
  igzstream is; is.open(filename.c_str());
  DBG(15) << "loading cluster model from file " << filename << endl;
  if (!is || !is.good()) {
    ERR << "Unable to read file '" << filename << "'." << endl;
  } else {
    string line;
    getline(is, line);
    if (!is.good()) {
      ERR << "Error reading file '" << filename << "'." << endl;
    } else {
      if (line != "# EM/LBG clustering model file") {
        ERR << "This is probably not an EM-Model file" << endl
            << "Continuing anyway" << endl;
      }
      while (!is.eof()) {
        getline(is,line);
        if (!is.eof()) {
          istringstream iss(line);
          string keyword;
          iss >> keyword;
          if (keyword == "gaussians" ) {
            uint size;
            iss >> size;
            densities.resize(size);
          } else if (keyword == "density") {
            uint no;
            iss >> no;
            iss >> keyword;
            if (keyword == "elements") {
              iss >> densities[no].elements;
            } else if (keyword == "dim") {
              iss >> densities[no].dim;
            } else if (keyword == "mean") {
              GaussianDensity& c = densities[no];
              c.mean = vector<double>(c.dim);
              for (uint i = 0; i < c.dim; ++i) {
                iss >> c.mean[i];
              }
            } else if (keyword == "variance") {
              GaussianDensity& c = densities[no];
              c.sigma = vector<double>(c.dim);
              for (uint i = 0; i < c.dim; ++i) {
                iss >> c.sigma[i];
              }
            } else {
              ERR << "Reading density received unknown keyword in position 3: '" << keyword << "'." << endl;
            }
	  } else if ((keyword == "splitmode") || (keyword == "maxsplits") || (keyword == "stopWithNClusters") || (keyword == "disturbMode") ||
		     (keyword == "poolMode") || (keyword == "dontSplitBelow") || (keyword == "iterationsBetweenSplits") || (keyword == "epsilon") ||
		     (keyword == "minObservationsPerCluster") || (keyword == "distance")) {
	    // ignore these keywords
          } else {
            ERR << "Unknown keyword '" << keyword << "'." << endl;
          }
        }
      }
    }
    is.close();
  }
}

void initDistribution(vector< vector<double> >& distribution, const vector<GaussianDensity>& cluster, int numClasses) {
  distribution = vector< vector<double> >(cluster.size() + 1);
  DBG(15) << cluster.size() << " clusters in distribution" << endl;
  DBG(15) << numClasses << " classes" << endl;
  for (int i = 0; i <= int(cluster.size()); i++) {
    distribution[i] = vector<double>(numClasses);
    for (int j = 0; j < numClasses; j++) {
      distribution[i][j] = 0.0;
    }
  }
}

void calcDistribution(vector<string> histofiles, vector< vector<double> >& distribution) {
  int numClusters = (int) distribution.size() - 1;
  for (int i = 0; i < int(histofiles.size()); i++) {
    igzstream ifs; ifs.open(histofiles[i].c_str());
    if (!ifs.good() || !ifs) {
      ERR << "Cannot open filelist " << histofiles[i] << ". Aborting." << endl;
      exit(20);
    }
    string fname;
    while (!ifs.eof()) {
      getline(ifs, fname);
      DBG(20) << "processing histogram '" << fname << "'" << endl;
      if (fname != "") {
	HistogramFeature histo;
	histo.load(fname);
	for (int j = 0; j < numClusters; j++) {
	  if (j >= (int) histo.size()) {
	    ERR << "asking for nonexisting bin " << j << ", histo size is " << histo.size() << endl;
	  }
	  distribution[j][i] += double(histo.bin(j));
	  distribution[numClusters][i] += double(histo.bin(j));
	}
      }
    }
    ifs.close();
  }
  for (int i = 0; i < (int) histofiles.size(); i++) {
    double checksum = 0.0;
    for (int j = 0; j < numClusters; j++) {
      distribution[j][i] /= distribution[numClusters][i];
      checksum += distribution[j][i];
    }
    if (fabs(checksum - 1.0) > 0.00001) {
      ERR << "wrong checksum for class " << i << ": " << checksum << endl;
      exit(1);
    }
  }
}

// classifies a given vector x using the cluster model
void classifyFeature(const vector<double>& x, const vector<GaussianDensity> cluster, const vector< vector<double> >& distribution, vector<double>& classProbs) {
  vector<double> p_x_k;
  uint observation_size = x.size();
  int numClasses = (int) classProbs.size();
  int numClusters = 0;
  
  // iterate over all clusters
  vector<double> p_x_i;

  int i = 0;
  for (vector<GaussianDensity>::const_iterator clusterIterator = cluster.begin(); clusterIterator != cluster.end(); clusterIterator++) {
    if (clusterIterator->dim != observation_size) {
      ERR << "size of local feature and cluster model do not match!" << endl;
    }
    numClusters++;
    double tmp = 0.0;
    double norm = 0.0;
    double aktDist = 0.0;
    uint dims = (uint) observation_size;
    vector<double>::const_iterator meanIterator = clusterIterator->mean.begin();
    vector<double>::const_iterator sigmaIterator = clusterIterator->sigma.begin();
    vector<double>::const_iterator obsIterator = x.begin();

    for (uint d = 0; d < dims; d++) {
      tmp = *obsIterator - *meanIterator;
      tmp *= tmp;
      if (*sigmaIterator > 1E-8) {
	tmp /= *sigmaIterator;
	norm += log(2 * M_PI * *sigmaIterator);
      } else {
	tmp /= 1E-8;
	norm += log(2 * M_PI * 1E-8);
	ERR << "variance is too small: " << *sigmaIterator << endl;
      }
      aktDist += tmp;
      obsIterator++; meanIterator++; sigmaIterator++;
    }
    double log_p_x_i = -0.5 * (aktDist + norm);
    double _p_x_i = exp(log_p_x_i);
    p_x_i.push_back(_p_x_i); // this is p(x|i)
    i++;
  }

  if (numClusters != (int) distribution.size() - 1) {
    ERR << "invalid distribution for cluster" << endl;
    exit(1);
  }
  
  double norm = 0.0;
 
  for (int k = 0; k < numClasses; k++) {

    // calculate p(x|k)
    double _p_x_k = 0.0;

    for (int i = 0; i < numClusters; i++) {
      _p_x_k += p_x_i[i] * distribution[i][k];
    }
    norm += _p_x_k;
    p_x_k.push_back(_p_x_k);
  }
  
  for (int k = 0; k < numClasses; k++) {
    classProbs[k] = p_x_k[k] / norm;
  }
  
}


void doClassification(vector<string> testfiles, const vector< vector<double> >& distribution, const vector<GaussianDensity>& cluster, int numClasses, bool sumRule = false, double conf = 0.0) {
  // load test file lists (for classification)
  vector< vector<string> > localFeaturesFilelists;
  for (int i = 0; i < int(testfiles.size()); i++) {
    vector<string> localFeatureFilelist;
    igzstream ifs; ifs.open(testfiles[i].c_str());
    if (!ifs.good() || !ifs) {
      ERR << "Cannot open filelist " << testfiles[i] << ". Aborting." << endl;
      exit(20);
    }
    string fname;
    while (!ifs.eof()) {
      getline(ifs, fname);      
      if (fname != "") {
	  localFeatureFilelist.push_back(fname);
      }
    }
    ifs.close();
    localFeaturesFilelists.push_back(localFeatureFilelist);
  }

  // load and classify samples
  int okSum = 0, wrongSum = 0, okProd = 0, wrongProd = 0;
  int confusionSum[numClasses][numClasses], confusionProd[numClasses][numClasses];
  for (int cl1 = 0; cl1 < numClasses; cl1++) {
    for (int cl2 = 0; cl2 < numClasses; cl2++) {
      confusionSum[cl1][cl2] = 0;
      confusionProd[cl1][cl2] = 0;
    }
  }
  int progress = 0;

  // classify
  for (int k = 0; k < int(localFeaturesFilelists.size()); k++) {
    for (int j = 0; j < int(localFeaturesFilelists[k].size()); j++) {
      string fname = localFeaturesFilelists[k][j];

      LocalFeatures localFeatures;
      localFeatures.load(fname);

      vector<double> logProbSums(numClasses);
      vector<double> probSums(numClasses);
      vector<double>::iterator logProbSumsIterator = logProbSums.begin();
      vector<double>::iterator probSumsIterator = probSums.begin();
      for (int init = 0; init < numClasses; init++) {
	*logProbSumsIterator = 0;
	if (sumRule) {
	  *probSumsIterator = 0;
	}
	logProbSumsIterator++;
	probSumsIterator++;
      }
      
      vector< vector<double> > patchProbs;
      vector<double> classProbs(numClasses);
      for (int f = 0; f < (int) localFeatures.numberOfFeatures(); f++) {
	// classify f-th local features
	classifyFeature(localFeatures[f], cluster, distribution, classProbs);
	patchProbs.push_back(classProbs);
      }

      int pCount = 0;
      // combine probabilities of local features
      for (vector< vector<double> >::iterator it = patchProbs.begin(); it != patchProbs.end(); it++) {
	bool usePatch = false;
	if (conf > 0.0) {
	  // check if the probability is greater than the confidence threshold
	  double maxClassProb = 0.0;
	  for (vector<double>::iterator mcp = it->begin(); mcp != it->end(); mcp++) {
	    if (*mcp > maxClassProb) {
	      maxClassProb = *mcp;
	    }
	  }
	  usePatch = (maxClassProb >= conf);
	} else {
	  // no confidence threshold
	  usePatch = true;
	}
	if (usePatch) {
	  pCount++;
	  vector<double>::iterator logProbSumsIterator = logProbSums.begin();
	  vector<double>::iterator probSumsIterator = probSums.begin();
	  vector<double>::iterator classProbIterator = it->begin();
	  for (int i = 0; i < numClasses; i++) {
	    *logProbSumsIterator += log(*classProbIterator);
	    if (sumRule) {
	      *probSumsIterator += *classProbIterator;
	    }
	    probSumsIterator++;
	    logProbSumsIterator++;
	    classProbIterator++;
	  }
	}
      }
      
      // find out class with highest probability
      int maxClassSum = -1, maxClassProd = -1;
      double maxProbSum = 0.0, maxProbProd = 0.0;
      for (int i = 0; i < numClasses; i++) {
				if ((maxClassProd == -1) || (logProbSums[i] > maxProbProd)) {
	  			maxProbProd = logProbSums[i];
		  		maxClassProd = i;
				}
      }
      if (sumRule) {
				for (int i = 0; i < numClasses; i++) {
	  			if ((maxClassSum == -1) || (probSums[i] > maxProbSum)) {
	    			maxProbSum = probSums[i];
	    			maxClassSum = i;
	  			}
				}
			}

      // error rate calculation for product rule
      if (maxClassProd == k) {
				okProd++;
				DBG(15) << "(" << pCount << ") - OK: ";
      } else {
				wrongProd++;
				DBG(15) << "(" << pCount << ") - ERROR: ";
      }
      for (int r = 0; r < (int) logProbSums.size(); r++) {
				BLINK(15) << logProbSums[r] << "/";
      }
      BLINK(15) << endl;
      confusionProd[k][maxClassProd]++;
      // error rate calculation for sum rule
      if (sumRule) {
				if (maxClassSum == k) {
	  			okSum++;
	  			DBG(15) << " - OK: ";
				} else {
	  			wrongSum++;
	  			DBG(15) << " - ERROR: ";
				}
				for (int r = 0; r < (int) probSums.size(); r++) {
	  			BLINK(15) << probSums[r] << "/";
				}
				BLINK(15) << endl;
				confusionSum[k][maxClassSum]++;
      }
      progress++;
      DBG(15) << progress << ": error rate(s): ";
      if (sumRule) {
				BLINK(15) << " sum rule=" << (double(wrongSum) / double(okSum + wrongSum));
      }
      BLINK(15) << " product rule=" << (double(wrongProd) / double(okProd + wrongProd)) << endl;
    }

    // print statistics for each file (i.e. each class)
    DBG(10) << endl << "Summary for class " << k << ":" << endl;
    if (sumRule) {
      DBG(10) << "Sum rule: " << endl;
      int numImages = 0;
      for (int n = 0; n < numClasses; n++) {
				numImages += confusionSum[k][n];
      }
      DBG(10) << "# images: " << numImages << endl;
      if (numImages > 0) {
				DBG(10) << "correctly classified: " << confusionSum[k][k] << " (" << (double(confusionSum[k][k]) / double(numImages)) << ")" << endl;
				DBG(15) << "confusion:" << endl;
				for (int k2 = 0; k2 < numClasses; k2++) {
	  			if ((k2 != k) && (confusionSum[k][k2] > 0)) {
	    			DBG(15) << "class " << k2 << ": " << confusionSum[k][k2] << " (" << (double(confusionSum[k][k2]) / double(numImages)) << ")" << endl;
	  			}
				}
      }
    }
    DBG(10) << "Product rule: " << endl;
    int numImages = 0;
    for (int n = 0; n < numClasses; n++) {
      numImages += confusionProd[k][n];
    }
    DBG(10) << "# images: " << numImages << endl;
    if (numImages > 0) {
      DBG(10) << "correctly classified: " << confusionProd[k][k] << " (" << (double(confusionProd[k][k]) / double(numImages)) << ")" << endl;
      DBG(15) << "confusion:" << endl;
      for (int k2 = 0; k2 < numClasses; k2++) {
				if ((k2 != k) && (confusionProd[k][k2] > 0)) {
	  			DBG(15) << "class " << k2 << ": " << confusionProd[k][k2] << " (" << (double(confusionProd[k][k2]) / double(numImages)) << ")" << endl;
				}
      }
    }
  }

  if (sumRule) {
    DBG(10) << "Sum rule: " << endl;
    for (int k1 = 0; k1 < numClasses; k1++) {
      int misclassified = 0;
      for (int k2 = 0; k2 < numClasses; k2++) {
				if (k1 != k2) {
	  			misclassified += confusionSum[k2][k1];
				}
      }
      if (misclassified > 0) {
				DBG(15) << misclassified << " images were misclassified as class " << k1 << endl;
      }
    }
  }

  DBG(10) << endl << "Product rule: " << endl;
  for (int k1 = 0; k1 < numClasses; k1++) {
    int misclassified = 0;
    for (int k2 = 0; k2 < numClasses; k2++) {
      if (k1 != k2) {
				misclassified += confusionProd[k2][k1];
      }
    }
    if (misclassified > 0) {
      DBG(15) << misclassified << " images were misclassified as class " << k1 << endl;
    }
  }

  if (sumRule) {
    DBG(10) << "Sum rule error rate: " << (double(wrongSum) / double(okSum + wrongSum)) << endl;
  }
  DBG(10) << "Product rule error rate: " << (double(wrongProd) / double(okProd + wrongProd)) << endl;

}


int main(int argc, char** argv) {

  GetPot cl(argc, argv);

  // parse command line arguments

  vector<GaussianDensity> cluster;
  if (!cl.search(1, "--clusterfile")) {
    USAGE(argc,argv);
  } else {
    loadCluster(cl.next(" "), cluster);
  }

  vector<string> histofiles;
  if (!cl.search(1, "--histofiles")) {
		ERR << "parameter --histofiles missing!" << endl;
    USAGE(argc,argv);
  } else {
    string filename = cl.next(" ");
    while ((filename != " ") && (filename != "--testfiles")) {
      histofiles.push_back(filename);
      filename = cl.next(" ");
    }
  }

  bool sumRule = cl.search(2, "-s", "--sumRule");
  double conf = 0.0;
  if (cl.search(2, "-c", "--confidence")) {
    conf = cl.next(0.0);
  }
  if (conf > 0.0) {
    DBG(10) << "using only patches with confidence >= " << conf << endl;
  }

  int numClasses = histofiles.size();

  vector<string> testfiles;
  if (!cl.search(1, "--testfiles")) {
		ERR << "parameter --testfiles missing!" << endl;
    USAGE(argc,argv);
  } else {
    string filename = cl.next(" ");
    while (filename != " ") {
      testfiles.push_back(filename);
      filename = cl.next(" ");
    }
  }

  if (testfiles.size() != histofiles.size()) {
    ERR << histofiles.size() << " histogram files, but " << testfiles.size() << " test files given!" << endl;
    exit(1);
  }

  vector< vector<double> > distribution;
  initDistribution(distribution, cluster, numClasses);
  calcDistribution(histofiles, distribution);

  doClassification(testfiles, distribution, cluster, numClasses, sumRule, conf);

  DBG(10) << "cmdline was: "; printCmdline(argc,argv);

}
