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
#include <iostream>
#include "diag.hpp"
#include "getpot.hpp"

using namespace std;


void USAGE() {
  cout << "USAGE: demoprogram <options>" << endl
       << "   -h,--help    show this help" << endl
       << endl;
}

int main(int argc, char**argv) {
  GetPot cl(argc, argv);
  
  if(cl.search(2,"-h","--help")) {USAGE(); exit(0);}

  DBG(10) << "This is a framework for writing new programs in the fire framework" << endl;
  
}
