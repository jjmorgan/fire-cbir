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
#include <sstream>
#include <algorithm>
#include "database.hpp"
#include "gzstream.hpp"
#include "imagefeature.hpp"
#include "vectorfeature.hpp"
#include "localfeatures.hpp"
#include "binaryfeature.hpp"
#include "metafeature.hpp"
#include "textfeature.hpp"
#include "largefeaturefile.hpp"
#include "mpeg7feature.hpp"
#include "histogramfeature.hpp"
#include "sparsehistogramfeature.hpp"
#include "histogrampairfeature.hpp"
#include "distancefilefeature.hpp"
#include "facefeature.hpp"

using namespace std;

Database::~Database() {
  this->clear();
}

void Database::clear() {
  for(uint i=0;i<database_.size();++i) {
    delete database_[i];
  }
  database_.clear();
  suffixList_.clear();
}

uint Database::loadFileList(::std::string filelist) {
  igzstream is; is.open(filelist.c_str());
  if(!is.good()) {
    ERR << "Unable to open filelist '"<< filelist << "'."<< endl;
    return 0;
  } else {
    string line;
    
    // process filelist file
    getline(is,line);
    if(line!="FIRE_filelist") {
      ERR << "File not a valid filelist file: '" << filelist << "'." << endl;
      return 0;
    }
    while(!is.eof()) {
      if('#'==is.peek()) { // comment line
        getline(is,line); 
      } else { // not a comment line
        getline(is,line);
        if(!is.eof()) {
          istringstream iss(line);
          string keyword;
          iss >> keyword;
          if("path"==keyword) {
            iss >> path_;
            if(path_ == "this") {
              string tmp;
              tmp.assign(filelist,0,filelist.rfind("/")+1);
              if(tmp.find("/")< tmp.size()) {
                path_=tmp;
              } else {
                path_.assign(GetCurrentWorkingDirectory(), 0, GetCurrentWorkingDirectory().size()); 
              }
            }
            if(path_.find("this+")<path_.size()) {
              string suffix;
              string tmp;
              suffix.assign(path_,path_.find("this+")+5,path_.size());
              DBG(60) << VAR(suffix) << endl;
              tmp.assign(filelist,0,filelist.rfind("/")+1);
              DBG(60) << VAR(tmp) << endl; 
              if(tmp.find("/")<tmp.size()) {
                path_=tmp;
              } else {
                path_.assign(GetCurrentWorkingDirectory(), 0, GetCurrentWorkingDirectory().size());
              }
              DBG(60) << VAR(path_) << endl;
              path_+="/";
              path_+=suffix;
              path_+="/";
              DBG(60) << VAR(path_) << endl;
            }
          } else if("file"==keyword) {
            string filename;
            iss >> filename;
            ImageContainer* ic=new ImageContainer(filename, suffixList_.size());
            uint classe;
            string description;
            if(classes_) {
              iss >> classe;
              ic->clas()=classe;
              DBG(25) << VAR(ic->clas()) << " " << VAR(classe) << endl;
            }
            if(descriptions_) {
              string word;
              while(! iss.eof()) {
                iss >> word;
                ic->description().insert(word);
              }
            }
            database_.push_back(ic);
            name2IdxMap_[filename]=(database_.size()-1);
          } else if("suffix"==keyword) {
            string suffix;
            iss >> suffix;
            suffixList_.push_back(suffix);
            DBG(10) << "suffix[" << suffixList_.size()-1 << "]=" << suffixList_[suffixList_.size()-1] << endl;
          } else if("largefeaturefiles"==keyword) {
            string yesno;
            iss >> yesno;
            if("yes"==yesno || "true"==yesno) {
              largefeaturefiles_=true;
            } else {
              largefeaturefiles_=false;
            }
          } else if("classes"==keyword) {
            string yesno;
            iss >> yesno;
            if("yes"==yesno || "true"==yesno) {
              classes_=true;
            } else {
              classes_=false;
            }
          } else if("descriptions"==keyword) {
            string yesno;
            iss >> yesno;
            if("yes"==yesno || "true"==yesno) {
              descriptions_=true;
            } else {
              descriptions_=false;
            }
          } else if("featuredirectories"==keyword) {
            string yesno;
            iss >> yesno;
            if("yes"==yesno || "true"==yesno) {
              featuredirectories_=true;
            } else {
              featuredirectories_=false;
            }
          } else {
            ERR << "Unknown keyword in reading filelist '"<< filelist <<"':" << keyword << endl;
          }
        } // not eof
      } // else (not a comment line)
    }
    return this->size();
    ERR << "Read " << this->size() << " filenames" << endl;
  }
  ERR << "How can I be at this position?" << endl;
  return 0;
}


