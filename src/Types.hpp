#ifndef TYPES_HPP
#define TYPES_HPP
#include <iostream>
#include "include/json/json.h"
#include "Values.hpp"
#include "utils/Printer.hpp"
#include <set>
#include <cassert>
struct Tool {
    static bool allowTool;
    ToolEnum type = NOT_EQUIPPED;
    int value = 100;
};
class Tags : public std::set<int> {
  public:
    Tags() = default;
    bool intersectsWith(const Tags &t) const {
        for (auto i : t) {
            if (this->count(i))
                return true;
        }
        return false;
    }
};
class MaterialCategoryBuff {
  public:
    int vegetable;
    int meat;
    int fish;
    int creation;
    MaterialCategoryBuff() : vegetable(0), meat(0), fish(0), creation(0) {}
    void add(const MaterialCategoryBuff &m) {
        this->vegetable += m.vegetable;
        this->meat += m.meat;
        this->fish += m.fish;
        this->creation += m.creation;
    }
    std::vector<Printer> getPrinters() const {
        std::vector<Printer> p;
        p.push_back(Printer("菜", vegetable));
        p.push_back(Printer("肉", meat));
        p.push_back(Printer("鱼", fish));
        p.push_back(Printer("面", creation));
        return p;
    }
    int *operator[](std::string name) {
        if (name == "Vegetable") {
            return &this->vegetable;
        }
        if (name == "Meat") {
            return &this->meat;
        }
        if (name == "Fish") {
            return &this->fish;
        }
        if (name == "Creation") {
            return &this->creation;
        }
        return NULL;
    }
};

class FlavorBuff {
  public:
    // Order MATTERS!
    int sweet;
    int salty;
    int sour;
    int bitter;
    int spicy;
    int tasty;
    FlavorBuff() : sweet(0), salty(0), sour(0), bitter(0), spicy(0), tasty(0) {}
    void add(const FlavorBuff &f) {
        this->sweet += f.sweet;
        this->salty += f.salty;
        this->sour += f.sour;
        this->bitter += f.bitter;
        this->spicy += f.spicy;
        this->tasty += f.tasty;
    }
    std::vector<Printer> getPrinters() const {
        std::vector<Printer> p;
        p.push_back(Printer("甜", sweet));
        p.push_back(Printer("咸", salty));
        p.push_back(Printer("酸", sour));
        p.push_back(Printer("苦", bitter));
        p.push_back(Printer("辣", spicy));
        p.push_back(Printer("鲜", tasty));
        return p;
    }
    int operator*(const FlavorEnum f) const {
        const int *ptr = &this->sweet;
        return ptr[f - FLAVOR_ENUM_START - 1];
    }
    int *operator[](std::string name) {
        if (name == "Sweet") {
            return &this->sweet;
        }
        if (name == "Salty") {
            return &this->salty;
        }
        if (name == "Sour") {
            return &this->sour;
        }
        if (name == "Bitter") {
            return &this->bitter;
        }
        if (name == "Spicy") {
            return &this->spicy;
        }
        if (name == "Tasty") {
            return &this->tasty;
        }
        return NULL;
    }
};
class Ability {
  public:
    int stirfry = 0;
    int bake = 0;
    int boil = 0;
    int steam = 0;
    int fry = 0;
    int knife = 0;
    Ability() : stirfry(0), bake(0), boil(0), steam(0), fry(0), knife(0) {}
    Ability(int stirfry, int bake, int boil, int steam, int fry, int knife)
        : stirfry(stirfry), bake(bake), boil(boil), steam(steam), fry(fry),
          knife(knife) {}
    Ability(Tool tool) {
        ToolEnum t = tool.type;
        int value = tool.value;
        if (t == NOT_EQUIPPED || t == NO_TOOL)
            return;
        int *ptr = &this->stirfry;
        ptr[t - ABILITY_ENUM_START] = value;
    }
    void multiply(double a) {
        int *ptr = &this->stirfry;
        for (int i = 0; i < 6; i++) {
            ptr[i] = int(ptr[i] * a);
        }
    }
    int operator/(const Ability &a) const {
        int grade = 5;
        const int *thisptr = &this->stirfry;
        const int *aptr = &a.stirfry;
        for (int i = 0; i < 6; i++) {
            if (aptr[i] != 0) {
                grade = grade < (thisptr[i] / aptr[i]) ? grade
                                                       : (thisptr[i] / aptr[i]);
            }
        }
        grade = grade < 0 ? 0 : grade;
        return grade;
    };
    Ability operator+(Tool t) const {
        Ability tmp(t);
        tmp.add(*this);
        return tmp;
    }
    Ability operator&&(const Ability &a) const {
        Ability tmp;
        const int *thisptr = &this->stirfry;
        const int *aptr = &a.stirfry;
        int *tmpptr = &tmp.stirfry;
        for (int i = 0; i < 6; i++) {
            tmpptr[i] = thisptr[i] && aptr[i];
        }
        return tmp;
    }
    int operator!=(int a) const {
        const int *ptr = &this->stirfry;
        int count = 0;
        for (int i = 0; i < 6; i++) {
            count += (ptr[i] != a);
        }
        return count;
    }
    bool operator==(int a) const {
        const int *ptr = &this->stirfry;
        for (int i = 0; i < 6; i++) {
            if (ptr[i] != a)
                return false;
        }
        return true;
    }
    void add(const Ability &a) {
        int *thisptr = &this->stirfry;
        const int *aptr = &a.stirfry;
        for (int i = 0; i < 6; i++) {
            thisptr[i] += aptr[i];
        }
    }
    void add(int a) {
        int *ptr = &this->stirfry;
        for (int i = 0; i < 6; i++) {
            ptr[i] += a;
        }
    }
    std::vector<Printer> getPrinters(bool percent) const {
        std::vector<Printer> p;
        p.push_back(Printer("炒", stirfry, percent));
        p.push_back(Printer("烤", bake, percent));
        p.push_back(Printer("煮", boil, percent));
        p.push_back(Printer("蒸", steam, percent));
        p.push_back(Printer("炸", fry, percent));
        p.push_back(Printer("切", knife, percent));
        return p;
    }
    /* Knife, Stirfry, Bake, Boil, Steam, Fry */
    int operator[](int name) {
        if (name == KNIFE) {
            return this->knife;
        }
        if (name == STIRFRY) {
            return this->stirfry;
        }
        if (name == BAKE) {
            return this->bake;
        }
        if (name == BOIL) {
            return this->boil;
        }
        if (name == STEAM) {
            return this->steam;
        }
        if (name == FRY) {
            return this->fry;
        }
        std::cout << "Ability::operator[]: Error: " << name << std::endl;
        exit(1);
        return -1;
    }
    int *operator[](std::string name) {
        if (name == "Knife") {
            return &this->knife;
        }
        if (name == "Stirfry") {
            return &this->stirfry;
        }
        if (name == "Bake") {
            return &this->bake;
        }
        if (name == "Boil") {
            return &this->boil;
        }
        if (name == "Steam") {
            return &this->steam;
        }
        if (name == "Fry") {
            return &this->fry;
        }
        return NULL;
    }
};

