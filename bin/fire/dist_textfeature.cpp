#include "dist_textfeature.hpp"
#include "textfeature.hpp"
#include "net.hpp"

using namespace std;

double TextFeatureDistance::distance(const BaseFeature* queryFeature, const BaseFeature* databaseFeature) {
  
  const TextFeature* db=dynamic_cast<const TextFeature*>(databaseFeature);
  const TextFeature* query=dynamic_cast<const TextFeature*>(queryFeature);

  // There are two situations in which this can be called:
  // A normal image-based retrieval and a text-based retrieval
  // In the latter case, query_wmir is called before this 
  // function from textretrieve() to fill the rsv_table. Then,
  // this function is called with an empty textfeature as query.
  // In the former case, however, the queryFeature contains a filename.
  // If so, we do a file-based query, unless, of course, we already did
  // a query with this filename.
  if(   (query->value().compare("") != 0)
        &&(query->value().compare(queryfile_) != 0) ) {
    DBG(10) << "Doing text-based retrieval from file: "
            << query->value() << endl;
    query_wmir(":qfile "+query->value());
    queryfile_ = query->value();
  }

  if(rsv_table_.size() > 0) {

    if(db && query) {
      
      if( rsv_table_.find(db->value()) != rsv_table_.end() ) {
        double dist = max_rsv_ - rsv_table_[db->value()];
        /*
          DBG(10) << "found one with filename "<<db->value()
          << " and RSV " << rsv_table_[db->value()]
          << " => dist: " << dist
          << endl;
        */
        return dist;
      } else {
        return max_rsv_; // Just to have a value that is much higher than the value for the found documents
      }
	
    } else {
      ERR << "Features not comparable" << ::std::endl;
      if(!db) {
        ERR << "no db" << ::std::endl;
      } else {
        ERR << "no query" << ::std::endl;
      }
      return -1.0;
    }

  } else {
    return 10000.0;
  }
}

// Fill the rsv table with results from wmir with the query-string
void TextFeatureDistance::query_wmir(const string& _query) {
  rsv_table_.clear();

  // Get RSVs from WMIR
  DBG(15) << "trying to contact WMIR ...";
  Socket sock(server_, port_);
  if( !sock.connected() ) {
    DBG(15) << "not OK" << endl;
    ERR << "Could not connect! Make sure WMIR is started." << endl;
  } else {
    DBG(15) << "OK" << endl;
  }

  // Query WMIR and store the result in the rsv_table
  DBG(30) << "Sending query" << endl;
  sock << _query + "\r\n";
  string line;
  DBG(30) << "getting count" << endl;
  line = sock.getline();
  unsigned int count = atoi(line.c_str());
  DBG(30) << "It's " << count << endl;
    
  max_rsv_ = 0.0;
  for(unsigned int i = 0; i < count; ++i) {
    line = sock.getline();
    istringstream is(line);
    string filename, rsv_s;
    is >> filename;
    is >> rsv_s;
    double rsv = atof(rsv_s.c_str());
    if(rsv > max_rsv_) {
      max_rsv_ = rsv;
    }
    rsv_table_[filename] = rsv;
    DBG(30) << filename << " " << rsv << endl;
  }
    
  DBG(10) << "max_rsv="<<max_rsv_ << endl;


  if(count == 0) {
    DBG(30) << "No results!" << endl;
    max_rsv_ = 10000.0;
  }

  DBG(30) << "Maximum Retrieval Status Value for this query image: "
          << max_rsv_ << endl;

  DBG(30) << "Disconnecting" << endl;
    
  sock << ":bye\r\n";
  sock.close();
}

void TextFeatureDistance::set_rsv_table(const map<string, double>& _rsv_table) {
  rsv_table_ = _rsv_table;

  max_rsv_ = 0.0;
  if(rsv_table_.size()>0) {
    map<string, double>::iterator rsvit;
    for(rsvit = rsv_table_.begin(); rsvit != rsv_table_.end(); ++rsvit) {
      if(rsvit->second > max_rsv_) {
        max_rsv_ = rsvit->second;
      }
    }
  } else {
    DBG(20) << "Empty RSV table, assuming max_rsv=10000" << endl;
    max_rsv_ = 10000.0;
  }

  DBG(20) << "Maximum Retrieval Status Value for this query image: "
          << max_rsv_ << endl;
}

