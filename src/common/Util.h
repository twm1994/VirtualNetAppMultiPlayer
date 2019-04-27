//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <climits>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <cstdarg>
#include <cstringtokenizer.h>
#include <cenvir.h>
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"
#include "../crypto/sha1.h"

using namespace std;
using namespace omnetpp;
using namespace boost;
//using namespace boost::tuples;

namespace util {

string getHostName(string& fullName);

int getHostIndex(string& fullName);

int strToInt(string& str);
unsigned short strToUShort(string& str);
unsigned long strToLong(string& str);
double strToDouble(string& str);

string intToStr(int i);
string longToStr(unsigned long i);

bool strStartWith(string str, string substr);

map<string, int> intersectMaps(map<string, int> map1, map<string, int> map2);
set<string> intersectSets(set<string> map1, set<string> map2);

void splitString(string& str, string delimitor, vector<string>& container);

// num indicates the number variable arguments, and variable arguments can only be const char*
string convertListToStr(vector<string>& list, int num...);

// num indicates the number variable arguments, and variable arguments can only be const char*
string convertMapKeyToStr(map<string, int>& m, int num... );
string convertSetToStr(set<string>& m, int num... );
void convertStrToSet(string str, set<string>& s);

//return an string of format id1:content1;id2:content2; ...
string convertMapToStr(map<int, string>& m);
string convertMapToStr(map<string, int>& m);
string convertMapToStr(map<string, string>& m);
string convertMapToStr(map<unsigned long, string>& m);
string convertMapToStr(map<unsigned long, int>& m);
// return an string of format round1:id1-event1,id2-event,...;round2:id1-event1,id2-event2,...;...
string convertMapToStr(map<int, map<unsigned long, string> >& m);
string convertMapToStr(map<unsigned long, map<string, string> >& m);
string convertMapToStr(map<boost::tuple<int, unsigned long, int>, string>& m);
string convertMapToStr(
        multimap<int, boost::tuple<unsigned long, int, string> >& m);

// return a map from a string of format id1:content1;id2:content2; ...
void convertStrToMap(string& str, map<int, string>& m);
void convertStrToMap(string& str, map<string, int>& m);
void convertStrToMap(string& str, map<string, string>& m);
void convertStrToMap(string& str, map<unsigned long, string>& m);
void convertStrToMap(string& str, map<unsigned long, int>& m);
// return a map from a string of format round1:id1-event1,id2-event,...;round2:id1-event1,id2-event2,...;...
void convertStrToMap(string& str, map<int, map<unsigned long, string> >& m);
void convertStrToMap(string& str, map<unsigned long, map<string, string> >& m);
void convertStrToMap(string& str,
        map<boost::tuple<int, unsigned long, int>, string>& m);
void convertStrToMap(string& str,
        multimap<int, boost::tuple<unsigned long, int, string> >& m);

// num indicates the number variable arguments, and variable arguments can only be const char*
vector<string> loadMapKey(map<string, int>& m, int num...);

string packSenderSeqRanges(
        map<unsigned long, boost::tuple<int, int> >& senders);
void unpackSenderSeqRanges(string& senderInfo,
        map<unsigned long, boost::tuple<int, int> >& senders);

/* Convert a string into an unsigned long in [0, spacesize] */
unsigned long getSHA1(string str, int spacesize);

template<class K, class V>
typename multimap<K, V>::const_iterator multimap_find(const multimap<K, V>& map,
        const pair<K, V>& pair) {
    typedef typename multimap<K, V>::const_iterator it;
    std::pair<it, it> range = map.equal_range(pair.first);
    for (it p = range.first; p != range.second; ++p)
        if (p->second == pair.second)
            return p;
    return map.end();
}
// insert a pair into a multimap if it does not exist
template<class K, class V>
bool multimap_insert(multimap<K, V>& map, const pair<K, V>& pair) {
    if (multimap_find(map, pair) == map.end()) {
        map.insert(pair);
        return true;
    }
    return false;
}

void string_to_binary(const string source, unsigned char* destination,
        unsigned int length);
void binary_to_string(const unsigned char* source, unsigned int length,
        string& destination);
}

#endif /* UTIL_H_ */
