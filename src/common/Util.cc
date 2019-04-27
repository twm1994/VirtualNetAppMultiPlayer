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

#include "Util.h"

string util::getHostName(string& fullName) {
    int end = fullName.find("[");
    string hostName = fullName.substr(0, end);
    return hostName;
}

int util::getHostIndex(string& fullName) {
    int i = -1;
    size_t start = fullName.find("[") + 1;
    size_t end = fullName.find("]");
    if (start != string::npos && end != string::npos) {
        size_t range = end - start;
        string index = fullName.substr(start, range);
        if (!index.empty()) {
            i = util::strToInt(index);
        }
    }
    return i;
}

int util::strToInt(string& str) {
    int value = INT_MIN;
    stringstream ss(str);
    ss >> value;
    return value;
}

unsigned short util::strToUShort(string& str) {
    unsigned short value = 0;
    stringstream ss(str);
    ss >> value;
    return value;
}

unsigned long util::strToLong(string& str) {
    unsigned long value = INT_MIN;
    stringstream ss(str);
    ss >> value;
    return value;
}

double util::strToDouble(string& str) {
    double value = DBL_MIN;
    stringstream ss(str);
    ss >> value;
    return value;
}

string util::intToStr(int i) {
    stringstream ss;
    ss << i;
    return ss.str();
}

string util::longToStr(unsigned long i) {
    stringstream ss;
    ss << i;
    return ss.str();
}

bool util::strStartWith(string str, string substr) {
    return strncmp(str.c_str(), substr.c_str(), strlen(substr.c_str())) == 0;
}

map<string, int> util::intersectMaps(map<string, int> map1,
        map<string, int> map2) {
    map<string, int> map3;
    for (map<string, int>::iterator it = map1.begin(); it != map1.end(); it++) {
        string key = it->first;
        int value = it->second;
        if (map2.find(key) != map2.end()) {
            map3[key] = value;
        }
    }
    return map3;
}

set<string> util::intersectSets(set<string> set1, set<string> set2) {
    set<string> set3;
    for (set<string, int>::iterator it = set1.begin(); it != set1.end(); it++) {
        string element = *it;
        if (set2.count(element) > 0) {
            set3.insert(element);
        }
    }
    return set3;
}

void util::splitString(string& str, string delimitor,
        vector<string>& container) {
    cStringTokenizer tokenizer(str.c_str(), delimitor.c_str());
    while (tokenizer.hasMoreTokens()) {
        const char* token = tokenizer.nextToken();
//        EV << "entries contains: " << token << endl;
        container.push_back(token);
    }
}

