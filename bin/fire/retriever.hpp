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
#ifndef __retriever_hpp__
#define __retriever_hpp__

#include <string>
#include <stack>
#include "diag.hpp"
#include "imagecontainer.hpp"
#include "database.hpp"
#include "imagecomparator.hpp"
#include "basescoring.hpp"
#include "linearscoring.hpp"
#include "maxentscoring.hpp"
#include "getscoring.hpp"
#include "distanceinteractor.hpp"

typedef ::std::pair<double,uint> ResultPair;



/** retriever-class: this class contains the logic of the image
    retriever. Here several classes are used which together form the
    retrieval engine.

    - database: here the features representing the images in which we
    can search for images are managed. The database can be seen as a
    vector of ImageContainers. Each ImageContainer is a vector of
    features representing the images. This is accompanied by some
    meta-data like file names.

    - ImageComparator: here the feature comparison methods are
    encapsulated. The ImageComparator is a container for image
    comparison methods and it is handed a query image and database
    images and it calls the approppriate distance functions for
    comparing images. 

    - Scorer: this is the class combining the results from the image
    comparator into one Retrieval result. That is, given two images,
    the image comparator return a vector of distances (as many as the
    images have features representing them), these distances are
    combined into one score. This can be seen as classification into
    relevant and irrelevant images and the returning score is the
    confidence for an image to be relevant. Thus, the images with
    highest scores are returned first. 

    additionally, the Retriever-class contains many helper functions
    to allow for access to data in higher layers.

*/
class Retriever {

private:

  /// this database object holds the database of images from which we retrieve images
  Database database_;

  /// this object knows how to compare images from the database. That
  /// is, the distance functions and the associated weightings are
  /// known here.
  ImageComparator imageComparator_;

  /// the number of results returned for a query 
  uint results_;
  
  /// the number of images taken from a query result which is used as
  /// automatic positive relevance feedback for query expansion
  uint extensions_;

  /// the object that calculates the score from a given distance vector
  BaseScoring *scorer_;
  
  /// DistanceInteractor is an object that can access all distances
  /// (basically the complete distance matrix for a query) and change
  /// them in taking into account certain interactions
  DistanceInteractor interactor_;
  
  /**
   * given the queries, find the appropriate ImageContainers. If a
   * name is given for which no image is in the database it is tried
   * to load the image.
   * @param queryNames the names of the queries, these have to be resolved
   * @param queries here the ImageContainers are put that belong to the queryNames
   * @param newCreated here pointers to those ImageContainers are
   *     stored that are newly created, that is loaded, in this
   *     function. This is necessary to allow the desctruction of these
   *     images as soon as they are not needed anymore.
   */
  void resolveNames(const ::std::vector< ::std::string > &queryNames, 
                    ::std::vector<ImageContainer*> &queries, 
                    ::std::stack<ImageContainer*> &newCreated);
public:

  /// default constructor
  Retriever() : database_(), imageComparator_(), results_(0), extensions_(0), interactor_() {
    scorer_=new LinearScoring();
  }
  
  ~Retriever() {
    delete scorer_;
    database_.clear();
  }

  
  /// given a set of positive and a set of negative example image
  /// names the retrieval process is started. This function is
  /// basically a wrapper to resolve names and
  /// retrieve(vector<ImageContainer>, vector<ImageContainer>)
  void retrieve(const ::std::vector< ::std::string >& posQueries,
                const ::std::vector< ::std::string >& neqQueries,
                ::std::vector<ResultPair>& results);
  
  /// given a set of positive and a set of negative example
  /// ImageContainers get the results of the retrieval. For this, the
  /// right ImageComparator is necessary.
  void retrieve(const ::std::vector<ImageContainer*>& posQueries, 
                const ::std::vector<ImageContainer*>& negQueries,
                ::std::vector<ResultPair>& results);
  
  /// start a retrieval using some meta information
  ::std::vector<ResultPair> metaretrieve(const ::std::string& query);

  /// start a retrieval using some text
  ::std::vector<ResultPair> textretrieve(const ::std::string& query);

  /// get a list of the available metafeature keys and an example value
  // for each key
  ::std::pair< ::std::set< ::std::string >,
               ::std::set< ::std::string > > getMetaFeatureInfo();

 /// get the distances from the given example ImageContainer q to all images in the database
  void Retriever::getScores(const ImageContainer* q,::std::vector<double> &scores);
  
  /// save the distances from the given example Image (specified by
  /// the imagename) to all database images to the specified file.
  void Retriever::saveDistances(::std::string imagename, ::std::string filename);
  
  /// set the idx-th distance in the ImageComparator used.
  ::std::string dist(const uint idx, BaseDistance* dist);

  /// return the idx-th distance in the ImageComparator used.
  BaseDistance* dist(const uint idx);
  
  /// set the idx-th weight in the ImageComparator used.
  ::std::string weight(const uint idx, const double w);

  /// return the idx-th weight in the ImageComparator used.
  double weight(const uint idx) const;

  /// return the scoring used
  BaseScoring *scorer() {return scorer_;}
  
  /// return a reference to the Interactor
  DistanceInteractor& interactor() {return interactor_;}

  /// set a new interactor
  void setInteractor(::std::string interactor) {
    interactor_=DistanceInteractor(interactor);
  }
  
  /// return the available scorings
  ::std::string availableScorings() const { return listScorings(); }

  /// set a new scoring algorithm
  void setScoring(const ::std::string &scoringname);
  
  /// how many images are in the database?
  uint numberOfFilelistEntries() const;

  /// return the nr-th filename from the filelist
  ::std::string filelist(const uint nr) const;

  /// load the filelist with the specified name
  ::std::string filelist(const ::std::string filelist);
  
  /// reutrn a string with all names from the filelist (starting from
  /// 0 to size(filelist)-1
  ::std::string filelistEntries() const;
  
  /// set the number of results to be returned in all queries from now on
  ::std::string results(const uint res);

  /// return the number of results delivered after every query
  uint results();

  /// set the number of images taken as positive examples from a query
  /// result for automatic relevance feedback a.k.a. query expansion for dummies
  ::std::string extensions(const uint ext);

  /// give a string providing information on the settings of this retriever
  ::std::string info() const;

  /// give a string containing the filenames of nr images taken randomly from the database
  ::std::string random(uint nr);

  /// return whether we have class information for the database loaded currently or not
  bool haveClasses() const;

  /// return the clas of the idx-th image from the database
  int clas(const uint &idx) const;

  /// return the class of the image with the given filename from the database
  int clas(const ::std::string &filename) const;

  /// return the number of suffices in the database
  uint numberOfSuffices() const {return database_.numberOfSuffices();}

  /// return the feature type of the i-th feature of the 0-th image in
  /// the database. This should be the feature type of the i-th
  /// feature of all images.
  FeatureType featureType(uint i) const {return database_.featureType(i);}

};

#endif
