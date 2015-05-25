#include <string>
#include "gzstream.hpp"
#include "basefeature.hpp"

using namespace std;

void BaseFeature::load(const ::std::string &filename) {
  DBG(20) << "Loading from filename '" << filename << "'." << endl;
  igzstream is; // if igzstream is constructed with the file to be
                // opened, the state is wrong! Thus we have
                // construction and opening in two different lines
  is.open(filename.c_str());
  if(!is.good()) {
    ERR << "Cannot open '" << filename << " for reading." << endl;
  } else {
    this->read(is);
  }
  DBG(20) << "Loaded from filename '" << filename << "'." << endl;
}

void BaseFeature::save(const ::std::string &filename) {
  DBG(20) << "Writing to '" << filename << "'." << endl;
  ogzstream os; // if ogzstream is constructed with the file to be
                // opened, the state is wrong! Thus we have
                // construction and opening in two different lines
  os.open(filename.c_str());
  if(!os.good()) {
    ERR << "Cannot open '" << filename << "' for writing." << endl;
  } else {
    this->write(os);
  }
  DBG(20) << "Saved to '" << filename << "'." << endl;
}