bool Database::loadQuery(const string& filename, ImageContainer *result) {
  bool featuresOK=true;
  for(uint j=0;j<numberOfSuffices();++j) {
    string path=path_;
    if(featuredirectories()) {
      path+="/"+suffixList_[j];
    }
    result->operator[](j)=fl.load(filename,suffixList_[j],path);
    if(!checkConsistency(database_[0]->operator[](j),result->operator[](j))) {
      featuresOK=false;
      DBG(10) << "loading feature " << j << ":"  << suffixList_[j] << ": features for " 
              << "0:" << database_[0]->basename() << " and queryfeature " 
              <<  filename << " are not consistent." << endl;
    }
  }
  return featuresOK;
}

void Database::loadFeatures() {
  uint percent10=max(int(database_.size()/10),1);

  if(!largefeaturefiles_) { //old style: no large feature files
    for(uint i=0;i<database_.size();++i) {
      if(i%percent10==0) DBG(10) << i << " images loaded." << endl;
      DBG(25) << "Loading features from " << i <<":'" << database_[i]->basename() << "'";
      BLINK(30) << ": ";
      for(uint j=0;j<suffixList_.size();++j) {
        string path=path_;
        if(featuredirectories_) {
          path+="/"+suffixList_[j];
        }
        BLINK(30) << suffixList_[j] << " ";
        database_[i]->operator[](j)=fl.load(database_[i]->basename(),suffixList_[j],path);
        if(!checkConsistency(database_[0]->operator[](j), database_[i]->operator[](j))) {
          DBG(10) << "loading feature " << j << ":"  << suffixList_[j] << ": features for " 
                  << "0:" << database_[0]->basename() << " and " 
                  <<  i <<":" << database_[i]->basename() << " are not consistent." << endl;
        }
      }
      BLINK(25) << endl;
    }
  } else {   // new style: large feature files
    for(uint j=0;j<suffixList_.size();++j) {
      string filename=path_+"/"+suffixList_[j]+".lff";
      LargeFeatureFile lff(filename);
      DBG(10) << "Loading all features for suffix " << suffixList_[j] << " from file " << filename << endl;
      for(uint i=0;i<database_.size();++i) {
        lff.readNext(database_[i],j);
        
        if(!checkConsistency(database_[0]->operator[](j), database_[i]->operator[](j))) {
          DBG(10) << "loading feature " << j << ":"  << suffixList_[j] << ": features for " 
                  << "0:" << database_[0]->basename() << " and " 
                  <<  i <<":" << database_[i]->basename() << " are not consistent." << endl;
        }
      }
      lff.closeReading();
    }
  }
  DBG(10) <<  database_.size() <<" images in database." << endl;
}

