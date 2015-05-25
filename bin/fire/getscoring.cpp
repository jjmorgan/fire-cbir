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

#include "basescoring.hpp"
#include "maxentscoring.hpp"
#include "linearscoring.hpp"
#include "getscoring.hpp"
#include "maxentscoringsecondorder.hpp"
#include "maxentscoringfirstandsecondorder.hpp"
#include <string>

using namespace std;

BaseScoring * getScoring(const ::std::string& scoringname,const uint numberOfDistances) {
  BaseScoring *result=NULL;
  ::std::string configfile="";

  uint pos=scoringname.find("CONFIG=");
  if(pos<scoringname.size()) {
    for(uint i=pos+7; (scoringname[i]>='0' and scoringname[i]<='9') or (scoringname[i]>='a' and scoringname[i]<='z') or (scoringname[i] >= 'A' and scoringname[i]<='Z') or (scoringname[i]=='.') or (scoringname[i]=='/') or (scoringname[i]=='-') and i<scoringname.size();++i) {
      configfile+=scoringname[i];
    }
  }
  
  
  if(configfile=="") {
    if(scoringname=="linear") {
      result=new LinearScoring(numberOfDistances);
    }
  } else {
    if(scoringname.find("linear")<scoringname.size()) {
      result=new LinearScoring(configfile);
    } else if(scoringname.find("maxent1st2nd") < scoringname.size()) {
      result=new MaxEntFirstAndSecondOrderScoring(configfile);
    } else if(scoringname.find("maxent2nd") < scoringname.size()) {
      result=new MaxEntSecondOrderScoring(configfile);
    } else if(scoringname.find("maxent") < scoringname.size()) {
      result=new MaxEntScoring(configfile);
    } else {
      ERR << "Scoring '" << scoringname << "' unkown. Returning LinearScoring." << ::std::endl;
      result=new LinearScoring(numberOfDistances);
    }
    DBG(10) << "Scoring: " << result->type() << endl;
  }
  return result;
}

const ::std::string listScorings() {
  return "linear maxent maxent2nd maxent1st2nd";
}
