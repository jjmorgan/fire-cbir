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
#include <sstream>
#include "distancemaker.hpp"

using namespace std;

DistanceMaker::DistanceMaker() {
  distanceNames_["base"]=DT_BASE;
  distanceNames_["euclidean"]=DT_EUCLIDEAN;
  distanceNames_["l1"]=DT_L1;
  distanceNames_["cityblock"]=DT_L1;
  distanceNames_["jsd"]=DT_JSD;
  distanceNames_["jd"]=DT_JSD;
  distanceNames_["kld"]=DT_KLD;
  distanceNames_["chisquare"]=DT_CHISQUARE;
  distanceNames_["histogramintersection"]=DT_HISTOGRAMINTERSECTION;
  distanceNames_["his"]=DT_HISTOGRAMINTERSECTION;
  distanceNames_["reldev"]=DT_RELDEV;
  distanceNames_["relativedeviation"]=DT_RELDEV;
  distanceNames_["relbindev"]=DT_RELBINDEV;
  distanceNames_["relativebindeviation"]=DT_RELBINDEV;
  distanceNames_["crosscorrelation"]=DT_CROSSCORRELATION;
  distanceNames_["fidelity"]=DT_ONEMINUSFIDELITY;
  distanceNames_["oneminusfidelity"]=DT_ONEMINUSFIDELITY;
  distanceNames_["sqrtoneminusfidelity"]=DT_SQRTONEMINUSFIDELITY;
  distanceNames_["logtwominusfidelity"]=DT_LOGTWOMINUSFIDELITY;
  distanceNames_["arccosfidelity"]=DT_ARCCOSFIDELITY;
  distanceNames_["sinfidelity"]=DT_SINFIDELITY;
  distanceNames_["oneminusfidelitysquare"]=DT_SINFIDELITY;
  distanceNames_["idm"]=DT_IDM;
  distanceNames_["binary"]=DT_BINARYFEATURE;
  distanceNames_["binaryfeature"]=DT_BINARYFEATURE;
  distanceNames_["globallocalfeaturedistance"]=DT_GLFD;
  distanceNames_["metafeature"]=DT_METAFEATURE;
  distanceNames_["textfeature"]=DT_TEXTFEATURE;
  distanceNames_["glfd"]=DT_GLFD;
  distanceNames_["mpeg7"]=DT_MPEG7;
  distanceNames_["histpair"]=DT_HISTOPAIR;
  distanceNames_["lfhungarian"]=DT_LFHUNGARIAN;
  distanceNames_["distfile"]=DT_DISTFILE;
  distanceNames_["facefeature"]=DT_FACEFEAT;
  distanceNames_["facefeat"]=DT_FACEFEAT;
  //tbc
  
  defaultDistances_[FT_BASE]=DT_BASE;
  defaultDistances_[FT_IMG]=DT_EUCLIDEAN;
  defaultDistances_[FT_VEC]=DT_EUCLIDEAN;
  defaultDistances_[FT_HISTO]=DT_JSD;
  defaultDistances_[FT_LF]=DT_GLFD;
  defaultDistances_[FT_BINARY]=DT_BINARYFEATURE;
  defaultDistances_[FT_META]=DT_METAFEATURE;
  defaultDistances_[FT_MPEG7]=DT_MPEG7;
  defaultDistances_[FT_HISTOPAIR]=DT_HISTOPAIR;
  defaultDistances_[FT_SPARSEHISTO]=DT_JSD;
  defaultDistances_[FT_OLDHISTO]=DT_JSD;
  defaultDistances_[FT_META]=DT_METAFEATURE;
  defaultDistances_[FT_TEXT]=DT_TEXTFEATURE;
  defaultDistances_[FT_DISTFILE]=DT_DISTFILE;
  defaultDistances_[FT_FACEFEAT]=DT_FACEFEAT;
  //tbc
}


BaseDistance* DistanceMaker::makeDistance(const ::std::string& distancename) {
  BaseDistance* result; 
  string dn, par;

  uint pos=distancename.find(":");
  if(pos<distancename.size()) {
    dn=distancename.substr(0,pos);
    par=distancename.substr(pos+1,distancename.size()-1);
  } else {
    dn=distancename;
    par="";
  }
  
  //  cout << pos << " " << dn <<" "<< par<<  endl;
  
  if(distanceNames_.find(dn)==distanceNames_.end()) {
    ERR << "No such distance defined: " << distancename << ::std::endl;
    result=new BaseDistance();
  } else {
    result=makeDistance(distanceNames_[dn],par);
  }
  return result;
}


BaseDistance* DistanceMaker::getDefaultDistance(const FeatureType featType) {
  BaseDistance* result; 
  if(defaultDistances_.find(featType)==defaultDistances_.end()) {
    ERR << "No default distance for FeatureType " << featType << " defined." << ::std::endl;
    result=new BaseDistance();
  } else {
    result=makeDistance(defaultDistances_[featType]);
  }
  return result;
}

/// function returning the integer which follows the string t in string s.
/// examples:
///    getIntAfter("egal=7asdf","egal=",1) will return 7
///    getIntAfter("egal=7asdf","doof=",1) will return 1, as the string doof= is not found

const int getIntAfter(const string& s, const string& t, const int def) {
  int val=def;

  uint pos=s.find(t);
  string INTstring="";
  if(pos<s.size()) {
    for(uint i=pos+t.size();s[i]>='0' && s[i]<='9' && i<s.size() ; ++i) {
      INTstring+=s[i];
    }
    ::std::istringstream istr(INTstring);
    istr >> val;
  }
  return val;
}

