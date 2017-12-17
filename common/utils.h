
#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#define NOUSE(v) (void)(v);

#define _MIN_(a, b) (a)>=(b)?(b):(a)
#define _MAX_(a, b) (a)>=(b)?(a):(b)


static std::string RQSTID(int n=20) {
    static char CHARS[] = "abcdefghijklmnopqrstuvwxwz1234567890";
    std::string uuid(n, '0');
    for(size_t i=0; i<uuid.size(); i++)
        uuid[i] = CHARS[rand()%(sizeof(CHARS)-1)];
    return uuid;
}

static const std::vector<int32_t> iExplode(const std::string& s, const char& c) {
    std::string buff{""};
    std::vector<int32_t> v;
    for(const auto& n : s) {
        if(n != c) {
            buff += n;  
        } else if(n == c && buff != "") { 
            v.push_back(atoll(buff.c_str())); 
            buff = ""; 
        }   
    }   
    if(buff != "") 
        v.push_back((int32_t)atoll(buff.c_str()));
    return v;
}

static const std::vector<int64_t> i64Explode(const std::string& s, const char& c) {
    std::string buff{""};
    std::vector<int64_t> v;
    for(const auto& n : s) {
        if(n != c) {
            buff += n;  
        } else if(n == c && buff != "") { 
            v.push_back(atoll(buff.c_str())); 
            buff = ""; 
        }   
    }   
    if(buff != "") 
        v.push_back((int64_t)atoll(buff.c_str()));
    return v;
}

static const std::vector<std::string> sExplode(const std::string& s, const char& c) {
    std::string buff{""};
    std::vector<std::string> v;
    for(const auto& n : s) {
        if(n != c) {
            buff += n;  
        } else if(n == c && buff != "") { 
            v.push_back(buff); 
            buff = ""; 
        }   
    }   
    if(buff != "") 
        v.push_back(buff);
    return v;
}

static std::string sReplace(const std::string& s, const std::string& sub, const std::string& rep) {
    std::string cp{s};
    size_t pos = cp.find(sub);
    if(pos != std::string::npos) {
        cp.replace(pos, sub.size(), rep);
    }
    return cp;
}

static std::string& sReplaceIn(std::string& s, const std::string& sub, const std::string& rep) {
    size_t pos = s.find(sub);
    if(pos != std::string::npos) {
        s.replace(pos, sub.size(), rep);
    }
    return s;
}

template<typename T>
static const std::string Implode(std::vector<T>& lst, const char& c=',') {
    if(lst.empty()) return "";
    std::stringstream ss;
    for(auto& i : lst) {
        ss<<i<<c;
    }
    std::string s = ss.str();
    s.resize(s.size()-1);
    return s;
}


static std::string Upper(const std::string& s) {
    std::string s1{s};
    std::transform(s1.begin(), s1.end(), s1.begin(), toupper);
    return s1;
}
static std::string Lower(const std::string& s) {
    std::string s1{s};
    std::transform(s1.begin(), s1.end(), s1.begin(), tolower);
    return s1;
}

static void toUpper(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), toupper);
}
static void toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), tolower);
}

static time_t dateStrrToTimestamp(const char* s, const char* fmt) {
    tm _tm;
    strptime(s, "%Y-%m-%dT%H:%M:%SZ", &_tm);
    //strptime(s, fmt, &_tm);
    time_t t = mktime(&_tm);
    return t;

}

static void url_add_param(std::string& url, const std::string& key, const std::string& val) {
    size_t pos = url.find(key+"=");
    if(pos == std::string::npos) {
        size_t found = url.find('?');
        if(found == std::string::npos) {
            url = url + "?" + key + "=" + val; 
        } else {
            url = url + "&" + key + "=" + val; 
        }
        return;
    }
    size_t i = pos;
    for(; i<url.size(); i++) {
        if(url[i] == '&') {
            break;
        }
    }
    url.replace(pos, i-pos, key+"="+val);
}
