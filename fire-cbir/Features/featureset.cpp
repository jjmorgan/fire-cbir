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
#include <algorithm>
#include "featureset.hpp"

FeatureSet::FeatureSet() : features_() {
}

FeatureSet::~FeatureSet() {
  for(uint i=0;i<features_.size();++i) {
    delete features_[i];
  }
}

void FeatureSet::add_feature(BaseFeature* feature) {
  features_.push_back(feature);
}

uint FeatureSet::feature_count() const {
  return features_.size();
}

BaseFeature*& FeatureSet::operator[](uint idx) {
  return features_[idx];
}

const BaseFeature* FeatureSet::operator[](uint idx) const {
  return features_[idx];
}
