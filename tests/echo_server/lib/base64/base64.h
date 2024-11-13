#ifndef _BASE64_H_
#define _BASE64_H_
#include <string>
#include <vector>

class Base64
{

public:
    static std::string decode(const std::string& encoded_string);
    static std::string encode(const std::string &input) ;
};


#endif