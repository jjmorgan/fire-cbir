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
#include "localfeatures.hpp"
#include "imagefeature.hpp"
#include "getpot.hpp"
#include "imagelib.hpp"
#include "gzstream.hpp"
#include "pca.hpp"
#include "sift.hpp"
#include "salientpoints.hpp"

using namespace std;

void USAGE() {
  cout << "USAGE:" << endl
       << "extractlocalfeatures [options] (--color|--gray) (--images filename1 [filename2 ... ]|--filelist <filelist>)" << endl
       << "   Options:" << endl
       << "    -h, --help   show this help" << endl
       << "    --suffix <suffix>     to set the suffix of the output file, default: lf.gz" << endl
       << "    --winsize <winsize>   to set the size of the extracted local features, default: 5" << endl
       << "    --extractionSize <sizes> to specify the sizes of extracted patches to be scaled to winsize later" << endl
       << "    --padding             take lf ranging out of the image, default: no" << endl
       << "    --normalize           to mean/variance normalize extracted features, default: no" << endl
       << "    --tmpforlf <tmp>      to save localfeatures which are not necessary after pca transformation" << endl
       << "    --relativePositions   set positions in local features relative instead of absolute" << endl
       << "  OPTIONS FOR ADDING SOBEL FILTERS" << endl
       << "    --sobel1              add sobel values to the patches (discarding some pixel values)" << endl
       << "    --sobel2              create additional patches for sobel values, mixing them with pixel value patches" << endl
       << "    --sobel3              create additional patches for sobel values, saved in seperate files" << endl
       << "  OPTIONS WHERE FEATURES ARE EXTRACTED" << endl
       << "   if more than one of these methods is specified, the extraction points are added" << endl
       << "    --varpoints <no>      to take the first no features instead of all above a certain variance threshold" << endl
       << "    --varthreshold <t>    to set the variance threshold" << endl
       << "    --salientpoints <no>  to use salient point feature extraction and get <no> features" << endl
       << "    --dogpoints <no>      to use <no> Difference-of-Gaussian interest points" << endl
       << "    --uniformx <x>        to specify a regular grid of feature extraction points" << endl
       << "    --uniformy <y>        to specify a regular grid of feature extraction points" << endl
       << "  OPTIONS REGARDING PCA" << endl
       << "    --pca <dim>           to apply pca dimensionality reduction right after extraction" << endl
       << "    --loadPCA <filename>  don't calculate pca on the local features created, but load the given one" << endl
       << "    --savePCA <filename>  save the calculated PCA to the given file" << endl
       << "  OPTIONS REGARDING VISUALIZATION" << endl
       << "    --markPoints          with this option, the complete image will be saved with extraction points marked" << endl
       << endl;
  exit(20);
}



bool operator<(const pair<double,Point> &l, const pair<double,Point> &r) { return l.first<r.first;}

