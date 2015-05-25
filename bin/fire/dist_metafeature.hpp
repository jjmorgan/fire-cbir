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
#ifndef __dist_metafeature_hpp__
#define __dist_metafeature_hpp__

#include "basedistance.hpp"
#include "metafeature.hpp"

using namespace std;

class MetaFeatureDistance : public BaseDistance {
public:

  virtual double distance(const BaseFeature* queryFeature, const BaseFeature* databaseFeature) {
  
    const MetaFeature* db=dynamic_cast<const MetaFeature*>(databaseFeature);
    const MetaFeature* query=dynamic_cast<const MetaFeature*>(queryFeature);

    if(db && query) {
      double dist = 0.0;
      map<string, string>::const_iterator mf_it, curr_key;

      for(mf_it = query->values().begin(); mf_it != query->values().end(); ++mf_it) {
        
	// The dist is increased if db has no such key or the value of
	// this key is different
	curr_key = db->values().find(mf_it->first);
	if(curr_key == db->values().end()) {
	  dist += 1.0;
	} else {
	  if(curr_key->second != mf_it->second) {
	    dist += 1.0;
	  }
	}

      }
      
      return dist;
  
    } else {
      ERR << "Features not comparable" << ::std::endl;
      return -1.0;
    }
  }

  virtual ::std::string name() {return "metafeature";}
  virtual void start(const BaseFeature *) {}
  virtual void stop(){}
};

#endif
