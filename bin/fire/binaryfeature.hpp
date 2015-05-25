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
#ifndef __binaryfeature_hpp__
#define __vectorfeature_hpp__

#include <string>
#include <iostream>
#include "basefeature.hpp"

class BinaryFeature : public BaseFeature {
private:
  bool value_;
public:
 
  /// constructor, if no parameter (or 0) given -> defaults to false,
  /// else to true
  BinaryFeature(int val=0)  {
    type_=FT_BINARY;
    if(val!=0) value_=true;
    else value_=false;
  }
  
  /// constructor. Quite intuitive, isn't it?
  BinaryFeature(bool val) :value_(val){type_=FT_BINARY;}


  /// derived from base feature, read from stream
  void BinaryFeature::read(::std::istream &is) {
    ::std::string line;
    ::std::getline(is,line);
    if(line=="COLOR" || line=="false" || line=="0") {
      value_=false;
    } else if(line=="GRAY" || line=="true" || line=="1") {
      value_=true;
    } else {
      ERR << "Unknown token: " << line << ::std::endl;
    }
  }

  /// derived from base feature, write to stream
  void BinaryFeature::write(::std::ostream &os) {
    if(value_) {
      os << "true" << ::std::endl;
    } else {
      os << "false" << ::std::endl;
    }
  }

  /// return the value
  const bool value() const {return value_;}

  /// return the value and allow for changing
  bool& value() {return value_;}

};

#endif
