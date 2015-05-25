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
#include "imagecontainer.hpp"

using namespace std;

ImageContainer::ImageContainer() : features_() {
}

ImageContainer::ImageContainer(const string& filename, const uint nof) : basename_(filename), features_(nof) {
}
 

ImageContainer::~ImageContainer() {
  for(uint i=0;i<features_.size();++i) {
    delete features_[i];
  }
}


BaseFeature*& ImageContainer::operator[](uint idx) {
  return features_[idx];
}

const BaseFeature* ImageContainer::operator[](uint idx) const {
  return features_[idx];
}


const ::std::string& ImageContainer::basename() const {
  return basename_;
}


const uint& ImageContainer::clas() const {
  return class_;
}

uint& ImageContainer::clas() {
  return class_;
}


const DescriptionSet& ImageContainer::description() const {
  return description_;
}

DescriptionSet& ImageContainer::description() {
  return description_;
}
