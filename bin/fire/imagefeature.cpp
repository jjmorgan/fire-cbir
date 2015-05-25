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
#include "imagefeature.hpp"
#include <sstream>
#include <Magick++.h>
#include <cmath>

using namespace std;
using namespace Magick;

ImageFeature::ImageFeature() : xsize_(0), ysize_(0), zsize_(0),
                               data_(0,vector<double>(0)) {type_=FT_IMG;}

ImageFeature::ImageFeature(uint xsize, uint ysize, uint zsize) : xsize_(xsize), ysize_(ysize), zsize_(zsize),
                                                                 data_(zsize,vector<double>(xsize*ysize)) {type_=FT_IMG;}


ImageFeature::~ImageFeature() {
  for(uint i=0;i<data_.size();++i) {
    data_[i].clear();
  }
  data_.clear();
}
 

// methods to import/export from JF files
void ImageFeature::createFromJF(DoubleVector* data) {
  int width = (int) sqrt((double) data->size());
  xsize_ = width;
  ysize_ = width;
  zsize_ = 1;
  data_.resize(zsize_);
  for (uint c = 0; c < zsize_; ++c) {
    data_[c].resize(xsize_ * ysize_);
  }
  for (uint i = 0; i < xsize_ * ysize_ ; i++) {
    data_[0][i] = (*data)[i];
  }
}

DoubleVector* ImageFeature::toJF(int channel) {
  DoubleVector* dv = new DoubleVector(0);
  for (uint i = 0; i < xsize_ * ysize_; i++) {
    dv->push_back(data_[channel][i]);
  }
  return dv;
}

// method to create an image feature from an array of ints
void ImageFeature::createFromPixelset(int width, int height, int* pixels) {
  xsize_ = width;
  ysize_ = height;
  zsize_ = 1;
  data_.resize(zsize_);
  for (uint c = 0; c < zsize_; ++c) {
    data_[c].resize(xsize_ * ysize_);
  }
  for (uint i = 0; i < xsize_ * ysize_; i++) {
    data_[0][i] = (pixels[i] / 255.0);
  }
}

void ImageFeature::createFromPixelset(int width, int height, const vector<int>& pixels) {
  xsize_ = width;
  ysize_ = height;
  zsize_ = 1;
  data_.resize(zsize_);
  for (uint c = 0; c < zsize_; ++c) {
    data_[c].resize(xsize_ * ysize_);
  }
  for (uint i = 0; i < xsize_ * ysize_; i++) {
    data_[0][i] = (pixels[i] / 255.0);
  }
}



void ImageFeature::load(const ::std::string &filename) {
  this->load(filename,false);
}
 
//inherited from BaseFeature
void ImageFeature::load(const ::std::string& filename, bool forceGray){
  //temporary pixel data
  ColorGray grayPixel;
  ColorRGB rgbPixel;
  
  if(filename.find("gray") < filename.size()) {
    forceGray=true;
  }
  
  //temporary image data
  Image loaded;
  // try {
    loaded.read(filename);
    //  } catch( Exception &error_ ) {
    // ERR << "Exception loading image: " << error_.what() << endl;
    // return;
    // }
  
  xsize_=loaded.columns();
  ysize_=loaded.rows();
  if(loaded.colorSpace()==RGBColorspace && !forceGray) {
    zsize_=3;
    DBG(30) << "found image in RGB" << endl;
  } else if ( loaded.colorSpace()==GRAYColorspace || forceGray) {
    DBG(30) << "found image in gray" << endl;
    zsize_=1;
  } else {
    ERR << "Unknown color space" << loaded.colorSpace() << endl;
  }
  
  data_.resize(zsize_);
  for(uint c=0;c<zsize_;++c) data_[c].resize(xsize_*ysize_);
  
  switch(zsize_) {
  case 1: //gray image
    DBG(30) << "Loading gray image" << endl;
    for(uint y=0;y<ysize_;++y) {
      for(uint x=0;x<xsize_;++x) {
        rgbPixel=loaded.pixelColor(x,y);
        data_[0][y*xsize_+x]=(rgbPixel.red()+rgbPixel.green()+rgbPixel.blue())/3;
      }
    }
    break;
  case 3: //rgb image
    DBG(30) << "Loading rgb image" << endl;
    for(uint y=0;y<ysize_;++y) {
      for(uint x=0;x<xsize_;++x) {
        rgbPixel=loaded.pixelColor(x,y);
        data_[0][y*xsize_+x]=rgbPixel.red();
        data_[1][y*xsize_+x]=rgbPixel.green();
        data_[2][y*xsize_+x]=rgbPixel.blue();
      }
    }
    break;
  default:
    ERR << "Unknown image format, depth=" << zsize_ << endl;
    break;
  }
}

Image ImageFeature::makeMagickImage(const uint &idx1, const uint &idx2, const uint& idx3) const {
  Image result(Geometry(xsize_,ysize_),"red");
  result.quality(100);
  
  if(zsize_==1 || (idx1==idx2 && idx1==idx3)) {
    DBG(30) << "Making Magick image in GRAY" << endl;
    result.colorSpace(GRAYColorspace);
    for(uint x=0;x<xsize_;++x) {
      for(uint y=0;y<ysize_;++y) {
        result.pixelColor(x,y,ColorGray(data_[idx1][y*xsize_+x]));
      }
    }
  } else {
    result.colorSpace(RGBColorspace);
    DBG(30) << "Making Magick image in RGB" << endl;
    for(uint x=0;x<xsize_;++x) {
      for(uint y=0;y<ysize_;++y) {
        result.pixelColor(x,y,ColorRGB(data_[idx1][y*xsize_+x], data_[idx2][y*xsize_+x], data_[idx3][y*xsize_+x]));
      }
    }
  }
  result.syncPixels();
  return result;
}

