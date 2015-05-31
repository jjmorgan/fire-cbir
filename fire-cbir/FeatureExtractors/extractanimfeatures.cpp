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
#include "tamurafeature.hpp"
#include "histogramfeature.hpp"
#include "imagelib.hpp"
#include "basedistance.hpp"
#include "dist_jsd.hpp"

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
       << "    --all           extract all features" << endl
       << "    --color         extract color histograms" << endl
       << "    --tamura        extract tamura texture features" << endl
       << "    --colordelta    minimum jsd for identifying key frames using color histogram" << endl
       << "    --tamuradelta   minimum jsd for identifying key frames using tamura texture features" << endl
       << endl;
}

int extract_color_histograms(string filename, vector<Image> frames, double delta) {
  
  string suffix = "color.histo.gz";
  
  // compare histograms using jsd distance
  JSDDistance jsd = JSDDistance();
  HistogramFeature lastKeyFrameHist;
  
  int extract_count = 0;
  
  for (uint i = 0; i < frames.size(); i++) {
    ImageFeature im;
    im.load(filename, frames[i]);

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
    
    bool extract = false;
    if (i == 0) {
      // set first key frame
      lastKeyFrameHist = result;
      extract = true;
    }
    else {
      double score = jsd.distance(&result, &lastKeyFrameHist);
      if (score > delta) {
        // set new key frame
        lastKeyFrameHist = result;
        extract = true;
      }
    }
    
    if (extract) {
      ostringstream outfilename;
      if (i == 0)
        outfilename << filename << "." << suffix;
      else
        outfilename << filename << "." << extract_count + 1 << "." << suffix;
      result.save(outfilename.str());
      
      ostringstream outtemp;
      outtemp << filename << "." << i << ".jpg";
      frames[i].write(outtemp.str());
      
      extract_count++;
    }

  }
  
  return extract_count;
}

int extract_tamura_features(string filename, vector<Image> frames, double delta) {
  
  string suffix = "tamura.histo.gz";
  
  // compare histograms using jsd distance
  JSDDistance jsd = JSDDistance();
  HistogramFeature lastKeyFrameHist;
  
  int extract_count = 0;
  
  for (uint i = 0; i < frames.size(); i++) {
    ImageFeature im, tamuraImage;
    im.load(filename, frames[i]);
    tamuraImage = calculate(im);
    normalize(tamuraImage);
    
    HistogramFeature result = histogramize(tamuraImage);
    
    bool extract = false;
    if (i == 0) {
      // set first key frame
      lastKeyFrameHist = result;
      extract = true;
    }
    else {
      double score = jsd.distance(&result, &lastKeyFrameHist);
      DBG(20) << "Score: " << score << endl;
      if (score > delta) {
        // set new key frame
        lastKeyFrameHist = result;
        extract = true;
      }
    }
    
    if (extract) {
      ostringstream outfilename;
      if (i == 0)
        outfilename << filename << "." << suffix;
      else
        outfilename << filename << "." << extract_count + 1 << "." << suffix;
      result.save(outfilename.str());
      
      ostringstream outtemp;
      outtemp << filename << "." << i << ".jpg";
      frames[i].write(outtemp.str());
      
      extract_count++;
    }
  }
  
  return extract_count;
}

int main(int argc, char** argv) {
#ifdef HAVE_IMAGE_MAGICK
	GetPot cl(argc,argv);
  //command line parsing
  if(cl.search(2,"--help","-h")) USAGE();
  if(cl.size()<2) USAGE();

  //string suffix=cl.follow("color.histo.gz","--suffix");
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

  bool all = cl.search(2,"--all","-a");
  bool color = cl.search(2,"--color","-c");
  bool tamura = cl.search(2,"--tamura","-t");
  
  double color_delta=cl.follow(0.2,"--colordelta");
  double tamura_delta=cl.follow(0.7,"--tamuradelta");
  
  // process files
  for (uint i = 0; i < infiles.size(); i++) {
    DBG(10) << "Processing '" << infiles[i] << "' (" << i+1<< "/" << infiles.size() << ")." << endl;
    
    vector<Image> frames_in;
    vector<Image> frames;
    try {
      readImages(&frames_in, infiles[i]);
      frames = frames_in;
      coalesceImages(&frames, frames_in.begin(), frames_in.end());
    } catch( ... ) {
      ERR << "Exception loading image: " << infiles[i] << endl;
    }
    
    if (all || color) {
      int count = extract_color_histograms(infiles[i], frames, color_delta);
      DBG(20) << "Extracted color histograms for " << count << " / " << frames.size() << " frames from '" << infiles[i] << "'." << endl;
    }

    if (all || tamura) {
      int count = extract_tamura_features(infiles[i], frames, tamura_delta);
      DBG(20) << "Extracted Tamura texture features for " << count << " / " << frames.size() << " frames from '" << infiles[i] << "'." << endl;
    }

  }
  
  DBG(10) << "cmdline was: "; printCmdline(argc,argv);
#else
#warning "Cannot extract animation features without ImageMagick"
#endif 
}