class AbilityBuff : public Ability {
  public:
    AbilityBuff() {}
    AbilityBuff(int stirfry, int bake, int boil, int steam, int fry, int knife)
        : Ability(stirfry, bake, boil, steam, fry, knife) {}
};
class CookAbility : public Ability {

  public:
    CookAbility(int stirfry, int bake, int boil, int steam, int fry, int knife)
        : Ability(stirfry, bake, boil, steam, fry, knife) {}
    CookAbility() : Ability() {}
    CookAbility(const Json::Value &v);
    // int operator/(const Ability &a) const;
    void print(std::string end = "\t") const {
        Printer p("技法");
        p.noValue();
        p.add(Ability::getPrinters(false));
        p.print("", " ", end.c_str());
    }
    int operator*(const AbilityBuff &a) const;
};
class RarityBuff {
    int data[5] = {0, 0, 0, 0, 0};

  public:
    /*几火就是几，不用减一*/
    int &operator[](int i) { return data[i - 1]; }
    int operator[](int i) const { return data[i - 1]; }
    bool print() const {
        if (data[0] || data[1] || data[2] || data[3] || data[4]) {
            std::cout << "稀有度加成: ";
            for (int i = 0; i < 5; i++) {
                std::cout << data[i] << "% ";
            }
            return true;
        } else {
            return false;
        }
    }
};
class DiscretizedBuff {
    int data[5] = {0, 0, 0, 0, 0};

