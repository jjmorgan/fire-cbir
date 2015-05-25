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
#include "featureloader.hpp"
#include "vectorfeature.hpp"
#include "imagefeature.hpp"
#include "histogramfeature.hpp"
#include "sparsehistogramfeature.hpp"
#include "binaryfeature.hpp"
#include "localfeatures.hpp"
#include "metafeature.hpp"
#include "textfeature.hpp"
#include "histogrampairfeature.hpp"
#include "mpeg7feature.hpp"
#include "distancefilefeature.hpp"
#include "facefeature.hpp"
                   
using namespace std;

FeatureLoader::FeatureLoader() {
  map_["png"]=FT_IMG;
  map_["jpg"]=FT_IMG;
  map_["jpg"]=FT_IMG;
  map_["vec"]=FT_VEC;
  map_["histo"]=FT_HISTO;
  map_["lf"]=FT_LF;
  map_["gab"]=FT_GABOR;
  map_["blb"]=FT_BLOBS;
  map_["reg"]=FT_REGIONS;
  map_["oldhisto"]=FT_OLDHISTO;
  map_["oldvec"]=FT_VEC;
  map_["bin"]=FT_BINARY;
  map_["txt"]=FT_META;
  map_["mp7"]=FT_MPEG7;
  map_["histopair"]=FT_HISTOPAIR;
  map_["histopairs"]=FT_HISTOPAIR;
  map_["sparsehisto"]=FT_SPARSEHISTO;
  map_["textID"]=FT_TEXT;
  map_["dists"]=FT_DISTFILE;
  map_["facefeat"]=FT_FACEFEAT;
  map_["faces"]=FT_FACEFEAT;
}
  

FeatureType FeatureLoader::suffix2Type(const ::std::string& suffix) const {
  string lastSuffix=suffix.substr(suffix.rfind(".")+1,suffix.size());
  DBG(60) << VAR(lastSuffix) << endl;
  if(lastSuffix=="gz") { 
    uint posPoint=suffix.rfind(".");
    uint posPoint2=suffix.rfind(".",posPoint-1);
    lastSuffix=suffix.substr(posPoint2+1,posPoint-posPoint2-1);
  }
  DBG(50) << VAR(lastSuffix) << endl;
  FeatureType result=0;
  if(map_.find(lastSuffix) == map_.end()) {
    ERR << "No filetype assiciated with this suffix: " << lastSuffix << endl;
  } else {
    result=map_.find(lastSuffix)->second;    
  }
  DBG(50) << VAR(result) << endl;
  return result;
}

BaseFeature* FeatureLoader::load(::std::string basename, ::std::string suffix, ::std::string path) {
    
  BaseFeature *result=NULL;
  string filename=path+"/"+basename+"."+suffix;
  
  FeatureType type=suffix2Type(suffix);
  DBG(50) << VAR(type) << endl;

  switch(type) {
  case FT_IMG:
    DBG(35) << "Loading ImageFeature." << endl;
    result=new ImageFeature();
    result->load(filename);
    DBG(35) << "ImageFeature with " << dynamic_cast<ImageFeature*>(result)->size() << " values loaded" << endl;
    break;
  case FT_VEC:
    DBG(35) << "Loading VectorFeature." << endl;
    result=new VectorFeature();
    result->load(filename);
    DBG(35) << "VectorFeature with " << dynamic_cast<VectorFeature*>(result)->size() << " elements loaded" << endl;
    break;
  case FT_HISTO:
    DBG(35) << "Loading HistogramFeature." << endl;
    result=new HistogramFeature();
    result->load(filename);
    DBG(35) << "HistogramFeature with " << dynamic_cast<HistogramFeature*>(result)->size() << " elements loaded" << endl;
    break;
  case FT_SPARSEHISTO:
    DBG(35) << "Loading SparseHistogramFeature " << endl;
    result=new SparseHistogramFeature();
    result->load(filename);
    DBG(35) << "SparseHistogramFeature with " << dynamic_cast<SparseHistogramFeature*>(result)->size() << " elements loaded" << endl;
    break;
  case FT_OLDHISTO:
    DBG(35) << "Loading HistogramFeature of old type." << endl;
    result=new HistogramFeature();
    dynamic_cast<HistogramFeature*>(result)->loadOld(filename);
    DBG(35) << "HistogramFeature with " << dynamic_cast<HistogramFeature*>(result)->size() << " elements loaded oldstyle." << endl;
    break;
  case FT_BINARY:
    DBG(35) << "Loading BinaryFeature." << endl;
    result=new BinaryFeature();
    result->load(filename);
    DBG(35) << "BinaryFeature loaded." << endl;
    break;
  case FT_LF:
    DBG(35) << "Loading LocalFeatures." << endl;
    result=new LocalFeatures();
    result->load(filename);
    DBG(35) << "LocalFeatures loaded." << endl;
    break;
  case FT_META:
    DBG(35) << "Loading MetaFeature." << endl;
    result=new MetaFeature();
    result->load(filename);
    DBG(35) << "MetaFeature Loaded." << endl;
    break;
  case FT_TEXT:
    DBG(35) << "Loading TextFeature." << endl;
    result=new TextFeature();
    result->load(filename);
    DBG(35) << "TextFeature Loaded." << endl;
    break;
  case FT_MPEG7:
    DBG(35) << "Setting up MPEG7Feature (this is not loading, but only setting up)." << endl;
    result=new MPEG7Feature();
    dynamic_cast<MPEG7Feature*>(result)->setMPEG7Type(suffix);
    dynamic_cast<MPEG7Feature*>(result)->basename()=basename;
    DBG(35) << "MPEG7Feature set up." << endl;
    break;
  case FT_HISTOPAIR:
    DBG(35) << "Loading HistogramPairFeature." << endl;
    result=new HistogramPairFeature();
    result->load(filename);
    DBG(35) << "HistogramPairFeature Loaded." << endl;
    break;
  case FT_DISTFILE:
    DBG(35) << "Not loading DistanceFileFeature." << endl;
    result=new DistanceFileFeature(filename);
    //result->load(filename);
    DBG(35) << "DistanceFileFeature not loaded." << endl;
    break;
  case FT_FACEFEAT:
    DBG(35) << "Loading FaceFeatureFile" << endl;
    result=new FaceFeature();
    result->load(filename);
    DBG(35) << "FaceFeatureFile loaded." << endl;
    break;
  case FT_GABOR:
  case FT_BLOBS:
  case FT_REGIONS:
    
    ERR << "This feature type is not yet implemented:" <<suffix << endl;
  default:
    ERR << "This feature type is not yet known:" << suffix << endl;
  }
  return result;
}
