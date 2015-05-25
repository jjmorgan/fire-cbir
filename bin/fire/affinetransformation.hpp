#ifndef __AFFINETRANSFORMATION_HPP__
#define __AFFINETRANSFORMATION_HPP__

#include <iostream>

/**
 *  rotate by alpha: [ cos(alpha)   sin(alpha)  ]
 *                   [ -sin(alpha)  cos(alpha)  ]
 *
 */





class AffineTransformation {
public:
  /// the matrix representing the Linear-Part of this affine transformation
  double A[2][2];
  /// the vector representing the translational part of this affine transformation
  double b[2];
  
  void Display() {
    ::std::cout 
      << "A=/ " << this->A[0][0] << " " << this->A[0][1] << " \\  b=/" << this->b[0] << " \\"<< ::std::endl
      << "  \\ "<< this->A[1][0] << " " << this->A[1][1] << " /     \\"<< this->b[1] << "/"<< ::std::endl;
  }
  
  AffineTransformation(double a1=0.0,double a2=0.0, double a3=0.0, double a4=0.0, double b0=0.0, double b1=0.0){
    A[0][0]=a1;
    A[0][1]=a2;
    A[1][0]=a3;
    A[1][1]=a4;
    b[0]=b0;
    b[1]=b1;
  }
};
#endif
