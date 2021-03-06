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

#ifndef __localfeatures_hpp__
#define __localfeatures_hpp__

#include <string>
#include <iostream>
#include <vector>
#include "vectorvectorfeature.hpp"


class LocalFeatures : public VectorVectorFeature {
private:
  uint winsize_, subsampling_, padding_, numberOfFeatures_, dim_, zsize_;
  double varthreshold_;
  ::std::string filename_;
  ::std::vector< ::std::pair<uint, uint> > positions_;
  ::std::vector< ::std::pair<double, double> > relativePositions_;
  ::std::vector< uint > extractionSizes_;
  
public:

  LocalFeatures() : winsize_(0), subsampling_(0), padding_(0), numberOfFeatures_(0), zsize_(0), 
                    varthreshold_(0.0), filename_(""), positions_(::std::vector< ::std::pair<uint, uint> >()),
		    relativePositions_(::std::vector< ::std::pair<double, double> >()),
                    extractionSizes_(::std::vector<uint>()) {
    type_=FT_LF;
  } 

  
  /// destructor
  virtual ~LocalFeatures() {}

  //inherited from BaseFeature
  //virtual void load(const ::std::string &filename);
  //virtual void save(const ::std::string &filename);

  /// read local features / patches from the given istream
  virtual void read(::std:: istream & is);

  /// write local features / patches to the given istream
  virtual void write(::std::ostream & os); 

  // add a patch
  void addLocalFeature(const ::std::vector<double> &lf, ::std::pair<uint, uint> pos, uint extractionSize);
  void addLocalFeature(const ::std::vector<double> &lf, ::std::pair<uint, uint> pos, ::std::pair<double, double> relpos, uint extractionSize);

  // add multiple patches
  void addLocalFeatures(const LocalFeatures& lfs);  

  /// give the position of the idx-th patch in this set
  const ::std::pair<uint, uint>& position(uint idx) const {return positions_[idx];}
  const ::std::pair<double, double>& relativePosition(uint idx) const { return relativePositions_[idx];}

  /// give the extraction size of the idx-th patch in this set
  const uint& extractionSize(uint idx) const { return extractionSizes_[idx]; }
  
  /// give the winsize of the saved patches (if patches from different
  /// sizes are extracted, here we expect the winsize of the scaled/stored patches
  const uint& winsize() const {return winsize_;}
 
  /// get the depth of the patches
  const uint& zsize() const {return zsize_;}

  /// get/set the depth of the patches
  uint& zsize()  {return zsize_;}

  const uint& dim() const {return dim_;}

  /// has subsampling been used for the extraction? if each pixel was
  /// considered this is 0, otherwise each subsampling()+1 pixel was
  /// considered
  const uint& subsampling() const {return subsampling_;}

  /// if padding was used -> 1, 0 otherwise 
  const uint& padding() const {return padding_;}

  /// is the number of features which have been extracted
  const uint& numberOfFeatures() const {return numberOfFeatures_;}

  /// the variance threshold which was used
  const double& varthreshold() const {return varthreshold_;}

  /// the filename of the image where the features have been extracted
  /// from. This is used in GlobalLocalFeatureDistance
  const ::std::string& filename() const {return filename_;}

  /// function to delete certain dimensions from the features
  /// (i.e. set to zero).  useful to discard dimension 0, which
  /// usually accounts for brightness.
  void discardDimension(const uint d);
  

  ///  return the number of local features stored, somehow redundant
  const uint size() const {return data_.size();}

  /// position of the idx-th patch
  ::std::pair<uint, uint>& position(uint idx) {return positions_[idx];}
  
  /// give the winsize of the saved patches (if patches from different
  /// sizes are extracted, here we expect the winsize of the scaled/stored patches
  uint& winsize() {return winsize_;}


  uint& dim() {return dim_;}

  /// has subsampling been used for the extraction? if each pixel was
  /// considered this is 0, otherwise each subsampling()+1 pixel was
  /// considered
  uint& subsampling()  {return subsampling_;}

  
  /// if padding was used -> 1, 0 otherwise 
  uint& padding()  {return padding_;}

  /// is the number of features which have been extracted
  uint& numberOfFeatures()  {return numberOfFeatures_;}

  /// the variance threshold which was used
  double& varthreshold()  {return varthreshold_;}

  /// the filename of the image where the features have been extracted
  /// from. This is used in GlobalLocalFeatureDistance
  ::std::string& filename()  {return filename_;}

  /// direct access to the internal data structure. should not be used!!
  const::std::vector< ::std::vector<double> >& getData() const { return data_; }
  
};

#endif
