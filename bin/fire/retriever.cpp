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
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <algorithm>
#include "retriever.hpp"
#include "dist_metafeature.hpp"
#include "dist_textfeature.hpp"
#include "textfeature.hpp"
#include "getscoring.hpp"
#include "net.hpp"

using namespace std;

void Retriever::setScoring(const string &scoringname) {
  delete scorer_;
  scorer_=getScoring(scoringname,database_.numberOfSuffices());
}

void Retriever::resolveNames(const vector< string > &queryNames, 
                             vector<ImageContainer*> &queries, 
                             stack<ImageContainer*> &newCreated){
  
  for(uint i=0;i<queryNames.size();++i) {
    ImageContainer* tmp=NULL;
    tmp=database_.getByName(queryNames[i]);
    if(!tmp) { 
      DBG(10) << "Image not in database, trying to load: " << queryNames[i];
      tmp=new ImageContainer(queryNames[i],database_.numberOfSuffices());
      if(!database_.loadQuery(queryNames[i],tmp)) {
        delete tmp;
        tmp=NULL;
      }
      if(tmp) {
        newCreated.push(tmp);
        BLINK(10) << " ... done"<< endl;
      } else BLINK(10) << "... failed" << endl;
    }
    if(tmp) queries.push_back(tmp);
  }
}

void Retriever::retrieve(const vector< string >& posQueryNames,
                         const vector< string >& negQueryNames,
                         vector<ResultPair>& results) {
  
  // get image containers for these images
  vector<ImageContainer*> posQueries;
  vector<ImageContainer*> negQueries;
  
  stack<ImageContainer*> newCreated; // memorize those images that must be deleted after processing the query
  
  resolveNames(posQueryNames, posQueries, newCreated);
  resolveNames(negQueryNames, negQueries, newCreated);

  retrieve(posQueries,negQueries,results);
  while(!newCreated.empty()) {
    delete newCreated.top();
    newCreated.pop();
  }
}

void Retriever::getScores(const ImageContainer* q, vector<double>&scores) {
  uint N=database_.size();
  uint M=database_.numberOfSuffices();
  
  vector< vector<double> > distMatrix(N,vector<double>(M));
  vector<double> imgDists;
  
  //get distance to each of the database images
  for(uint i=0;i<N;++i) {
    vector<double>&d=distMatrix[i];
    imgDists=imageComparator_.compare(q,database_[i]);
    for(uint j=0;j<M;++j) {
      d[j]=imgDists[j];
    }
  }
  
  //normalize
  for(uint j=0;j<M;++j) {
    double sum=0.0;
    for (uint i=0;i<N;++i) {sum+=distMatrix[i][j];}
    sum/=double(N);
    if(sum!=0.0) {
      double tmp=1/sum;
      for(uint i=0;i<N;++i) {
        distMatrix[i][j]*=tmp;
      }
    }
  }

  interactor_.apply(distMatrix);
  
  //now get the scores
  for(uint i=0;i<N;++i) {
    scores[i]=scorer_->getScore(distMatrix[i]);
  }
}

void Retriever::saveDistances(string imagename, string filename) {
  /*----------------------------------------------------------------------
   * get distance matrix 
   * --------------------------------------------------------------------*/
  uint N=database_.size();
  uint M=database_.numberOfSuffices();
  bool newlyLoaded=false;
  vector< vector<double> > distMatrix(N,vector<double>(M));
  vector<double> imgDists;
  
  ImageContainer *q=database_.getByName(imagename);
  if(!q) { 
    DBG(10) << "Image not in database, trying to load: " << imagename;
    q=new ImageContainer(imagename,database_.numberOfSuffices());
    if(!database_.loadQuery(imagename,q)) {
      delete q;
      q=NULL;
      BLINK(10) << "... failed" << endl;
    } else {
      BLINK(10) << " ... done"<< endl;
      newlyLoaded=true;
    }
  }
  
  //get distance to each of the database images
  for(uint i=0;i<N;++i) {
    vector<double>&d=distMatrix[i];
    imgDists=imageComparator_.compare(q,database_[i]);
    for(uint j=0;j<M;++j) {
      d[j]=imgDists[j];
    }
  }

 
  //normalize
  for(uint j=0;j<M;++j) {
    double sum=0.0;
    for (uint i=0;i<N;++i) {sum+=distMatrix[i][j];}
    sum/=double(N);
    if(sum!=0.0) {
      double tmp=1/sum;
      for(uint i=0;i<N;++i) {
        distMatrix[i][j]*=tmp;
      }
    }
  }
  
  /*----------------------------------------------------------------------
   * save distance matrix
   * --------------------------------------------------------------------*/

  ifstream is;
  is.open(filename.c_str());
  if(is.good()) {
    ERR << "Distance file to be written '" << filename << "' exists already. Aborting" << endl;
    exit(10);
  }
  
  ofstream os;
  os.open(filename.c_str());
  if(!os.good()) {
    ERR << "Cannot write distance file '" <<filename << "'." << endl;
    exit(10);
  } else {
    os << "# distmatrix for "<< q->basename() << endl;
    os << "# distances" ; for(uint j=0;j<M;++j) { os << " " << imageComparator_.distance(j)->name(); } os << endl; 
    os << "nofdistances "<< N << " " << M << endl;
    
    for(uint i=0;i<N;++i) {
      os << i;
      for(uint j=0;j<M;++j) {
        os << " "<<distMatrix[i][j];
      } os << endl;
    } os.close();
  }

  if(newlyLoaded) {
    delete q;
    q=NULL;
  }


}

