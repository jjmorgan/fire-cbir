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
#ifndef __distancefilefeature_hpp__
#define __distancefilefeature_hpp__
#include "diag.hpp"
#include <string>
#include <fstream>

class DistanceFileFeature: public VectorVectorFeature {
private:
  uint N_,M_;
  ::std::string filename_;
public:
  DistanceFileFeature(const ::std::string & fn): N_(0), M_(0), filename_(fn) {
    type_ = FT_DISTFILE;
  }
  
  virtual ~DistanceFileFeature() {}
 
  void clear() {
    data_.clear();
  }
  
  void loadYourself() {
    this->load(filename_);
  }

  void DistanceFileFeature::read(::std::istream &is) {
    ::std::istringstream iss;
    ::std::string line,token;

    while('#'==is.peek()) {
      getline(is,line);
    }

    getline(is,line);iss.clear(); iss.str(line); iss >> token;
    if(token!="nofdistances") {
      ERR << "not a valid distance file" << ::std::endl
          << " got '" << token << "' instead of 'nofdistances'." << ::std::endl;
      exit(10);
    } else {
      iss >> N_ >> M_;
    }

    data_=::std::vector< ::std::vector<double> >(N_, ::std::vector<double>(M_,0.0));
    uint icheck;
    for(uint i=0;i<N_;++i) {
      getline(is,line);iss.clear(); iss.str(line); 
      iss >> icheck;
      if(icheck != i) {DBG(10) << "Strange: " << icheck << "!=" <<i << ::std::endl;}
      for(uint j=0;j<M_;++j) {
        iss >> data_[i][j];
      }
    }
  }
  
  void DistanceFileFeature::write(::std::ostream &os) {
    os << "# distmatrix from DistanceFileFeature" << ::std::endl
       << "nofdistances " << N_ << " " << M_ << ::std::endl;
    for(uint i=0;i<N_;++i) {
      os << i;
      for(uint j=0;j<M_;++j) {
        os << " " <<data_[i][j];
      } os << ::std::endl;
    }
  }
  
};


#endif
