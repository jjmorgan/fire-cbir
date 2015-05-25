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
#include <string>
#include <vector>
#include "getpot.hpp"
#include "gzstream.hpp"
#include "diag.hpp"
#include "imagefeature.hpp"
#include "histogramfeature.hpp"
#include "imagelib.hpp"
using namespace std;

/// extractanimfeatures
/// --- Extracts multiple features from a set of animated images.
/// --- Uses a given distance parameter to select key frames for extraction

void USAGE() {
	cout << "USAGE:" << endl
       << "extractanimfeatures [options] (--images filename1 [filename2 ... ]|--filelist <filelist>)" << endl
       << "   Options:" << endl
       << "    -h, --help      show this help" << endl
       << "    -s, --suffix    suffix of output files" << endl
       << "    --steps         how many steps per dimension, default: 8/color 256/gray." << endl
	   << "    -d, --distance  minimum distance (color histogram) between key frames" << endl;
       << endl;
}

int main(int argc, char** argv) {
	//TODO
}