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
#ifndef __database_hpp__
#define __database_hpp__

#include <vector>
#include <string>
#include "imagecontainer.hpp"
#include "diag.hpp"
#include "featureloader.hpp"

class Database {
private:

  /// this is the main part of this class. it does contain all ImageContainer
  ::std::vector<ImageContainer *> database_;

  /// this is for fast name based access to the data
  ::std::map< ::std::string, uint > name2IdxMap_;

  /// what suffices, that is what types of features does every image have?
  ::std::vector< ::std::string > suffixList_;
  

  /// an object that handles the loading of features. The
  /// file suffices are mapped to the right feature type, the feature
  /// type is constructed and the file is loaded.
  FeatureLoader fl;

  /// the features are in the same directory as the images or in
  /// subdirectories of the database directory with the names of the suffices
  bool featuredirectories_;

  /// we have class information?
  bool classes_;

  /// we have a textual description for the images
  bool descriptions_;

  /// large feature files are used. 
  bool largefeaturefiles_;

  /// the basepath of the database
  ::std::string path_;

  ///  a method that compares whether the two given features are
  ///  consistent. returns true if they are, false otherwise. But true
  ///  is only a "probably true"
  bool checkConsistency(const BaseFeature *ref, const BaseFeature *test) const;
  
public:


  /// constructor
  Database() : featuredirectories_(false), classes_(false), descriptions_(false), largefeaturefiles_(false), path_("") {}
  ~Database();


  /// load an image that is not in the database and the appropriate
  /// features. This method is implemented here as only the database
  /// knows which features have to be loaded. This won't use any large
  /// feature file but assumes that the features for this file are
  /// given in single files
  bool loadQuery(const ::std::string& filename, ImageContainer *result);

  /// return the idx-th ImageContainer object
  ImageContainer* operator[](uint idx) { return database_[idx]; }

  /// return the idx-th ImageContainer object
  const ImageContainer* operator[](uint idx) const { return database_[idx]; }

  /// search an ImageContainer given it's name. Take care, this is
  /// inefficiently implemented (linear search)
  ImageContainer* getByName(const ::std::string &filename) const;

  /// return whether featuredirectories are used or not
  bool featuredirectories() const {return featuredirectories_;}

  /// clear the database. afterwards there won't be any features left
  void clear();

  /// load a filelist (but not the features)
  /// TODO: document format of filelist
  uint loadFileList(::std::string filelist);
  
  /// load the features specified by a previously loaded filelist
  void loadFeatures();
  
  /// how many images are in this database
  uint size() const { return database_.size(); }

  /// what is the filename of the idx-th image
  ::std::string filename(const uint idx) const;

  /// what is the basepath of the database. 
  const ::std::string& path() const {return path_;}

  /// how many suffices (that is, how many features per image) do we have?
  uint numberOfSuffices() const; 
  
  /// give the idx-th suffix
  const ::std::string& suffix(const uint idx) const { return suffixList_[idx];}
  
  /// do we have classes?
  bool haveClasses() const;

  FeatureType featureType(const uint i) const {return fl.suffix2Type(suffixList_[i]);}

  /// get a list of the available metafeature keys with example values
  ::std::pair< ::std::set< ::std::string >,
    ::std::set< ::std::string > > getMetaFeatureInfo() const;
};

#endif