LocalFeatures extractPatches(const ImageFeature &img, // the image where we want to extract the patches
                             uint winsize,            // the winsize of the saved patches (2*winsize+1)²
                             bool padding,            // use padding?
                             uint salientpoints,      // how many salientpoints should be used
			     uint dogpoints,          // how many Difference-of-Gaussian interest points should be used
                             uint varpoints,          // how many points based on local variance should be used
                             uint uniformx,           // how many grid points in x-direction
                             uint uniformy,           // how many grid points in y-direction
                             double varthreshold,     // what is the variance threshold to be used for point finding
                             bool markPoints,         // save image with extraction positions marked?
                             string filename,         // the filename of the input image
                             vector<uint> extractionSizes, // different winsizes to be used for extraction
                                                      // if empty: use winsize only
                             bool normalize,          // normalize mean and variance
                             bool forceGray,          // are we working on gray images?
			     bool relativePositions   // shall positions in the local features be encoded as relative positions? 
                             ) {
  /*------------------------------------------------------------
    init
    ------------------------------------------------------------*/
  
  bool useScaling;
  uint paddingOffset=0; 
  ImageFeature padded;
  ImageFeature varimg;
  Point p;

  LocalFeatures lf;
  lf.winsize()=winsize;
  lf.padding()=padding;
  lf.filename()=filename;
  lf.varthreshold()=varthreshold;
  lf.zsize()=img.zsize();

  if(extractionSizes.size()==0) {extractionSizes.push_back(winsize); useScaling=false;}
  else {sort(extractionSizes.begin(), extractionSizes.end()); useScaling=true;}
  
  if(varthreshold>=0.0 || varpoints>0) {
    varimg=localvariance(img,winsize);
  }
  
  if(padding) {
    DBG(20) << "Padding" << endl;
    paddingOffset=extractionSizes[extractionSizes.size()-1]+2;
    padded=zeropad(img,paddingOffset,paddingOffset);
  } else {
    DBG(20) << "Not padding" << endl;
    padded=img;
  }

  // stores a list of elements, where each element is a pair
  // containing a point to be extracted and a list of patch sizes for extraction
  vector< pair<Point, vector<uint> > > extractionPoints;

  /*------------------------------------------------------------
    Get extraction points
    ------------------------------------------------------------*/

  if(salientpoints>0) {
    DBG(20) << "Extracting SalientPoints" << endl;

    SalientPoints sp;
    if(forceGray) {
      sp=SalientPoints(img.layer(0));
    } else {
      if(img.zsize()==3) {
        sp=SalientPoints(img);
      } else if(img.zsize()==9) {
        ImageFeature tmpimg(img.xsize(),img.ysize(),0);
        tmpimg.append(img.layer(0));
        tmpimg.append(img.layer(1));
        tmpimg.append(img.layer(2));
        sp=SalientPoints(tmpimg);
      } else {
        ERR << "Was expecting image with 3 layers (color) or 3*3 layers (color+sobel), but got " << img.zsize() << " layers." << endl;
        exit(10);
      }
    }
    vector<Point> sPoints = sp.getSalientPoints(salientpoints);
    for (vector<Point>::iterator spIterator = sPoints.begin(); spIterator != sPoints.end(); spIterator++) {
      extractionPoints.push_back(pair<Point, vector<uint> >(*spIterator, extractionSizes));
    }
    DBG(15) << "After SalientPoints: Having " << extractionPoints.size() << " feature extraction points." << endl;
  }
  
  if(varthreshold>=0.0) {
    DBG(20) << "Applying VarianceThreshold" << endl;
    for(uint x=0;x<varimg.xsize();++x) {
      for(uint y=0;y<varimg.ysize();++y) {
        if(varimg(x,y,0) > varthreshold) {
          p=Point(x,y);
          extractionPoints.push_back(pair<Point, vector<uint> >(p, extractionSizes));
        }
      }
    }
    DBG(15) << "After VarianceThreshold: Having " << extractionPoints.size() << " feature extraction points." << endl;
  }

  if(varpoints>0) {
    DBG(20) << "Extracting VariancePoints" << endl;
    vector< pair<double,Point> > variances;
    for(uint x=0;x<varimg.xsize();++x) {
      for(uint y=0;y<varimg.ysize();++y) {
        variances.push_back(pair<double,Point>(varimg(x,y,0),Point(x,y)));
      }
    }
    sort(variances.begin(), variances.end());
    for(uint i=0;i<varpoints;++i) {
      extractionPoints.push_back(pair<Point, vector<uint> >(variances[i].second, extractionSizes));
    }
    DBG(15) << "After VariancePoints: Having " << extractionPoints.size() << " feature extraction points." << endl;
  }
  
  if(uniformx >0 and uniformy>0) {
    DBG(20) << "Applying UniformGrid" << endl;
    uint stepx=max(uint(1),img.xsize()/(uniformx+1));
    uint stepy=max(uint(1),img.ysize()/(uniformy+1));
    if(uniformx>0 || uniformy>0) {
      for(uint x=1;x<=uniformx;++x) {
        for(uint y=1;y<=uniformy;++y) {
          p=Point(x*stepx,y*stepy);
          extractionPoints.push_back(pair<Point, vector<uint> >(p, extractionSizes));
        }
      }
    }
    DBG(15) << "After UniformGrid: Having " << extractionPoints.size() << " feature extraction points." << endl;
  }

  if (dogpoints > 0) {
    DBG(20) << "Applying Difference-of-Gaussian" << endl;
    SIFT sift(img);
    vector<InterestPoint> interestPoints = sift.getInterestPoints(dogpoints);
    for (vector<InterestPoint>::const_iterator ipIterator = interestPoints.begin(); ipIterator != interestPoints.end(); ipIterator++) {
      Point p; p.x = ipIterator->x; p.y = ipIterator->y;
      if (ipIterator->scale >= 5) {
	extractionPoints.push_back(pair<Point, vector<uint> >(p, vector<uint>(1, ipIterator->scale / 2)));
      }
    }
    DBG(15) << "After Difference-of-Gaussian: Having " << extractionPoints.size() << " feature extraction points." << endl;
  }

  /*------------------------------------------------------------
    Extraction
    ------------------------------------------------------------*/
  DBG(15) << "Starting to extract from " << extractionPoints.size() << " feature extraction points." << endl;

  uint savesize=winsize*2+1;

  ImageFeature patch;
  for(uint i=0;i<extractionPoints.size();++i) {
    pair<Point, vector<uint> > extractionElement = extractionPoints[i];
    p=extractionElement.first;
    DBG(50) << "Extracting image patch " << i << " at (" << p.x << ", " << p.y << ") in size";
    for(uint j=0;j<extractionElement.second.size();++j) {
      int w=extractionElement.second[j];
      BLINK(50) << " " << w;
      if(padding) {
        patch=getPatch(padded,p.x+paddingOffset,p.y+paddingOffset,w);

        if(normalize) { meanAndVarianceNormalization(patch,0.5,0.5); cutoff(patch); }
        //if(useScaling and w!=int(winsize)) { patch=scale(patch,savesize,savesize); }
        if (w != int(winsize)) { patch=scale(patch,savesize,savesize); }
        
        DBGI(100,patch.display());
	if (relativePositions) {
	  lf.addLocalFeature(toVector(patch),pair<uint,uint>(p.x,p.y), pair<double, double>(double(p.x) / double(img.xsize()), 
											    double(p.y) / double(img.ysize())), w);
	} else {
	  lf.addLocalFeature(toVector(patch),pair<uint,uint>(p.x,p.y),w);
	}
      } else {
        if((p.x-w>=0) and (p.y-w >=0) and (p.x+w<int(padded.xsize())) and (p.y+w<int(padded.ysize()))) {
          patch=getPatch(padded,p.x,p.y,w);

          if(normalize) { meanAndVarianceNormalization(patch,0.5,0.5); cutoff(patch);}
          //if(useScaling and w!=int(winsize)) { patch=scale(patch,savesize,savesize); }
          if(w != int(winsize)) { patch=scale(patch,savesize,savesize); }
          DBGI(100,patch.display());
	  if (relativePositions) {
	    lf.addLocalFeature(toVector(patch),pair<uint,uint>(p.x,p.y), pair<double, double>(double(p.x) / double(img.xsize()), 
											      double(p.y) / double(img.ysize())), w);
	  } else {
	    lf.addLocalFeature(toVector(patch),pair<uint,uint>(p.x,p.y),w);
	  }
        }
      }
    }
    BLINK(50) << endl;
  }

  /*------------------------------------------------------------
    Marking of extraction points
    ------------------------------------------------------------*/
  if(markPoints) {
    vector<double> color(3,1.0);
    for(uint i=0;i<extractionPoints.size();++i) {
	pair<Point, vector<uint> > extractionElement = extractionPoints[i];
        p=extractionElement.first;
        cross(padded,p.x+paddingOffset,p.y+paddingOffset,color,3);
        for(uint j=0;j<extractionElement.second.size();++j) {
          uint w=extractionElement.second[j];
          box(padded,p.x+paddingOffset, p.y+paddingOffset,color,w);
        }
    }
    DBG(10) << "Saving positionmarkimage to " << filename+".featureextractionpoints.png" << endl;
    padded.save(filename+".featureextractionpoints.png");
  }

  /*------------------------------------------------------------
    End
    ------------------------------------------------------------*/

  lf.numberOfFeatures()=lf.size();
  if (lf.size() > 0) {
    lf.dim()=lf[0].size();
  }
  DBG(10) << lf.size() << " patches extracted." << endl;
  return lf;
}
                             
