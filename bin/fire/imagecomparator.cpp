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
using namespace std;

#include "imagecomparator.hpp"

using namespace std;

ImageComparator::ImageComparator() : distances_(0) {
}

ImageComparator::ImageComparator(uint size)  :distances_(size) {};



ImageComparator::~ImageComparator() {
  for(uint i=0;i<distances_.size();++i) {
    delete distances_[i];
  }
}

void ImageComparator::start(const ImageContainer *query) {
  for(uint i=0;i<distances_.size();++i) {
    distances_[i]->start((*query)[i]);
 }
}

vector<double> ImageComparator::compare(const ImageContainer *queryImage, const ImageContainer* databaseImage) {
  vector<double> result;
  DBG(35) << "Comparing " << queryImage->basename() << " with " << databaseImage->basename() << endl;
  for(uint i=0;i<distances_.size();++i) {
    result.push_back(distances_[i]->distance((*queryImage)[i],(*databaseImage)[i]));
  }
  return result;
}

void ImageComparator::stop() {
  for(uint i=0;i<distances_.size();++i) {
    distances_[i]->stop();
  }
}


void ImageComparator::distance(const uint& idx, BaseDistance* d) {
  if(distances_.size() < idx+1) {
    distances_.resize(idx+1);
  }
  if(d!=distances_[idx]) {delete distances_[idx];}

  distances_[idx]=d;
}

BaseDistance *ImageComparator::distance(const uint& idx) const {
  return distances_[idx];
}


uint ImageComparator::size() const {
  return distances_.size();
}
