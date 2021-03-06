//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//

#ifndef BASE64_H
#define BASE64_H

#include <string>

std::string base64_encode(unsigned char const* , size_t len);
std::string base64_decode(std::string const& s);

#endif /* BASE64_H */