/// function returning the double which follows the string t in string s.
/// examples:
///    getDoubleAfter("egal=7asdf","egal=",1) will return 7
///    getDoubleAfter("egal=7asdf","doof=",1) will return 1, as the string doof= is not found

const double getDoubleAfter(const string& s, const string& t, const double def) {
  double val=def;

  uint pos=s.find(t);
  string DOUBLEstring="";
  if(pos<s.size()) {
    for(uint i=pos+t.size();(s[i]>='0' && s[i]<='9' || s[i]=='.') && i<s.size(); ++i) {
      DOUBLEstring+=s[i];
    }
    ::std::istringstream istr(DOUBLEstring);
    istr >> val;
  }
  return val;
}

/// function returning the string which follows the string t in string s.
/// delimiter can be set
/// examples:
///    getDoubleAfter("egal=7asdf","egal=",1) will return 7
///    getDoubleAfter("egal=7asdf","doof=",1) will return 1, as the string doof= is not found

const string getStringAfter(const string& s, const string& t, const string& def, const char delimiter=':') {
  string val=def;
  
  uint pos=s.find(t);
  string STRINGstring="";
  if(pos<s.size()) {
    for(uint i=pos+t.size();s[i]!=delimiter && i<s.size() ; ++i) {
      STRINGstring+=s[i];
    }
    val=STRINGstring;
  }
  return val;
}

///function returning whether a certain string is part of another string
const bool getBooleanString(const string& s, const string &t) {
  return s.find(t)<s.size();
}

BaseDistance* DistanceMaker::makeDistance(const DistanceType distType, const string &par) {
  BaseDistance* result; 
  switch(distType) {
  case DT_BASE:
    result=new BaseDistance();
    break;
  case DT_EUCLIDEAN:
    result=new EuclideanDistance();
    break;
  case DT_CROSSCORRELATION: {
    int d=getIntAfter(par,"D=",4);
    result=new CrosscorrelationDistance(d);
    break;
  }
  case DT_L1:
    result=new L1Distance();
    break;
  case DT_JSD:{
    double smoothFactor=getDoubleAfter(par,"SF=",0.5);
    result=new JSDDistance(smoothFactor);
    break;}
  case DT_KLD:
    result=new KLDDistance();
    break;
  case DT_CHISQUARE:
    result=new ChisquareDistance();
    break;
  case DT_HISTOGRAMINTERSECTION:
    result=new HistogramintersectionDistance();
    break;
  case DT_RELDEV:
    result=new RelativeDeviationDistance();
    break;
  case DT_RELBINDEV:
    result=new RelativeBinDeviationDistance();
    break;
  case DT_ONEMINUSFIDELITY:
    result=new OneMinusFidelityDistance();
    break;
  case DT_SQRTONEMINUSFIDELITY:{
    result=new SqrtOneMinusFidelityDistance();
    break;}
  case DT_LOGTWOMINUSFIDELITY:{
    result=new LogTwoMinusFidelityDistance();
    break;}
  case DT_ARCCOSFIDELITY:{
    result=new ArccosFidelityDistance();
    break;}
  case DT_SINFIDELITY:{
    result=new SqrtOneMinusFidelityDistance();
    break;}
  case DT_BINARYFEATURE:{
    result=new BinaryFeatureDistance();
    break;}
  case DT_METAFEATURE:{
    result=new MetaFeatureDistance();
    break;}
  case DT_TEXTFEATURE:{
    string serverstr=getStringAfter(par,"SERVER=","localhost");
    int portno=getIntAfter(par,"PORT=",4242);
    string language=getStringAfter(par,"LANG=","None");
    result=new TextFeatureDistance(serverstr, portno, language);
    break;}
  case DT_IDM: {
    int wr1=getIntAfter(par,"WR1=",3);
    int wr2=getIntAfter(par,"WR2=",1);
    double threshold=getDoubleAfter(par,"TH=",0.05);
    bool sobel=not getBooleanString(par,"NOSOBEL");
    
    result=new ImageDistortionModelDistance(wr1,wr2,threshold,sobel);
    break;
  }

  case DT_GLFD: {
    string treename=getStringAfter(par,"TREE=","tree.kdt");
    uint k=getIntAfter(par,"K=",10);
    double epsilon=getDoubleAfter(par,"EPS=",0.1);
    
    result=new GlobalLocalFeatureDistance(treename,k,epsilon);
    break;}

  case DT_MPEG7:{
    string xmmainpath=getStringAfter(par,"XMMAIN=","./XMMain.exe");
    string mpegdatapath=getStringAfter(par,"MPD=","mpegdata");
    
    result=new MPEG7Distance(xmmainpath,mpegdatapath);
    break;}
  case DT_HISTOPAIR:{
    string grounddistname=getStringAfter(par,"GDIST=","jsd");
    double centerweight=getDoubleAfter(par,"CWEIGHT=",0.5);
    
    result=new HistogramPairDistance(makeDistance(grounddistname), centerweight);
    break;}
  case DT_DISTFILE:{
    string scoringname=getStringAfter(par,"SCORING=","linear",' ');
    result=new DistanceFileDistance(scoringname);
    break;
  }
 case DT_LFHUNGARIAN:{
    result=new LFHungarianDistance();
    break;
  }
  case DT_FACEFEAT: {
    string allOrOne=getStringAfter(par,"USE=","one");
    result=new FaceFeatureDistance(allOrOne);
    break;
  }
  default:
    ERR << "Distancetype not known: " << distType << endl;
    result=new BaseDistance();
    break;
  }
  return result;
}

string DistanceMaker::availableDistances() const {
  ostringstream oss("");
    
  for(map<const string,DistanceType>::const_iterator i=distanceNames_.begin();i!=distanceNames_.end();++i) {
    oss << i->first << " ";
  }
  return oss.str();
}