void Retriever::retrieve(const vector<ImageContainer*>& posQueries, 
                                       const vector<ImageContainer*>& negQueries,
                                       vector<ResultPair>& results) {
  
  uint N=database_.size();
  vector<double> activeScores(N,0.0);
  
  //init result field
  results.clear();
  for(uint i=0;i<database_.size();++i) {
    results.push_back(ResultPair(0.0,i));
  }
  
  //positive queries
  for(uint q=0;q<posQueries.size();++q) {
    DBG(10) << "Positive query: " << posQueries[q]->basename() << endl;
    if(imageComparator_.size() != posQueries[q]->numberOfFeatures()) {
      ERR << "ImageComparator has different number of distances (" << imageComparator_.size() << ") than positive query " << q << " (" << posQueries[q]->numberOfFeatures() << ")." << endl;
    }
    imageComparator_.start(posQueries[q]);
    getScores(posQueries[q],activeScores);
    imageComparator_.stop();
    for(uint i=0;i<N;++i) {
      results[i].first+=activeScores[i];
    }
  }
  
  //negative queries
  for(uint q=0;q<negQueries.size();++q) {
    DBG(10) << "Negative query: " << negQueries[q]->basename() << endl;
    if(imageComparator_.size() != negQueries[q]->numberOfFeatures()) {
      ERR << "ImageComparator has different number of distances (" << imageComparator_.size() << ") than negative query " << q << " (" << negQueries[q]->numberOfFeatures() << ")." << endl;
    }
    imageComparator_.start(negQueries[q]);
    getScores(negQueries[q],activeScores);
    imageComparator_.stop();
    for(uint i=0;i<N;++i) {
      results[i].first+=(1-activeScores[i]);
    }
  }
  
  //check whether query expansion has to be done
  if(extensions_!=0) {
    
    //if query expansion has to be done:
    // get ranks
    sort(results.rbegin(), results.rend());
    
    vector<ImageContainer*> expansion;

    //copy extensions_ into positive queries
    for(uint i=0;i<extensions_;++i) {
      expansion.push_back(database_[results[i].second]);
    }

    //init result field
    results.clear();
    for(uint i=0;i<N;++i) {
      results.push_back(ResultPair(0.0,i));
    }
    
    // and requery using these positive queries
    for(uint q=0;q<expansion.size();++q) {
      imageComparator_.start(expansion[q]);
      getScores(expansion[q],activeScores);
      imageComparator_.stop();
      for(uint i=0;i<N;++i) {
        results[i].first+=activeScores[i];
      }
    }
  }
}


