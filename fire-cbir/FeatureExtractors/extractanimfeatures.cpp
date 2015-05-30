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
#include <sstream>
#include <vector>
#include "getpot.hpp"
#include "gzstream.hpp"
#include "diag.hpp"
#include "imagefeature.hpp"
#include "histogramfeature.hpp"
#include "imagelib.hpp"

#ifdef HAVE_IMAGE_MAGICK
#include "Magick++.h"
#else
#warning "Functionality of imagefeature is reduced without ImageMagick: no loading, saving, displaying." << endl;
#endif

using namespace std;

using namespace Magick;

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
	     << "    -d, --distance  minimum distance (color histogram) between key frames" << endl
       << "    --color         extract color histograms only" << endl
       << endl;
}

void process_file(string filename, string suffix) {
#ifdef HAVE_IMAGE_MAGICK
  /// convert the image to the data structure used by image
  /// magick. This is needed for saving and displaying of images.
  
  vector<Image> frames_in;
  vector<Image> frames;
  try {
    readImages(&frames_in, filename);
    frames = frames_in;
	  coalesceImages(&frames, frames_in.begin(), frames_in.end());
  } catch( ... ) {
    ERR << "Exception loading image: " << filename << endl;
    return;
  }
  
  for (uint i = 0; i < frames.size(); i++) {
    ImageFeature im;
    im.load(filename, frames[i]);
    
    // for now extract color histograms for every frame to separate files
  
    HistogramFeature result(vector<uint>(3,8));
    result.min()=vector<double>(3,0.0);
    result.max()=vector<double>(3,1.0);
    result.initStepsize();

    vector<double> tofeed(3);

    for(uint x=0;x<im.xsize();++x) {
      for(uint y=0;y<im.ysize();++y) {
        tofeed[0]=im(x,y,0);
        tofeed[1]=im(x,y,1);
        tofeed[2]=im(x,y,2);
        result.feed(tofeed);
      }
    }
    
    ostringstream outfilename;
    if (i == 0)
      outfilename << filename << "." << suffix;
    else
      outfilename << filename << "." << i << "." << suffix;
    result.save(outfilename.str());
  }

  DBG(20) << "Extracted features for " << frames.size() << " frames from '" << filename << "'." << endl;
  
#else
#warning "Cannot extract animation features without ImageMagick"
#endif 
}

int main(int argc, char** argv) {
	GetPot cl(argc,argv);
  //command line parsing
  if(cl.search(2,"--help","-h")) USAGE();
  if(cl.size()<2) USAGE();

  string suffix;
  suffix=cl.follow("color.histo.gz","--suffix");
  uint steps=cl.follow(8,"--steps");

  //get list of files to be processed
  vector<string> infiles;

  if(cl.search("--images")) {
    string filename=cl.next(" ");;
    while(filename!=" ") {
      infiles.push_back(filename);
      filename=cl.next(" ");
    }
  } else if (cl.search("--filelist")) {
    string filename="test";
    igzstream ifs; ifs.open(cl.follow("list","--filelist"));
    if(!ifs.good() || !ifs) {
      ERR << "Cannot open filelist " <<cl.follow("list","--filelist")  << ". Aborting." << endl;
      exit(20);
    }
    while(!ifs.eof() && filename!="") {
      getline(ifs,filename);
      if(filename!="") {
        infiles.push_back(filename);
      }
    }
    ifs.close();
  } else {
    USAGE();
    exit(20);
  }
  
  // process files
  for (uint i = 0; i < infiles.size(); i++) {
    DBG(10) << "Processing '" << infiles[i] << "' (" << i+1<< "/" << infiles.size() << ")." << endl;
    process_file(infiles[i], suffix);
  }
  
  DBG(10) << "cmdline was: "; printCmdline(argc,argv);
}