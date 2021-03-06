/*
This file is part of the FIRE -- Flexible Image Retrieval System

FIRE is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.

FIRE is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with FIRE; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <iostream>
#include <sstream>
#include "localfeatures.hpp"
#include "diag.hpp"

using namespace std;

void LocalFeatures::discardDimension(const uint d) {
  for(uint i=0;i<data_.size();++i) {
    data_[i][d]=0.0;
  }
}

void LocalFeatures::read(istream &is) {
  string line, keyword;
  uint noffeat;
  istringstream iss;
  getline(is,line);
  if(!is.good()) {
    ERR << "Error reading from stream" << endl;
    return ;
  }
  
  if(line!="FIRE_localfeatures") {
    ERR << "Magicnumber not found, expected localfeatures, got something else" << endl;
    return;
  }
  
  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword== "winsize" ) { iss >> winsize_; } 
  else { ERR << "Expected 'winsize', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="dim" ) { iss >> dim_; }
  else { ERR << "Expected 'dim', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="subsampling" ) { iss >> subsampling_; }
  else { ERR << "Expected 'subsampling', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="padding" ) { iss >> padding_; }
  else { ERR << "Expected 'padding', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="numberOfFeatures" ) { iss >> numberOfFeatures_; }
  else { ERR << "Expected 'numberOfFeatures', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="varthreshold" ) { iss >> varthreshold_; }
  else { ERR << "Expected 'varthreshold', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="zsize" ) { iss >> zsize_; }
  else { ERR << "Expected 'zsize', got " << line << endl; return;}

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="filename" ) { iss >> filename_; }
  else { ERR << "Expected 'filename', got " << line << endl; return;}

  
  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if(keyword=="features" ) { iss >> noffeat; } 
  else { ERR << "Expected 'features', got " << line << endl; return;}

  for(uint i=0;i<noffeat;++i) {
    getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
    if(keyword=="feature"){
      uint w;
      double x, y;
      vector<double> feat(dim_);
      iss >> x >> y >> w;
      for(uint j=0;j<dim_;++j) {
        iss >> feat[j];
      }
      positions_.push_back(pair<uint,uint>((uint) x, (uint) y));
      relativePositions_.push_back(pair<double, double>(x, y));
      extractionSizes_.push_back(w);
      data_.push_back(feat);
    } else { ERR << "Expected 'feature', got " << line << endl; return;} 
  }
  if(data_.size() != noffeat) { ERR << "Strange: noffeat != number of features read: " << noffeat << "!=" << data_.size() << endl;}
}

void LocalFeatures::write(ostream &os) {
  os << "FIRE_localfeatures" << endl
     << "winsize " << winsize_ << endl
     << "dim " << dim_ << endl
     << "subsampling " << subsampling_ << endl
     << "padding " << padding_ << endl
     << "numberOfFeatures " << numberOfFeatures_ << endl
     << "varthreshold " << varthreshold_ << endl
     << "zsize " << zsize_ << endl
     << "filename " << filename_ << endl
     << "features " << data_.size() << endl;
  for(uint i=0;i<data_.size();++i) {
    os << "feature " << relativePositions_[i].first << " " << relativePositions_[i].second << " " << extractionSizes_[i];
    for(uint j=0;j<data_[i].size();++j) {
      os << " " << data_[i][j];
    }
    os << endl;
  }
}

void LocalFeatures::addLocalFeature(const vector<double> &lf, pair<uint, uint> pos, uint extractionSize) {
  data_.push_back(lf);
  positions_.push_back(pos);
  relativePositions_.push_back(pair<double, double>(double(pos.first), double(pos.second)));
  extractionSizes_.push_back(extractionSize);
}

void LocalFeatures::addLocalFeature(const vector<double> &lf, pair<uint, uint> pos, pair<double, double> relpos, uint extractionSize) {
  data_.push_back(lf);
  positions_.push_back(pos);
  relativePositions_.push_back(relpos);
  extractionSizes_.push_back(extractionSize);
}

void LocalFeatures::addLocalFeatures(const LocalFeatures& lfs) {
  for (int i = 0; i < (int) lfs.numberOfFeatures(); i++) {
    addLocalFeature(lfs.data_[i], lfs.position(i), lfs.relativePosition(i), lfs.extractionSize(i));
  }
  numberOfFeatures_ += lfs.numberOfFeatures();
}