vector<ResultPair> Retriever::metaretrieve(const string& query) {
  uint N=database_.size();
  uint M=database_.numberOfSuffices();
  vector<ResultPair> result;
  
  //init result field
  /*  for(uint i=0;i<database_.size();++i) {
    result.push_back(ResultPair(0.0,i));
    }*/
  
  MetaFeatureDistance metadist;

  // Find index of metafeature
  string dist_name;
  int metafeatureidx = -1;  
  for(unsigned int i=0; i<M; ++i) {
    dist_name = imageComparator_.distance(i)->name();
    if(dist_name=="metafeature" ){
      metafeatureidx = i;
      break;
    }
  }

  if(metafeatureidx==-1) {
    return vector<ResultPair>(0);
  }
  
  //Now parse the query string and build the corresponding metafeature
  ImageContainer imgcon("temporary query object", M);

  vector<string> tokens;
  const string delimiters = ":,";

  // Tokenizer ripped from the web (what a shame that c++ doesn't have one)
  // Skip delimiters at beginning.
  string::size_type lastPos = query.find_first_not_of(delimiters, 0);

  // Find first "non-delimiter".
  string::size_type pos = query.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(query.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = query.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = query.find_first_of(delimiters, lastPos);
  }

  // Quick sanity check
  if( tokens.size() % 2 > 0 ) {
    DBG(10) << "Invalid metaquery!" << endl;
    return result;
  }
  
  // Another sanity check
  ::std::set< ::std::string > valid_keys;
  valid_keys = getMetaFeatureInfo().first;
  for(unsigned i=0; i<tokens.size(); i+=2) {
    string key = tokens[i];
    if(valid_keys.find(key) == valid_keys.end()) {
      // Alert!
      DBG(10) << "Invalid metaquery!" << endl;
      return result;
    }
  }

  // Now make the metafeature map from the token list
  std::map<std::string,std::string> val;

  string dbgstr = "Metaquery - ";
  string key, value;
  vector<string>::iterator t_it;
  for(t_it = tokens.begin(); t_it!=tokens.end(); ++t_it) {
    key = string(*t_it);
    ++t_it;
    value = string(*t_it);
    val[key] = value;
    dbgstr += key + ":" + value + " ";
  }
  DBG(10) << dbgstr << endl;

  // This is a virtual image that only has a meta feature
  imgcon[metafeatureidx] = new MetaFeature(val);
  
  vector<double> distsToImages(N);
  vector<double> imgDists;
  
  // get distance to each of the database images
  for(uint i=0;i<N;++i) {
    BaseFeature *dbmf= database_[i]->operator[](metafeatureidx);
    distsToImages[i]=metadist.distance(imgcon[metafeatureidx],dbmf);
  }
  
  // normalize
  double sum=0.0;
  for (uint i=0;i<N;++i) {sum+=distsToImages[i];}
  sum/=double(N);
  if(sum!=0.0) {
    double tmp=1/sum;
    for(uint i=0;i<N;++i) {
      distsToImages[i]*=tmp;
    }
  }

  // Compute the scores (this omits the use of a scoring class)
  unsigned max_dist = val.size();
  for(uint i=0;i<N;++i) {
    if(distsToImages[i] < max_dist) {
      result.push_back(ResultPair(exp(-distsToImages[i]),i));
    }
  }

  //check whether query expansion has to be done
  if(extensions_!=0) {
    vector<double> activeScores(N,0.0);
    //if query expansion has to be done:
    
    // get ranks
    sort(result.rbegin(), result.rend());
    
    //reset positive queries (also clear negative queries)
    vector<ImageContainer*> expansion;

    //copy extensions_ into positive queries
    for(uint i=0;i<extensions_;++i) {
      expansion.push_back(database_[result[i].second]);
    }


    //init result field
    result.clear();
    for(uint i=0;i<database_.size();++i) {
      result.push_back(ResultPair(0.0,i));
    }
    
    // and requery using these positive queries
    for(uint q=0;q<expansion.size();++q) {
      imageComparator_.start(expansion[q]);
      getScores(expansion[q],activeScores);
      imageComparator_.stop();
      for(uint i=0;i<N;++i) {
        result[i].first+=activeScores[i];
      }
    }
  }

  return result;
}


