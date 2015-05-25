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

#ifndef __imagecomparator_hpp__
#define __imagecomparator_hpp__

#include "basedistance.hpp"
#include "imagecontainer.hpp"


/** Class for comparing ImageContainer.  in this class the complete
 * knowledge about how images are compared is collected for the
 * retrieval process.  That is: The distances used are stored here.
 */
class ImageComparator {
private:
  /** the distances used */
  ::std::vector<BaseDistance*> distances_;

public:

  ///default constructor
  ImageComparator(); 

  ///constructor to have space for size many distances
  ImageComparator(uint size);
  
  ///time to die
  ~ImageComparator();

  /// set idx-th distance
  void distance(const uint& idx, BaseDistance* d);

  /// return the idx-th distance
  BaseDistance *distance(const uint& idx) const;
  
  ///  how many distances  do we have?
  uint size() const;

  /// initialize all distance functions. for the specified query
  /// image. This function is necessary as some distance functions
  /// need an initialization to be read to return the actual
  /// distances. Examples for this are globallocalfeaturedistance and
  /// mpeg7distance. For these, all distances are calculated in this
  /// step and in the normal comparison step they are only returned.
  /// this functions calls the start-method for all distance functions
  /// defined.
  void start(const ImageContainer *query);
  
  /// the counterpiece to start. Here information that was stored in
  /// some distance functions is discarded. This function calls the
  /// stop-method for all distance functions defined.
  void stop();
  
  /// return the distance vector for the two images given.
  ::std::vector<double> compare(const ImageContainer* queryImage, 
                                const ImageContainer* databaseImage);
};
#endif