  public:
    typedef std::tuple<bool, bool, bool, bool, bool> Mask;
    /*几火/几级就是几，不用减一*/
    int &operator[](int i) { return data[i - 1]; }
    int operator[](int i) const { return data[i - 1]; }
    void add(const DiscretizedBuff &r) {
        for (int i = 0; i < 5; i++)
            this->data[i] += r.data[i];
    }
    void print(const std::string &name) const {
        int sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += data[i];
        }
        if (sum > 0) {
            std::cout << name << ": ";
            for (int i = 0; i < 5; i++) {
                std::cout << data[i] << "% ";
            }
        }
    }
    void masked_add(Mask m, int value) {
        bool mask[5] = {std::get<0>(m), std::get<1>(m), std::get<2>(m),
                        std::get<3>(m), std::get<4>(m)};
        for (int i = 0; i < 5; i++) {
            if (mask[i]) {
                this->data[i] += value;
            }
        }
    }
};

class BuffCondition;
class ConditionalBuff;
typedef std::vector<std::shared_ptr<ConditionalBuff>> ConditionalEffects;

class Skill {

  public:
    enum Type { SELF, NEXT, PARTIAL, UNSET };
    Type type = UNSET;
    Tags chefTagsForPARTIAL;
    // Whenever a new member is added, remember to update the add function.
    static std::map<int, Skill> skillList;
    CookAbility ability;
    CookAbility cookAbilityPercentBuff;
    AbilityBuff abilityBuff;
    AbilityBuff abilityBaseBuff;
    FlavorBuff flavorBuff;
    FlavorBuff flavorBaseBuff;
    MaterialCategoryBuff materialBuff;
    MaterialCategoryBuff materialBaseBuff;
    DiscretizedBuff rarityBuff;
    DiscretizedBuff rarityBaseBuff;
    DiscretizedBuff gradeBuff; // 几级就填当前那一级，比它高的不用填。
    int multiToolEffect = 1;   // 1: 正常, 2: 翻倍

    int pricePercentBuff = 0;
    int baseAddBuff = 0;
    ConditionalEffects conditionalEffects;
    Skill() = default;
    Skill(Type type) : type(type) {}
    static void loadJson(const Json::Value &v);
    void operator+=(const Skill &s);
    Skill operator+(const Skill &s) {
        Skill tmp(*this);
        tmp += s;
        return tmp;
    }
    void print() const {
        this->ability.print("\t");
        Printer p("\n加成");
        p.noValue();
        p.add(abilityBuff.getPrinters(true));
        p.add(flavorBuff.getPrinters());
        p.add(materialBuff.getPrinters());
        p.add("金币", this->pricePercentBuff, true);
        p.add("基础售价", this->baseAddBuff, true);
        p.print("", "  ", "\t");
        if (!(this->abilityBaseBuff == 0)) {
            Printer p("基础技法加成");
            p.noValue();
            p.add(abilityBaseBuff.getPrinters(true));
            p.add(flavorBaseBuff.getPrinters());
            p.add(materialBaseBuff.getPrinters());
            p.print("", "  ", "\t");
        }
        this->rarityBuff.print("菜品火数加成");
        this->gradeBuff.print("菜品品级加成");

        std::cout << std::endl;
    }
    ~Skill() {} // conditionalEffects should be handled manually.
};
class Skill;
class Recipe;
// Records nonconventional buffs that requires consideration of multiple dishes
class BuffCondition {

  public:
    const std::string name;
    BuffCondition(const std::string &name = "") : name(name) {}
    virtual int test(const Skill *s, Recipe **r) = 0;
    virtual ~BuffCondition() {}
};
class GradeBuffCondition : public BuffCondition {
  public:
    int grade;
    GradeBuffCondition(int grade)
        : grade(grade),
          BuffCondition(std::string("等级做到") + (char)('0' + grade)) {}
    int test(const Skill *s, Recipe **r) override;
    ~GradeBuffCondition() override {}
};
class ThreeSameCookAbilityBuffCondition : public BuffCondition {
  public:
    ThreeSameCookAbilityBuffCondition() : BuffCondition("三技法相同") {}
    int test(const Skill *s, Recipe **r) override;
    ~ThreeSameCookAbilityBuffCondition() override {}
};

class ConditionalBuff {
  public:
    BuffCondition *conditionFunc;
    Skill skill;
    ConditionalBuff(BuffCondition *conditionFunc, Skill skill)
        : conditionFunc(conditionFunc), skill(skill) {}
    ConditionalBuff() : conditionFunc(NULL), skill(Skill()) {}
    ~ConditionalBuff() {
        // if (conditionFunc != NULL) // wierd, shouldn't need this.
        delete conditionFunc;
    }
};
#endif