bool Database::checkConsistency(const BaseFeature *ref, const BaseFeature *test) const {
  bool result=false;

  if(ref->type() != test->type()) {
    ERR << "Features of different types, probably wrong" << endl;
    return false;
  }

  switch(ref->type()) {
  case FT_BASE:{
    ERR << "Base features are never consistent" << endl;
    result=false;
    break;}
  case FT_IMG:{
    const ImageFeature *r_im=dynamic_cast<const ImageFeature*>(ref);
    const ImageFeature *t_im=dynamic_cast<const ImageFeature*>(test);
    if(r_im && t_im) {
      if(r_im->xsize() == t_im->xsize() && r_im->ysize() == t_im->ysize() && r_im->zsize()==t_im->zsize()) {
        DBG(30) << " image features of same size: OK" << endl;
        result=true;
      } else {
        DBG(28)  <<" image features of different sizes: Probably OK" << endl;
        result=true;
      }
    }
    break;}
  case FT_VEC:{
    const VectorFeature *r_vec=dynamic_cast<const VectorFeature*>(ref);
    const VectorFeature *t_vec=dynamic_cast<const VectorFeature*>(test);
    if(r_vec && t_vec) {
      if(r_vec->size() == t_vec->size()) {
        DBG(30)  <<" vector features of same size "<< r_vec->size() << ": OK" << endl;
        result=true;
      } else {
        DBG(15)  <<" vector features of different sizes: "<< r_vec->size() << " "<< t_vec->size()<< ": Probably WRONG" << endl;
        result=false;
      }
    }
    break;}
  case FT_META:{
    const MetaFeature *r_mf=dynamic_cast<const MetaFeature*>(ref);
    const MetaFeature *t_mf=dynamic_cast<const MetaFeature*>(test);
    if(r_mf && t_mf) {
      result=true;
    }
    break;}
   case FT_TEXT:{
    const TextFeature *r_tf=dynamic_cast<const TextFeature*>(ref);
    const TextFeature *t_tf=dynamic_cast<const TextFeature*>(test);
    if(r_tf && t_tf) {
      result=true;
    }
    break;}
  case FT_HISTO:{
    const HistogramFeature *r_histo=dynamic_cast<const HistogramFeature*>(ref);
    const HistogramFeature *t_histo=dynamic_cast<const HistogramFeature*>(test);
    if(r_histo && t_histo) {
      if(r_histo->size() == t_histo->size()) {
        DBG(30)  <<" histo features of same size "<< r_histo->size() << ": OK" << endl;
        result=true;
      } else {
        DBG(15)  <<" histo features of different sizes: "<< r_histo->size() << " "<< t_histo->size()<< ": Probably WRONG" << endl;
        result=false;
      }
    }      
    break;}
  case FT_SPARSEHISTO: {
    const SparseHistogramFeature *r_histo=dynamic_cast<const SparseHistogramFeature*>(ref);
    const SparseHistogramFeature *t_histo=dynamic_cast<const SparseHistogramFeature*>(test);
    // TODO: implement an exhaustive consistency check for sparse histogram features
    if(r_histo && t_histo) {
      result = true;
    } else {
      result = false;
    }
    break;
  }
  case FT_LF:{
    const LocalFeatures *r_lf=dynamic_cast<const LocalFeatures*>(ref);
    const LocalFeatures *t_lf=dynamic_cast<const LocalFeatures*>(test);
    if(r_lf && t_lf) {
      if(r_lf->dim()==t_lf->dim()) {
        DBG(30) << "LocalFeatures: dim=" << r_lf->dim() << ": OK" << endl;
        result=true;
      } else {
        DBG(15) << "LocalFeatures of different dimensionalities: " << r_lf->dim() << "!=" << t_lf->dim() << ": Probably Wrong" << endl;
        result=false;
      }
    }
    break;}
  case FT_MPEG7:{
    const MPEG7Feature *r_mp7=dynamic_cast<const MPEG7Feature*>(ref);
    const MPEG7Feature *t_mp7=dynamic_cast<const MPEG7Feature*>(test);
    if(r_mp7 && t_mp7) {
      result=r_mp7->type()==t_mp7->type();
    }
    break;}
  case FT_BINARY:{
    const BinaryFeature* r_bin=dynamic_cast<const BinaryFeature*>(ref);
    const BinaryFeature* t_bin=dynamic_cast<const BinaryFeature*>(test);
    if(r_bin && t_bin) {
      DBG(30)  <<" binary features: OK" << endl;
      result=true;
    }
    break;}
  case FT_HISTOPAIR:{
    const HistogramPairFeature* r_histopair=dynamic_cast<const HistogramPairFeature*>(ref);
    const HistogramPairFeature* t_histopair=dynamic_cast<const HistogramPairFeature*>(test);
    if(r_histopair && t_histopair) {
      result=true;
//      if( r_histopair->histo(0).size() == t_histopair->histo(0).size()) {
//        result=true;
//      } else {
//        result=false;
//      }
    }else {
      result=false;}
    break;}
    
  case FT_DISTFILE:{
    const DistanceFileFeature* r_dfile=dynamic_cast<const DistanceFileFeature*>(ref);
    const DistanceFileFeature* t_dfile=dynamic_cast<const DistanceFileFeature*>(test);
    if(r_dfile && t_dfile) {
      result=true;
    } else {
      result=false;
    }
    break;
  }
  case FT_FACEFEAT: {
    const FaceFeature* r_dfile=dynamic_cast<const FaceFeature*>(ref);
    const FaceFeature* t_dfile=dynamic_cast<const FaceFeature*>(test);
    if(r_dfile && t_dfile) {
      result=true;
    } else {
      result=false;
    }
    break;
  }
  case FT_VECTORVECTOR:
  case FT_GABOR:
  case FT_BLOBS:
  case FT_REGIONS:
  default:
    ERR << "These features are not yet implemented, thus we cannot check them" << endl;
    result=false;
    break;
  }
  return result;
}

uint Database::numberOfSuffices() const {
  return suffixList_.size();
}

string Database::filename(const uint idx) const {
  return database_[idx]->basename();
}

ImageContainer* Database::getByName(const string &filename) const {
  ImageContainer * result=NULL;

  if(name2IdxMap_.find(filename) != name2IdxMap_.end()) {
    uint i=name2IdxMap_.find(filename)->second;
    result=database_[i];
  }

  return result;
}

bool Database::haveClasses() const {
  return classes_;
}

::std::pair< ::std::set< ::std::string >,
             ::std::set< ::std::string > >
Database::getMetaFeatureInfo() const {
  // Find the index of the metafeature
  int metafeatureidx = -1;
  ImageContainer* ic = database_[0];
  for(unsigned i=0; i<ic->numberOfFeatures(); ++i) {
    if(ic->operator[](i)->type() == FT_META) {
      metafeatureidx = i;
    }
  }

  // Compile a list of available tags
  MetaFeature* mf;
  ::std::set< ::std::string > res1, res2;
  ::std::set< ::std::string >::iterator res_it;
  ::std::map< std::string,std::string >::const_iterator mi;

  for(unsigned i=0; i<database_.size(); ++i) {
    mf = (MetaFeature*)database_[i]->operator[](metafeatureidx);
    for(mi=mf->values().begin(); mi!=mf->values().end(); ++mi) {
      if( res1.find(mi->first) == res1.end() ) {
	res1.insert(mi->first);
	res2.insert(mi->second);
      }
    }
  }

   return make_pair(res1, res2);
}