vector<ResultPair> Retriever::textretrieve(const string& query) {
  uint N=database_.size();
  uint M=database_.numberOfSuffices();
  vector<ResultPair> result;

  // Where is which textfeature and which langage do they have ?
  string dist_name;
  TextFeatureDistance *tfd;
  map<string,TextFeatureDistance*> textdists;
  map<string,unsigned> textdistindices;
  for(unsigned int i=0; i<M; ++i) {
    dist_name = imageComparator_.distance(i)->name();
    if(dist_name=="textfeature" ){
      tfd = (TextFeatureDistance*)imageComparator_.distance(i);
      textdists[tfd->language()] = tfd;
      textdistindices[tfd->language()] = i;
      cout << "found " << tfd->language() << " at index " << i << endl;
    }
  }

  // Decompose query string in its languages
  cout << "decomposing query" << endl;
  map<string,string> cl_queries;
  map<string,TextFeatureDistance*>::iterator tdi;
  for(tdi=textdists.begin();tdi!=textdists.end();++tdi) {
    string lang = tdi->first;
    string langpart;
    unsigned langpos = query.find(lang);
    unsigned langbegin, langend;
    if(langpos != string::npos) {
      langbegin = langpos + lang.size() + 2;
      langend = query.find('"', langbegin);
      langpart = query.substr(langbegin, langend - langbegin);
      cl_queries[lang] = langpart;
      cout << "Query in " << lang << ": " << langpart << endl;
    }
  }

  // If nothing is found, but the string is non-empty, it's a
  // normal query, not a cross-language one.
  if(cl_queries.size()==0) {
    cout << "normal query" << endl;

    TextFeatureDistance textdist;
    
    // Do the textretriever query
    textdist.query_wmir(query);

    // Find index of textfeature
    // (we assume there is only one, but if there are multiple ones,
    // the last one is taken (for no particular reason))
    int textfeatureidx = -1;  
    for(unsigned int i=0; i<M; ++i) {
      dist_name = imageComparator_.distance(i)->name();
      if(dist_name=="textfeature" ){
	textfeatureidx = i;
	break;
      }
    }
  
    // Make an empty image
    ImageContainer imgcon("temporary query object", M);
    imgcon[textfeatureidx] = new TextFeature();

    vector<double> distsToImages(N);
    //vector<double> imgDists;

    // get distance to each of the database images
    BaseFeature *dbmf;
    for(uint i=0;i<N;++i) {
      dbmf = database_[i]->operator[](textfeatureidx);
      distsToImages[i]=textdist.distance(imgcon[textfeatureidx],dbmf);
    }

    // normalize
    double sum=0.0;
    double normalization_factor = 1.0;
    for (uint i=0;i<N;++i) {sum+=distsToImages[i];}
    sum/=double(N);
    if(sum!=0.0) {
      normalization_factor = 1/sum;
      for(uint i=0;i<N;++i) {
	distsToImages[i]*=normalization_factor;
      }
    }

    // Compute the scores (this omits the use of a scoring class)
    
    // maxval is the normalized dist value for an image that was not
    // in the textretriever's result.
    double maxval = 10000.0 * normalization_factor;

    for(uint i=0;i<N;++i) {
      if(distsToImages[i] != maxval) {
	ResultPair r;
	r.first = exp(-distsToImages[i]);
	r.second = i;
	result.push_back(r);
      }
    }

  } else {
    cout << "x-language query" << endl;

    // Query all distances with the correct queries
    map<string,string>::iterator clqi;
    for(clqi=cl_queries.begin(); clqi!=cl_queries.end(); ++clqi) {
      cout << "querying " << clqi->first << endl;
      textdists[clqi->first]->query_wmir(clqi->second);
    }

    // Make new maps that only contain the textdists and indices of languages that were
    // used for querying
    map<string,TextFeatureDistance*> querytextdists;
    map<string,unsigned> querytextdistindices;
    map<string,TextFeatureDistance*>::iterator tdi;
    map<string,unsigned>::iterator tdii;
    cout << "Only calculating dists for" << endl;
    for(tdi=textdists.begin(), tdii=textdistindices.begin(); tdi != textdists.end(); ++tdi, ++tdii) {
      clqi = cl_queries.find(tdi->first); // Find language in the query
      if(clqi != cl_queries.end()) { // If this language was part of the query, add it
	querytextdists[tdi->first] = tdi->second;
	querytextdistindices[tdii->first] = tdii->second;
	cout << tdii->first << endl;
      }
    }

    // Make an empty image with the queried textfeatures
    cout << "Making empty image" << endl;
    ImageContainer imgcon("temporary query object", M);
    for(tdii=querytextdistindices.begin();tdii!=querytextdistindices.end();++tdii) {
      imgcon[tdii->second] = new TextFeature();
    }
    
    // get distance to each of the database images
    cout << "Getting distances" << endl;
    BaseFeature *dbmf;
    map<string, vector<double> > distsToImages;
    for(tdii=querytextdistindices.begin();tdii!=querytextdistindices.end();++tdii) {
      string tdlang = tdii->first;
      unsigned tdidx = tdii->second;

      distsToImages[tdlang].resize(N, 0.0);
      for(uint i=0;i<N;++i) {
	dbmf = database_[i]->operator[](tdidx);
	distsToImages[tdlang][i]=querytextdists[tdlang]->distance(imgcon[tdidx],dbmf);
      }
    }

    // normalize
    cout << "normalizing" << endl;
    map<string,double> normalization_factors;
    for(tdii=querytextdistindices.begin();tdii!=querytextdistindices.end();++tdii) {
      double sum=0.0;
      normalization_factors[tdii->first] = 1.0;
      
      for (uint i=0;i<N;++i) {sum+=distsToImages[tdii->first][i];}
      sum/=double(N);
      if(sum!=0.0) {
	normalization_factors[tdii->first] = 1/sum;
	for(uint i=0;i<N;++i) {
	  distsToImages[tdii->first][i]*=normalization_factors[tdii->first];
	}
      }
    }

    // Compute the scores (this omits the use of a scoring class)
    
    // maxval is the normalized dist value for an image that was not
    // in the textretriever's result.
    map<string, double> maxvals;
    for(tdii=querytextdistindices.begin();tdii!=querytextdistindices.end();++tdii) {
      maxvals[tdii->first] = 10000.0 * normalization_factors[tdii->first];
    }

    // for each image, calculate the scores for the three features and add them
    double score;
    for(uint i=0;i<N;++i) {
      score = 0.0;
      for(tdii=querytextdistindices.begin();tdii!=querytextdistindices.end();++tdii) {
	if(distsToImages[tdii->first][i] != maxvals[tdii->first]) {
	  score += exp(-distsToImages[tdii->first][i]);
	}
      }
      if(score > 0.0) {
	ResultPair r;
	r.first = score;
	r.second = i;
	result.push_back(r);
      }
    }

  }

  cout << result.size() << " results." << endl;

  return result;
}

