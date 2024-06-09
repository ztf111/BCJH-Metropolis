#include "Values.hpp"
#include <string>
#include <map>

int getEnum(const std::string &s) {
    static const std::map<std::string, int> enumMap = {
        {"Stirfry", STIRFRY}, {"Bake", BAKE},   {"Fry", FRY},
        {"Boil", BOIL},       {"Steam", STEAM}, {"Knife", KNIFE},
        {"Sweet", SWEET},     {"Sour", SOUR},   {"Bitter", BITTER},
        {"Tasty", TASTY},     {"Spicy", SPICY}, {"Salty", SALTY}};
    auto it = enumMap.find(s);
    if (it != enumMap.end()) {
        return it->second;
    }
    return -1;
}
std::string getNameByEnum(int e) {
    static const std::map<int, std::string> enumMap = {
        {STIRFRY, "Stirfry"}, {BAKE, "Bake"},   {FRY, "Fry"},
        {BOIL, "Boil"},       {STEAM, "Steam"}, {KNIFE, "Knife"},
        {SWEET, "Sweet"},     {SOUR, "Sour"},   {BITTER, "Bitter"},
        {TASTY, "Tasty"},     {SPICY, "Spicy"}, {SALTY, "Salty"}};
    auto it = enumMap.find(e);
    if (it != enumMap.end()) {
        return it->second;
    }
    return "Unknown";
}
