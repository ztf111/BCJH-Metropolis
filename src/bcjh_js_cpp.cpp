#include "bcjh_js.hpp"
#include "../data/data.hpp"
#include <iostream>
#include <codecvt>
#include <locale>

std::string unicodeToString(const std::string &str) {
    std::string u8str;
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    for (size_t i = 0; i < str.length();) {
        char32_t uhan = strtol(str.substr(i, 4).c_str(), nullptr, 16);
        u8str += converter.to_bytes(uhan);
        i += 4;
    }
    return u8str;
}

std::string decodeUnicodeEscape(const std::string &input) {
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size() && input[i + 1] == 'u') {
            // Extract the hexadecimal value after "\\u"
            std::string hexValue = input.substr(i + 2, 4);
            try {
                result += unicodeToString(hexValue);
                // Skip the next 5 characters (including "\\u")
                i += 5;
            } catch (const std::invalid_argument &) {
                // Invalid hexadecimal value, skip this escape sequence
                result += input[i];
            }
        } else if (input[i] == '\\' && i + 1 < input.size() &&
                   input[i + 1] == 't') {
            result += '\t';
            i += 1;
        } else if (input[i] == '\\' && i + 1 < input.size() &&
                   input[i + 1] == 'n') {
            result += '\n';
            i += 1;
        } else {
            // Not an escape sequence, just append the character
            result += input[i];
        }
    }
    return result;
}

int main() {
    auto str = runjs(
        userData_CONST, ruleData_CONST, 3170000, 1, 1, true,
        "N2tTdKAxg9OMsiiw03pQUQEB_wAJEAAAFwMOCA4AAgn_"
        "AwUICwMIDgQIDgUIDgYIDgcIDggIDgkEEhQDDQgODAMNCA4OAw4IDhAHCAASGP8HEAATGP"
        "8GCBMSGP8JExQg_wYAFRQY_wASaIf_WHJFr9-PQa__"
        "5xovCl8ARpcUhQFwPwNfADRfAEpGGTtwXwI0XwM0XwNKRhk7CXAZOwlwGTsJcBk7CXAZOw"
        "lwGTsJcBk7CTtwXwtGlxiFAXBfDDRCrwE8SglEPF8MSkYZO3BfDjRCpwM8SgkJRDxfDkpG"
        "GTtwXxA0Qq8DPEoJCQlEPF8QSkYZO3BfEzRKBM8IhM8IcF8VNEoEzwiECc8IhNcFzwhwOA"
        "ABADUyNzcAAP_vYm6mU_mBWmsj8NtQz7ejNXilXCddhXwKCktcCh_"
        "yoWd0cacNZKYg1jHBQsfoksPN5yqeB5FlEkSvjg-buFgm95OBc6kV5KmNygXO0BPr-"
        "lQwCuRQ2ignDMFnEQj_V0Yux-Z-D4lDAQySe6wihGo98aXv-"
        "0RTyhpFlQuoIANyonsWCzK0n67d7y_FASLwywvgHZrbsz4S2KBWegPLiDDj-dD5S72_"
        "uvJu87h1BA9nVU-iis-jW9o_1gMCuLZVYtq5Gvg9DV-N2F1u1ii_4Ky8wEw-etlkps_"
        "ZK9EzwyDYSSWCEiJhb9cIGOMhUyv_yFvjmAY02JzN3QM_"
        "HFTqbcGd8ofFvsCTC8lHsqsPM_yL8LU1rR4o46oJqulOmSqI5H_CU_0VvLq3e1sAAAAA_"
        "Rejilmz9PX0-kUi0yer6LghODwf_w..");
    // "呼延吒绛-切(100)",
    // "贺岁老君-炸(60)",
    // "兰染-蒸(60)",
    // "老君-切(100)",
    // "风息-切(60)",
    // "蒙晗-切(100)"
    // 1207805
    Json::Value json;
    std::stringstream ss(str);
    ss >> json;
    json.removeMember("logs");
    auto str2 = json.toStyledString();
    // encode all unicodes in the string
    str = decodeUnicodeEscape(str2);
    std::cout << str << std::endl;

    // std::string input = "\\u968f\\u673"; // Your input string
    // std::string decoded = decodeUnicodeEscape(input);
    // std::cout << "Decoded string: " << decoded << std::endl;
    return 0;
}