void ImageFeature::display(const uint& idx1, const uint& idx2, const uint& idx3) const {
  Image toDisplay=makeMagickImage(idx1, idx2, idx3);
  toDisplay.display();
}

void ImageFeature::save(const ::std::string & filename, const uint& idx1, const uint& idx2, const uint& idx3) {
  Image toSave=makeMagickImage(idx1, idx2, idx3);
  //  try {
    toSave.write(filename);
    // } catch( Exception &error_ ) {
    //  ERR << "Exception loading image: " << error_.what() << endl;
    //  return;
    // }
}

const vector<double> ImageFeature::operator()(uint x, uint y) const{
  vector<double> result;
  for(uint c=0;c<zsize_;++c) {
    result.push_back(data_[c][y*xsize_+x]);
  }
  return result;
}

void ImageFeature::append(const ImageFeature& img) {
  if(img.xsize()==xsize_ && img.ysize() == ysize_) {
    uint oldsize=data_.size();
    data_.resize(data_.size()+img.zsize(),vector<double>(xsize()*ysize()));
    
    for(uint c=0;c<img.zsize();++c) {
      for(uint x=0;x<img.xsize();++x) {
        for(uint y=0;y<img.ysize();++y) {
          (*this)(x,y,oldsize+c)=img(x,y,c);
        }
      }
    }
    zsize_=data_.size();
  } else {
    ERR << "Images not compatible for appending" << endl;
  }
}

void ImageFeature::read(::std:: istream & is) {
  istringstream iss;
  string line, tmp;
  getline(is,line);
  if(line!="FIRE_imagefeature") {
    ERR << "Magicnumber not found. This is not a valid image feature" << endl;
    return;
  } 

  getline(is,line); iss.clear(); iss.str(line); iss >> tmp;
  if(tmp!="sizes") {
    ERR << "Expecting 'sizes', got '" << line << "'." << endl;
    return;
  } else {
    iss >> xsize_ >> ysize_ >> zsize_;
    data_.resize(zsize_);
    for(uint c=0;c<zsize_;++c) {
      data_[c].resize(xsize_*ysize_);
    }
  }

  getline(is,line); iss.clear(); iss.str(line); iss >> tmp;
  if(tmp!="data") {
    ERR << "Expecting 'data', got '" << line << "'." << endl;
    return;
  } else {
    for(uint x=0;x<xsize_;++x) {
      for(uint y=0;y<ysize_;++y) {
        for(uint z=0;z<zsize_;++z) {
          iss >> this->operator()(x,y,z);
        }
      }
    }
  }
}

const ImageFeature ImageFeature::layer(const uint i) const {
  ImageFeature result(xsize(),ysize(),1);
  for(uint x=0;x<xsize_;++x) {
    for(uint y=0;y<ysize_;++y) {
      result(x,y,0)=this->operator()(x,y,i);
    }
  }
  return result;
}

void ImageFeature::write(::std::ostream & os) {
  os << "FIRE_imagefeature" << endl
     <<  "sizes " << xsize_ << " " << ysize_ << " " << zsize_ << endl
     << "data" ;
  for(uint x=0;x<xsize_;++x) {
    for(uint y=0;y<ysize_;++y) {
      for(uint z=0;z<zsize_;++z) {
        os << " " << this->operator()(x,y,z);
      }
    }
  }
  os << endl;
}

HSVPixel ImageFeature::hsvPixel(int x, int y) {
  HSVPixel hsv;
  if (zsize_ != 3) {
    printf("error: can compute hsv pixel value only for rgb images!\n");
    hsv.h = 0;
    hsv.s = 0;
    hsv.v = 0;
    return hsv;
  } else {
    double min, max, delta;
    double red = (*this)(x, y, 0);
    double green = (*this)(x, y, 1);
    double blue = (*this)(x, y, 2);

    min=red;
    if (green<min) {
      min=green;
    }
    if(blue<min) {
      min=blue;
    }

    max=red;
    if (green>max) {
      max=green;
    }
    if(blue>max) {
      max=blue;
    }

    hsv.v = max;
    delta = max - min;

    if (max != 0.0) {
      hsv.s = delta / max;        // s
    } else {
      hsv.s = 0;
      hsv.h = 0;
      return hsv;
    }

    if (delta==0) {
      hsv.h=0; 
    } else  {
      if( red == max ) {
	if (delta==0) 
	  std::cerr << "FEHLER" << std::endl;
	hsv.h = ( green - blue ) / delta;        // between yellow & magenta
      } else if( green == max ) {
	if (delta==0) 
	  std::cerr << "FEHLER" << std::endl;
	hsv.h = 2 + ( blue - red ) / delta;    // between cyan & yellow
      } else  {
	if (delta==0) 
	  std::cerr << "FEHLER" << std::endl;
	hsv.h = 4 + ( red - green ) / delta;    // between magenta & cyan
       }
      hsv.h *= 60;                // degrees
    }
    if (hsv.h < 0)
        hsv.h += 360;
    return hsv;
  }



}