string util::convertListToStr(vector<string>& list, int num...) {
    stringstream ss;

    va_list args;
    va_start(args, num);
    for(int i = 0; i < num; i++) {
        const char* token = va_arg(args, const char*);
        ss << token << ";";
    }
    va_end(args);

    for (size_t i = 0; i < list.size(); i++) {
        ss << list[i] << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapKeyToStr(map<string, int>& m, int num...) {
    stringstream ss;

    va_list args;
    va_start(args, num);
    for(int i = 0; i < num; i++) {
        const char* token = va_arg(args, const char*);
        ss << token << ";";
    }
    va_end(args);

    for (map<string, int>::iterator it = m.begin(); it != m.end(); ++it) {
        string token = it->first;
        ss << token << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertSetToStr(set<string>& m, int num...) {
    stringstream ss;

    va_list args;
    va_start(args, num);
    for(int i = 0; i < num; i++) {
        const char* token = va_arg(args, const char*);
        ss << token << ";";
    }
    va_end(args);

    for (set<string>::iterator it = m.begin(); it != m.end(); ++it) {
        string token = *it;
        ss << token << ";";
    }
    string content = ss.str();
    return content;
}

void util::convertStrToSet(string str, set<string>& s) {
    // empty map first;
    s.clear();
    vector<string> tokens;
    splitString(str, ";", tokens);
    for (size_t i = 0; i < tokens.size(); i++) {
        string token = tokens[i];
        s.insert(token);
    }
}

string util::convertMapToStr(map<int, string>& m) {
    stringstream ss;
    for (map<int, string>::iterator it = m.begin(); it != m.end(); ++it) {
        string id = intToStr(it->first);
        string value = it->second;
        ss << id << ":" << value << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(map<string, int>& m) {
    stringstream ss;
    for (map<string, int>::iterator it = m.begin(); it != m.end(); ++it) {
        string first = it->first;
        string second = intToStr(it->second);
        ss << first << ":" << second << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(map<string, string>& m) {
    stringstream ss;
    for (map<string, string>::iterator it = m.begin(); it != m.end(); ++it) {
        string first = it->first;
        string second = it->second;
        ss << first << ":" << second << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(map<unsigned long, string>& m) {
    stringstream ss;
    for (map<unsigned long, string>::iterator it = m.begin(); it != m.end();
            ++it) {
        string id = longToStr(it->first);
        string value = it->second;
        ss << id << ":" << value << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(map<unsigned long, int>& m) {
    stringstream ss;
    for (map<unsigned long, int>::iterator it = m.begin(); it != m.end();
            ++it) {
        string id = longToStr(it->first);
        string value = intToStr(it->second);
        ss << id << ":" << value << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(map<int, map<unsigned long, string> >& m) {
    stringstream ss;
    for (map<int, map<unsigned long, string> >::iterator it = m.begin();
            it != m.end(); ++it) {
        string round = intToStr(it->first);
        ss << round << ":";
        if (!it->second.empty()) {
            for (map<unsigned long, string>::iterator it2 = it->second.begin();
                    it2 != it->second.end(); ++it2) {
                string id = longToStr(it2->first);
                string event = it2->second;
                ss << id << "-" << event << ",";
            }
        }
        ss << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(map<unsigned long, map<string, string> >& m) {
    stringstream ss;
    for (map<unsigned long, map<string, string> >::iterator it = m.begin();
            it != m.end(); ++it) {
        string round = longToStr(it->first);
        ss << round << ":";
        if (!it->second.empty()) {
            for (map<string, string>::iterator it2 = it->second.begin();
                    it2 != it->second.end(); ++it2) {
                string id = it2->first;
                string event = it2->second;
                ss << id << "-" << event << ",";
            }
        }
        ss << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(
        map<boost::tuple<int, unsigned long, int>, string>& m) {
    stringstream ss;
    for (map<boost::tuple<int, unsigned long, int>, string>::iterator it =
            m.begin(); it != m.end(); it++) {
        boost::tuple<int, unsigned long, int> first = it->first;
        string second = it->second;
        ss << get<0>(first) << "," << get<1>(first) << "," << get<2>(first)
                << "," << second << ";";
    }
    string content = ss.str();
    return content;
}

string util::convertMapToStr(
        multimap<int, boost::tuple<unsigned long, int, string> >& m) {
    stringstream ss;
    for (multimap<int, boost::tuple<unsigned long, int, string> >::iterator it =
            m.begin(); it != m.end(); it++) {
        int first = it->first;
        boost::tuple<unsigned long, int, string> second = it->second;
        ss << first << "," << get<0>(second) << "," << get<1>(second) << ","
                << get<2>(second) << ";";
    }
    string content = ss.str();
    return content;
}

void util::convertStrToMap(string& str, map<int, string>& m) {
    // empty map first;
    m.clear();
    vector<string> tokens;
    splitString(str, ";", tokens);
    for (size_t i = 0; i < tokens.size(); i++) {
        string token = tokens[i];
        vector<string> pair;
        splitString(token, ":", pair);
        int id = strToInt(pair[0]);
        if (pair.size() > 1) {
            m[id] = pair[1];
        } else {
            m[id] = "";
        }
    }
}

void util::convertStrToMap(string& str, map<string, int>& m) {
    // empty map first;
    m.clear();
    vector<string> tokens;
    splitString(str, ";", tokens);
    for (size_t i = 0; i < tokens.size(); i++) {
        string token = tokens[i];
        vector<string> pair;
        splitString(token, ":", pair);
        string key = pair[0];
        if (pair.size() > 1) {
            m[key] = strToInt(pair[1]);
        } else {
            m[key] = 0;
        }
    }
}

void util::convertStrToMap(string& str, map<string, string>& m) {
    // empty map first;
    m.clear();
    vector<string> tokens;
    splitString(str, ";", tokens);
    for (size_t i = 0; i < tokens.size(); i++) {
        string token = tokens[i];
        vector<string> pair;
        splitString(token, ":", pair);
        string key = pair[0];
        if (pair.size() > 1) {
            m[key] = pair[1];
        } else {
            m[key] = "";
        }
    }
}

void util::convertStrToMap(string& str, map<unsigned long, string>& m) {
    // empty map first;
    m.clear();
    vector<string> tokens;
    splitString(str, ";", tokens);
    for (size_t i = 0; i < tokens.size(); i++) {
        string token = tokens[i];
        vector<string> pair;
        splitString(token, ":", pair);
        unsigned long id = strToLong(pair[0]);
        if (pair.size() > 1) {
            m[id] = pair[1];
        } else {
            m[id] = "";
        }
    }
}

void util::convertStrToMap(string& str, map<unsigned long, int>& m) {
    // empty map first;
    m.clear();
    vector<string> tokens;
    splitString(str, ";", tokens);
    for (size_t i = 0; i < tokens.size(); i++) {
        string token = tokens[i];
        vector<string> pair;
        splitString(token, ":", pair);
        unsigned long id = strToInt(pair[0]);
        m[id] = strToInt(pair[1]);
    }
}

void util::convertStrToMap(string& str,
        map<int, map<unsigned long, string> >& m) {
    // empty map first;
    m.clear();
    vector<string> L1Tokens;
    splitString(str, ";", L1Tokens);
    for (size_t i = 0; i < L1Tokens.size(); i++) {
        string token = L1Tokens[i];
        vector<string> pair1;
        splitString(token, ":", pair1);
        int round = strToInt(pair1[0]);
        string roundEvents = pair1[1];
        vector<string> L2Tokens;
        splitString(roundEvents, ",", L2Tokens);
        for (size_t j = 0; j < L2Tokens.size(); j++) {
            string token2 = L2Tokens[j];
            vector<string> pair2;
            splitString(token2, "-", pair2);
            unsigned long senderId = strToLong(pair2[0]);
            string event = pair2[1];
            if (pair2.size() > 1) {
                m[round][senderId] = event;
            } else {
                m[round][senderId] = "";
            }
        }
    }
}

void util::convertStrToMap(string& str,
        map<unsigned long, map<string, string> >& m) {
    // empty map first;
    m.clear();
    vector<string> L0Tokens;
    splitString(str, ";", L0Tokens);
    for (size_t i = 0; i < L0Tokens.size(); i++) {
        string token0 = L0Tokens[i];
        vector<string> pair1;
        splitString(token0, ":", pair1);
        unsigned long key0 = strToLong(pair1[0]);
        string value0 = pair1[1];
        vector<string> L1Tokens;
        splitString(value0, ",", L1Tokens);
        for (size_t j = 0; j < L1Tokens.size(); j++) {
            string token1 = L1Tokens[j];
            vector<string> pair2;
            splitString(token1, "-", pair2);
            string key1 = pair2[0];
            string value1 = pair2[1];
            if (pair2.size() > 1) {
                m[key0][key1] = value1;
            } else {
                m[key0][key1] = "";
            }
        }
    }
}

void util::convertStrToMap(string& str,
        map<boost::tuple<int, unsigned long, int>, string>& m) {
    // empty map first;
    m.clear();
    vector<string> L1Tokens;
    splitString(str, ";", L1Tokens);
    for (size_t i = 0; i < L1Tokens.size(); i++) {
        string token = L1Tokens[i];
        vector<string> elements;
        splitString(token, ",", elements);
        int round = strToInt(elements[0]);
        unsigned long senderId = strToLong(elements[1]);
        unsigned long seq = strToInt(elements[2]);
        boost::tuple<int, unsigned long, int> key(round, senderId, seq);
        m[key] = elements[3];
    }
}

void util::convertStrToMap(string& str,
        multimap<int, boost::tuple<unsigned long, int, string> >& m) {
    // empty map first;
    m.clear();
    vector<string> L1Tokens;
    splitString(str, ";", L1Tokens);
    for (size_t i = 0; i < L1Tokens.size(); i++) {
        string token = L1Tokens[i];
        vector<string> elements;
        splitString(token, ",", elements);
        int round = strToInt(elements[0]);
        unsigned long senderId = strToLong(elements[1]);
        unsigned long seq = strToInt(elements[2]);
        string payload = elements[3];
        boost::tuple<unsigned long, int, string> event(senderId, seq, payload);
        m.insert( { round, event });
    }
}

vector<string> util::loadMapKey(map<string, int>& m, int num...) {
    vector<string> list;

    va_list args;
    va_start(args, num);
    for(int i = 0; i < num; i++) {
        const char* token = va_arg(args, const char*);
        list.push_back(token);
    }
    va_end(args);

    for (map<string, int>::iterator it = m.begin(); it != m.end(); ++it) {
        string key = it->first;
        list.push_back(key);
    }
    return list;
}

string util::packSenderSeqRanges(
        map<unsigned long, boost::tuple<int, int> >& senders) {
    stringstream ss;
    for (map<unsigned long, boost::tuple<int, int> >::iterator it =
            senders.begin(); it != senders.end(); it++) {
        unsigned long senderId = it->first;
        boost::tuple<int, int> range = it->second;
        int lowestSeq = get<0>(range);
        int sor = get<1>(range);
        ss << senderId << "," << lowestSeq << "," << sor << ";";
    }
    string senderInfos = ss.str();
    return senderInfos;
}

void util::unpackSenderSeqRanges(string& senderInfo,
        map<unsigned long, boost::tuple<int, int> >& senders) {
    senders.clear();
    vector<string> senderInfos;
    splitString(senderInfo, ";", senderInfos);
    for (size_t i = 0; i < senderInfos.size(); i++) {
        vector<string> infos;
        splitString(senderInfos[i], ",", infos);
        unsigned long senderId = strToLong(infos[0]);
        int lowestSeq = strToInt(infos[1]);
        int sor = strToInt(infos[2]);
        boost::tuple<int, int> seqRange(lowestSeq, sor);
        senders[senderId] = seqRange;
    }
}

void util::string_to_binary(const string source, unsigned char* destination,
        unsigned int size) {
    unsigned int effective_length = min(size,
            (unsigned int) source.length() / 2);
    for (unsigned int b = 0; b < effective_length; b++) {
        unsigned int ui;
        sscanf(source.data() + (b * 2), "%02x", &ui);
        destination[b] = ui;
    }
}

void util::binary_to_string(const unsigned char* source, unsigned int size,
        string& destination) {
    for (unsigned int i = 0; i < size; i++) {
        char digit[3];
        sprintf(digit, "%02x", source[i]);
        destination.append(digit);
    }
}

unsigned long util::getSHA1(string str, int spacesize) {
    SHA1 *sha1 = new SHA1();
    sha1->addBytes(str.c_str(), strlen(str.c_str()));
    unsigned char* digest = sha1->getDigest();
    unsigned long res = (unsigned long) sha1->shaToInteger(digest, 20,
            pow(2, spacesize));
    delete sha1;
    free(digest);
    return res;
}