int main(int argc, char** argv) {
  GetPot cl(argc,argv);
  if(cl.search(2,"--help","-h")) USAGE();
  if(cl.size()<2) USAGE();


  string TMPdir;

  int sobelMode = 0;
  if (cl.search("--sobel1")) {
    sobelMode = 1;
  } else if (cl.search("--sobel2")) {
    sobelMode = 2;
  } else if (cl.search("--sobel3")) {
    sobelMode = 3;
  }


  bool tmpforlf=cl.search("--tmpforlf");
  if(tmpforlf) TMPdir=cl.follow("/tmp/","--tmpforlf");


  string suffix=cl.follow("lf.gz","--suffix");

  uint winsize=cl.follow(5,"--winsize");
  bool padding=cl.search("--padding");
  bool normalize=cl.search("--normalize");
  bool relativePositions = cl.search("--relativePositions");

  vector<uint> extractionSizes;
  if(cl.search("--extractionSize")) {
    int exS=cl.next(-1);
    while(exS!=-1) {
      extractionSizes.push_back(exS);
      exS=cl.next(-1);
    }
  }
  
  bool trans_pca=cl.search("--loadPCA");  
  bool calc_pca=cl.search("--pca")&&!trans_pca;
  bool markPoints=cl.search("--markPoints");
  
  uint zsize; bool forceGray=false;
  if(cl.search("--color")) { zsize=3; forceGray=false;}
  else if(cl.search("--gray")) { zsize=1; forceGray=true;}
  else { USAGE(); exit(20);}

  PCA pca((2*winsize+1)*(2*winsize+1)*zsize);
  PCA sobelvPCA((2*winsize+1)*(2*winsize+1)*zsize);
  PCA sobelhPCA((2*winsize+1)*(2*winsize+1)*zsize);
  
  // get the information where the features are extracted
  uint varpoints=cl.follow(0,"--varpoints");
  double varthreshold=cl.follow(-1.0,"--varthreshold");
  uint salientpoints=cl.follow(0,"--salientpoints");
  uint dogpoints=cl.follow(0, "--dogpoints");
  uint uniformx=cl.follow(0,"--uniformx");
  uint uniformy=cl.follow(0,"--uniformy");
  
  vector<string> images;
  if(cl.search("--images")) {
    string filename=cl.next(" ");
    while(filename!=" ") {
      images.push_back(filename);
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
        images.push_back(filename);
      }
    }
    ifs.close();
    
  } else {
    USAGE();
    exit(20);
  }
  
  for(uint i=0;i<images.size();++i) {
    string filename=images[i];
    DBG(10) << "Processing '"<< filename << "'.(" << i << "/" << images.size()<< ")" <<endl;
    ImageFeature img; img.load(filename,forceGray);
    
    
    LocalFeatures lf;
    LocalFeatures lfSobelV;
    LocalFeatures lfSobelH;
    if (sobelMode == 0) {
      // no sobel filters get applied
      lf = extractPatches(img, winsize,padding,salientpoints,dogpoints,varpoints,uniformx,uniformy,varthreshold,markPoints,filename,extractionSizes,normalize,forceGray,relativePositions);
    } else if (sobelMode == 1) {
      ImageFeature imgSobelH = img;
      sobelh(imgSobelH);
      ::normalize(imgSobelH);
      ImageFeature imgSobelV = img;
      sobelh(imgSobelV);
      ::normalize(imgSobelV);
      img.append(imgSobelH);
      img.append(imgSobelV);
      lf = extractPatches(img, winsize,padding,salientpoints,dogpoints,varpoints,uniformx,uniformy,varthreshold,markPoints,filename,extractionSizes,normalize,forceGray,relativePositions);
    } else {
      lf = extractPatches(img, winsize,padding,salientpoints,dogpoints,varpoints,uniformx,uniformy,varthreshold,markPoints,filename,extractionSizes,normalize,forceGray,relativePositions);
      ImageFeature imgSobelH = img;
      sobelh(imgSobelH);
      ::normalize(imgSobelH);
      lfSobelH = extractPatches(imgSobelH, winsize,padding,salientpoints,dogpoints,varpoints,uniformx,uniformy,varthreshold,markPoints,filename+"-sobelh",extractionSizes,normalize,forceGray,relativePositions);
      ImageFeature imgSobelV = img;
      sobelv(imgSobelV);
      ::normalize(imgSobelV);
      lfSobelV = extractPatches(imgSobelV, winsize,padding,salientpoints,dogpoints,varpoints,uniformx,uniformy,varthreshold,markPoints,filename+"-sobelv",extractionSizes,normalize,forceGray,relativePositions);
    }
    if (sobelMode == 2) {
      lf.addLocalFeatures(lfSobelH);
      lf.addLocalFeatures(lfSobelV);
    }
   
    if(tmpforlf) {
      string fn=filename; 
      for(uint k=0;k<fn.size();++k) if (fn[k]=='/') fn[k]='_';
      lf.save(TMPdir+"/"+fn+"."+suffix);
      if (sobelMode == 3) {
	lfSobelV.save(TMPdir + "/" + fn + "-sobelv" + "." + suffix);
	lfSobelH.save(TMPdir + "/" + fn + "-sobelh" + "." + suffix);
      }
    } else {
      lf.save(filename+"."+suffix);
      if (sobelMode == 3) {
	lfSobelV.save(filename + "-sobelv" + "." + suffix);
	lfSobelH.save(filename + "-sobelh" + "." + suffix);
      }
    }
    DBG(10) << "Saved " << lf.numberOfVectors() << " local features to '" << filename << "." << suffix <<"'." << endl;
    if (sobelMode == 3) {
      DBG(10) << "Saved " << lfSobelV.numberOfVectors() << " local features to '" << filename << "-sobelv" <<  "." << suffix <<"'." << endl;
      DBG(10) << "Saved " << lfSobelH.numberOfVectors() << " local features to '" << filename << "-sobelh" << "." << suffix <<"'." << endl;
    }
    
    if(calc_pca) {
      if (sobelMode != 1) {
	for(uint i=0;i<lf.numberOfVectors();++i) {
	  pca.putData(lf[i]);
	}
	if (sobelMode == 3) {
	  for(uint i = 0; i < lfSobelV.numberOfVectors(); ++i) {
	    sobelvPCA.putData(lfSobelV[i]);
	  }
	  for(uint i = 0; i < lfSobelH.numberOfVectors(); ++i) {
	    sobelhPCA.putData(lfSobelH[i]);
	  }
	}
      } else {
	for(uint i = 0; i < lf.numberOfVectors(); ++i) {
	  int partSize = lf.dim() / 3;
	  vector<double> lfpart(partSize);
	  for (int j = 0; j < partSize; j++) {
	    lfpart[j] = lf[i][j];
	  }
	  pca.putData(lfpart);
	  for (int j = 0; j < partSize; j++) {
	    lfpart[j] = lf[i][j + partSize];
	  }
	  sobelhPCA.putData(lfpart);
	  for (int j = 0; j < partSize; j++) {
	    lfpart[j] = lf[i][j + 2 * partSize];
	  }
	  sobelvPCA.putData(lfpart);
	}
      }
    }
  }

  if(calc_pca||trans_pca) {
    if(calc_pca) {
      DBG(10) << "PCA received " << pca.counter() << " input vectors." << endl;
      pca.dataEnd();
      DBG(10) << "Starting to calculate PCA" << endl;
      pca.calcPCA();
      if(cl.search("--savePCA")) {
        pca.save(cl.follow("localfeatures.pca.gz","--savePCA"));
      }
      if ((sobelMode == 1) || (sobelMode == 3)) {
	DBG(10) << "vertical sobel PCA received " << sobelvPCA.counter() << " input vectors." << endl;
	sobelvPCA.dataEnd();
	DBG(10) << "Starting to calculate vertical sobel PCA" << endl;
	sobelvPCA.calcPCA();
	if(cl.search("--savePCA")) {
	  string tmpfilename = "sobelv-"; tmpfilename.append(cl.follow("localfeatures.pca.gz","--savePCA"));
	  pca.save(tmpfilename);
	}
	DBG(10) << "horizontal sobel PCA received " << sobelhPCA.counter() << " input vectors." << endl;
	sobelhPCA.dataEnd();
	DBG(10) << "Starting to calculate horizontal sobel PCA" << endl;
	sobelhPCA.calcPCA();
	if(cl.search("--savePCA")) {
	  string tmpfilename = "sobelh-"; tmpfilename.append(cl.follow("localfeatures.pca.gz","--savePCA"));
	  pca.save(tmpfilename);
	}
      }
    } else {
      pca.load(cl.follow("localfeatures.pca.gz","--loadPCA"));
      if ((sobelMode == 1) || (sobelMode == 3)) {
	string tmpfilename = "sobelv-"; tmpfilename.append(cl.follow("localfeatures.pca.gz","--loadPCA"));
	sobelvPCA.load(tmpfilename);
	tmpfilename = "sobelh-"; tmpfilename.append(cl.follow("localfeatures.pca.gz","--loadPCA"));
	sobelhPCA.load(tmpfilename);
      }
    }
    
    
    for(uint i=0;i<images.size();++i) {

      if ((sobelMode == 0) || (sobelMode == 2)) {
	uint dim=cl.follow(40,"--pca");
	string filename=images[i];
	DBG(10) << "PCA transforming '" << filename  << "." << suffix <<"'." << endl;
	LocalFeatures lf1; 
	if(tmpforlf) {
	  string fn=filename; for(uint k=0;k<fn.size();++k) if (fn[k]=='/') fn[k]='_';
	  lf1.load(TMPdir+"/"+fn+"."+suffix);
	} else {
	  lf1.load(filename+"."+suffix);
	}
	for(uint i=0;i<lf1.numberOfVectors();++i) {
	  DBGI(30,{DBG(30) << "untrans:";for(uint j=0;j<lf1[i].size();++j) BLINK(30) << lf1[i][j] << " ";BLINK(30) << endl;});
	  vector<double> transformed=vector<double>(pca.transform(lf1[i],dim));
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed.size();++j) BLINK(30) << transformed[j] << " "; BLINK(30) << endl;});
	  lf1[i].resize(dim);
	  for(uint j=0;j<dim;++j) { lf1[i][j]=transformed[j];}
	}
	lf1.dim()=dim;
	lf1.save(filename+".pca."+suffix);
	filename=cl.next(" ");

      } else if (sobelMode == 1) {
	uint dimTotal = cl.follow(40,"--pca");
	uint dimPixel = dimTotal / 2;
	uint dimSobelV = (dimTotal - dimPixel) / 2;
	uint dimSobelH = dimTotal - dimPixel - dimSobelV;
	string filename=images[i];
	DBG(10) << "PCA transforming '" << filename  << "." << suffix <<"'." << endl;
	LocalFeatures lfPixel; 
	if(tmpforlf) {
	  string fn=filename; for(uint k=0;k<fn.size();++k) if (fn[k]=='/') fn[k]='_';
	  lfPixel.load(TMPdir + "/" + fn + "." + suffix);
	} else {
	  lfPixel.load(filename + "." + suffix);
	}
	uint partSize = lfPixel.dim() / 3;
	for(uint i = 0; i < lfPixel.numberOfVectors(); ++i) {
	  vector<double> lfpart1(partSize), lfpart2(partSize), lfpart3(partSize);
	  for (int j = 0; j < (int) partSize; j++) { 
	    lfpart1[j] = lfPixel[i][j]; 
	    lfpart2[j] = lfPixel[i][j + partSize];
	    lfpart3[j] = lfPixel[i][j + 2 * partSize];
	  }
	  DBGI(30,{DBG(30) << "untrans:";for(uint j = 0; j < partSize; ++j) BLINK(30) << lfpart1[j] << " ";BLINK(30) << endl;});
	  DBGI(30,{DBG(30) << "untrans:";for(uint j = 0; j < partSize; ++j) BLINK(30) << lfpart2[j] << " ";BLINK(30) << endl;});
	  DBGI(30,{DBG(30) << "untrans:";for(uint j = 0; j < partSize; ++j) BLINK(30) << lfpart3[j] << " ";BLINK(30) << endl;});
	  vector<double> transformed1 = vector<double>(pca.transform(lfpart1, dimPixel));
	  vector<double> transformed2 = vector<double>(pca.transform(lfpart2, dimPixel));
	  vector<double> transformed3 = vector<double>(pca.transform(lfpart3, dimPixel));
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed1.size();++j) BLINK(30) << transformed1[j] << " "; BLINK(30) << endl;});
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed2.size();++j) BLINK(30) << transformed2[j] << " "; BLINK(30) << endl;});
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed3.size();++j) BLINK(30) << transformed3[j] << " "; BLINK(30) << endl;});
	  lfPixel[i].resize(dimTotal);
	  for(uint j = 0;j<dimPixel;++j) { lfPixel[i][j] = transformed1[j];}
	  for(uint j = dimPixel; j < dimPixel + dimSobelH; ++j) { lfPixel[i][j] = transformed2[j - dimPixel];}
	  for(uint j = dimPixel + dimSobelH; j < dimPixel + dimSobelH + dimSobelV; ++j) { lfPixel[i][j] = transformed3[j - dimPixel - dimSobelH];}
	}
	lfPixel.dim() = dimTotal;
	lfPixel.save(filename + ".pca." + suffix);
	filename = cl.next(" ");
	
      } else if (sobelMode == 3) {
	uint dim=cl.follow(40,"--pca");
	string filename=images[i];
	DBG(10) << "PCA transforming '" << filename  << "." << suffix <<"'." << endl;
	LocalFeatures lf1; 
	if(tmpforlf) {
	  string fn=filename; for(uint k=0;k<fn.size();++k) if (fn[k]=='/') fn[k]='_';
	  lf1.load(TMPdir+"/"+fn+"."+suffix);
	} else {
	  lf1.load(filename+"."+suffix);
	}
	for(uint i=0;i<lf1.numberOfVectors();++i) {
	  DBGI(30,{DBG(30) << "untrans:";for(uint j=0;j<lf1[i].size();++j) BLINK(30) << lf1[i][j] << " ";BLINK(30) << endl;});
	  vector<double> transformed=vector<double>(pca.transform(lf1[i],dim));
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed.size();++j) BLINK(30) << transformed[j] << " "; BLINK(30) << endl;});
	  lf1[i].resize(dim);
	  for(uint j=0;j<dim;++j) { lf1[i][j]=transformed[j];}
	}
	lf1.dim()=dim;
	lf1.save(filename+".pca."+suffix);
	DBG(10) << "PCA transforming '" << filename << "-sobelv" << "." << suffix <<"'." << endl;
	LocalFeatures lf2; 
	if(tmpforlf) {
	  string fn=filename; for(uint k=0;k<fn.size();++k) if (fn[k]=='/') fn[k]='_';
	  lf2.load(TMPdir + "/" + fn + "-sobelv" + "." + suffix);
	} else {
	  lf2.load(filename + "-sobelv" + "." + suffix);
	}
	for(uint i=0;i<lf2.numberOfVectors();++i) {
	  DBGI(30,{DBG(30) << "untrans:";for(uint j=0;j<lf2[i].size();++j) BLINK(30) << lf2[i][j] << " ";BLINK(30) << endl;});
	  vector<double> transformed=vector<double>(pca.transform(lf2[i],dim));
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed.size();++j) BLINK(30) << transformed[j] << " "; BLINK(30) << endl;});
	  lf2[i].resize(dim);
	  for(uint j=0;j<dim;++j) { lf2[i][j]=transformed[j];}
	}
	lf2.dim()=dim;
	lf2.save(filename + "-sobelv" + ".pca." + suffix);
	DBG(10) << "PCA transforming '" << filename << "-sobelh" << "." << suffix <<"'." << endl;
	LocalFeatures lf3; 
	if(tmpforlf) {
	  string fn=filename; for(uint k=0;k<fn.size();++k) if (fn[k]=='/') fn[k]='_';
	  lf3.load(TMPdir + "/" + fn + "-sobelh" + "." + suffix);
	} else {
	  lf3.load(filename + "-sobelh" + "." + suffix);
	}
	for(uint i=0;i<lf3.numberOfVectors();++i) {
	  DBGI(30,{DBG(30) << "untrans:";for(uint j=0;j<lf3[i].size();++j) BLINK(30) << lf3[i][j] << " ";BLINK(30) << endl;});
	  vector<double> transformed=vector<double>(pca.transform(lf3[i],dim));
	  DBGI(30,{DBG(30) << "transformed:"; for(uint j=0;j<transformed.size();++j) BLINK(30) << transformed[j] << " "; BLINK(30) << endl;});
	  lf3[i].resize(dim);
	  for(uint j=0;j<dim;++j) { lf3[i][j]=transformed[j];}
	}
	lf3.dim()=dim;
	lf3.save(filename + "-sobelh" + ".pca." + suffix);
	filename=cl.next(" ");
      }

    }
  }

  DBG(10) << "cmdline was: "; printCmdline(argc,argv);
}