::std::pair< ::std::set< ::std::string >,::std::set< ::std::string > >
Retriever::getMetaFeatureInfo() {
   return database_.getMetaFeatureInfo();
}

string Retriever::dist(const uint idx, BaseDistance* dist) {
  imageComparator_.distance(idx,dist);
  ostringstream oss("");
  oss << "dist " << idx << " " << dist->name();
  return oss.str();
}

string Retriever::weight(const uint idx, const double w) {
  LinearScoring* s=dynamic_cast<LinearScoring*>(scorer_);
  ostringstream oss("");
  if(s) {
    s->weight(idx)=w;
    oss << "weight " << idx << " " << w;
  } else {
    oss << "setting weight not supported";
  }
  return oss.str();
}

string Retriever::filelist(const string filelist) { 
  DBG(10) << "Reading filelist: " << filelist << endl;
  database_.clear();
  uint nr=database_.loadFileList(filelist);
  
  DBG(10) << "Loading features for " << database_.size() << " images..."<< endl;
  database_.loadFeatures();
  DBG(10) << "Finished loading features." << endl;
  ostringstream oss("");
  imageComparator_=ImageComparator(database_.numberOfSuffices());
  for(uint i=0;i<database_.numberOfSuffices();++i) {
    imageComparator_.distance(i,new BaseDistance());
  }
  oss << "filelist " << filelist << " " << nr;
  return oss.str();
}

string Retriever::results(const uint res) { 
  results_=res;
  ostringstream oss("");
  oss << "results = " << results_;
  return oss.str();
}

string Retriever::extensions(const uint ext) {
  extensions_=ext;
  ostringstream oss("");
  oss << "extensions = " << extensions_;
  return oss.str();
}

string Retriever::random(uint nr) {
  ostringstream out;
  int rndEntry;
  srand((unsigned) time(0));
  for(uint i=0;i<nr;++i) {
    rndEntry=rand() % database_.size();
    out << database_.filename(rndEntry) << " ";
  }
  return out.str();
}

uint Retriever::results() {
  return results_;
}

BaseDistance* Retriever::dist(const uint idx) {
  return imageComparator_.distance(idx);
}

double Retriever::weight(const uint idx) const {
  LinearScoring* s=dynamic_cast<LinearScoring*>(scorer_);
  double w=0;
  if(s) {
    w=s->weight(idx);
  } else {
    ERR << "Weights not supported in this Scorer!" << endl;
  }
  return w;
}

string Retriever::filelistEntries() const {
  ostringstream oss;
  for(uint i=0;i<database_.size();++i) {
    oss << database_.filename(i) << " ";
  }
  return oss.str();
}

string Retriever::filelist(const uint nr) const {
  return database_.filename(nr);
}

bool Retriever::haveClasses() const {
  return database_.haveClasses();
}

int Retriever::clas(const uint &idx) const {
  const ImageContainer *t=NULL;
  t=database_[idx];
  if(t) {
    return t->clas();
  } else {
    return -1;
  }
}

int Retriever::clas(const string &filename) const {
  const ImageContainer *t=NULL;
  t=database_.getByName(filename);
  if(t) {
    return t->clas();
  } else {
    return -1;
  }
}

::std::string Retriever::info()  const {
  ostringstream oss;
  oss << "filelist " << database_.size() << " "
      << "results " << results_ << " "
      << "path " << database_.path() << " "
      << "extensions " << extensions_ << " " 
      << "scoring " << scorer_->type() << " " << scorer_->settings();
  for(uint i=0;i<imageComparator_.size();++i) {
    oss << " suffix "<< i << " " << database_.suffix(i) << " " <<imageComparator_.distance(i)->name();
  }
  return oss.str();
}

uint Retriever::numberOfFilelistEntries() const {
  return database_.size();
}
