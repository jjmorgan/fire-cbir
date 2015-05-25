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
#include "vectorfeature.hpp"
#include "diag.hpp"

using namespace std;

void VectorFeature::read(istream & is) {
  string line;
  getline(is,line);
  istringstream iss;
  if(!is.good()) {
    ERR << "Error reading" << endl;
    return ;
  }
  
  if(line!="FIRE_vectorfeature") {
    DBG(30) << "Magic number not found, ignoring, because this could be an old file" << endl;
  }
  
  getline(is,line); iss.str(line); string keyword; iss >> keyword;
  if("dim"==keyword) {
    uint size;
    iss >> size;
    data_.resize(size);
  } else {
    ERR << "Expected 'dim', got " << line << endl;
    return;
  }

  getline(is,line); iss.clear(); iss.str(line); iss >> keyword;
  if ("data"==keyword) {
    for(uint i=0;i<data_.size();++i) {
      iss >> data_[i];
    }
  } else {
    ERR << "Expected 'data', got " << line << endl;
  }
}

void VectorFeature::write(ostream &os) {
  os <<"FIRE_vectorfeature" << endl;
  os << "dim " << data_.size() << endl
     << "data ";
  for(uint i=0;i<data_.size();++i) {
    os << data_[i] << " ";
  }
  os << endl;
}

const uint VectorFeature::size() const {
  return data_.size();
}
