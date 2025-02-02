#ifndef TYPES_HPP
#define TYPES_HPP
#include <iostream>
#include "include/json/json.h"
#include "Values.hpp"
#include "utils/Printer.hpp"
#include <set>
#include <cassert>
#include "include/cereal/archives/portable_binary.hpp"
#include "include/cereal/types/memory.hpp"
#include "include/cereal/types/vector.hpp"
#include "include/cereal/types/map.hpp"
#include "include/cereal/types/string.hpp"
#include "include/cereal/types/set.hpp"

struct Tool {
    ToolEnum type = NOT_EQUIPPED;
    int value = 100;

    template <class Archive> void serialize(Archive &archive) {
        archive(type, value);
    }
};

class Tags : public std::set<int> {
  public:
    Tags() = default;
    bool intersectsWith(const Tags &t) const;
    // Serialization inherited from std::set
};
class Materials;
class MaterialCategoryBuff {
  public:
    int vegetable;
    int meat;
    int fish;
    int creation;
    MaterialCategoryBuff() : vegetable(0), meat(0), fish(0), creation(0) {}
    void add(const MaterialCategoryBuff &m);
    std::vector<Printer> getPrinters() const;
    int *operator[](std::string name);
    int operator*(const Materials &m) const;
    template <class Archive> void serialize(Archive &archive) {
        archive(vegetable, meat, fish, creation);
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
    void add(const FlavorBuff &f);
    std::vector<Printer> getPrinters() const;
    int operator*(const FlavorEnum f) const;
    int *operator[](std::string name);

    template <class Archive> void serialize(Archive &archive) {
        archive(sweet, salty, sour, bitter, spicy, tasty);
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
    void multiply(double a);
    int operator/(const Ability &a) const;
    Ability operator+(Tool t) const;
    Ability operator&&(const Ability &a) const;
    int operator!=(int a) const;
    bool operator==(int a) const;
    void add(const Ability &a);
    void add(int a);
    std::vector<Printer> getPrinters(bool percent) const;
    int operator[](int name);
    int *operator[](std::string name);

    template <class Archive> void serialize(Archive &archive) {
        archive(stirfry, bake, boil, steam, fry, knife);
    }
};
class AbilityBuff;
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

    template <class Archive> void serialize(Archive &archive) {
        archive(cereal::base_class<Ability>(this));
    }
};
class AbilityBuff : public Ability {
  public:
    AbilityBuff() {}
    AbilityBuff(int stirfry, int bake, int boil, int steam, int fry, int knife)
        : Ability(stirfry, bake, boil, steam, fry, knife) {}
    int operator*(const CookAbility &a) const;
    template <class Archive> void serialize(Archive &archive) {
        archive(cereal::base_class<Ability>(this));
    }
};
class RarityBuff {
    int data[5] = {0, 0, 0, 0, 0};

  public:
    /*几火就是几，不用减一*/
    int &operator[](int i) { return data[i - 1]; }
    int operator[](int i) const { return data[i - 1]; }
    bool print() const;

    template <class Archive> void serialize(Archive &archive) { archive(data); }
};
class DiscretizedBuff {
    int data[5] = {0, 0, 0, 0, 0};

  public:
    /*几火/几级就是几，不用减一*/
    int &operator[](int i) { return data[i - 1]; }
    int operator[](int i) const { return data[i - 1]; }
    void add(const DiscretizedBuff &r);
    void print(const std::string &name,
               bool use_percentage_instead_of_plus = true) const;

    template <class Archive> void serialize(Archive &archive) { archive(data); }
};
class GradeBuff {
    int data[5] = {0, 0, 0, 0, 0};

  public:
    int operator[](int i) const { return data[i - 1]; }
    void gte_add(int threshold, int value);
    void operator+=(const GradeBuff &a);
    void print(const std::string &word, bool perc) const;
    template <class Archive> void serialize(Archive &archive) { archive(data); }
};
class AmountBuff {
  private:
    int buff[64] = {0};

  public:
    int operator[](int i) const { return buff[i + 1]; }
    void gte_add(int threshold, int value);
    void lte_add(int threshold, int value);
    void operator+=(const AmountBuff &a);
    void print(const std::string &word, bool perc) const;

    template <class Archive> void serialize(Archive &archive) { archive(buff); }
};

class BuffCondition;
class ConditionalBuff;
typedef std::vector<std::shared_ptr<ConditionalBuff>> ConditionalEffects;
struct BuffSum {
    int perc = 0;
    int basePerc = 0;
    int baseAbs = 0;
    BuffSum operator+(const BuffSum &b) const;
};
struct BuffSummary {
    BuffSum flavor;
    BuffSum material;
    BuffSum rarity;
    BuffSum grade;
    BuffSum amount;
    BuffSum ability;
    BuffSum unconditional;
    BuffSum sum() const;
};
class Chef;
class PriceBuffData {
  private:
    int unconditional = 0;
    AbilityBuff abilityBuff;
    FlavorBuff flavorBuff;
    MaterialCategoryBuff materialBuff;
    DiscretizedBuff rarityBuff;
    GradeBuff gradeBuff; // 几级就填当前那一级，比它高的不用填。
    AmountBuff amountBuff;
    friend class Skill;
    friend class Chef;
    void operator+=(const PriceBuffData &p);
    PriceBuffData operator+(const PriceBuffData &p) const;
    void print(const char *title, bool perc) const;

  public:
    template <class Archive> void serialize(Archive &archive) {
        archive(unconditional, abilityBuff, flavorBuff, materialBuff,
                rarityBuff, gradeBuff, amountBuff);
    }
};
class Skill;
class Recipe;
class Skill {

  public:
    enum Type { SELF, NEXT, PARTIAL, UNSET, GLOBAL };
    Type type = UNSET;
    Tags chefTagsForPARTIAL;
    // Whenever a new member is added, remember to update the add function.
    static std::map<int, Skill> skillList;
    static std::map<int, Skill> globalSkillList;
    static std::map<int, Skill> globalMaleSkillList;
    static std::map<int, Skill> globalFemaleSkillList;
    CookAbility ability;
    CookAbility cookAbilityPercentBuff;
    PriceBuffData pricePerc;
    PriceBuffData priceBasePerc;
    PriceBuffData priceBaseAbs;
    DiscretizedBuff amountAdd;
    // int multiToolEffect = 1; // 1: 正常, 2: 翻倍
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
    void print(bool printNum = false) const;
    ~Skill() {} // conditionalEffects should be handled manually.

    template <class Archive> void serialize(Archive &archive) {
        archive(type, chefTagsForPARTIAL, amountAdd, ability, pricePerc,
                priceBasePerc, priceBaseAbs);
    }

    /**
     * @return pricePerc, priceBasePerc, priceBaseAbs
     */
    BuffSummary getBuffs(const Recipe *r) const;
};

// Records nonconventional buffs that requires consideration of multiple dishes
class BuffCondition {

  public:
    std::string name;
    BuffCondition(const std::string &name = "") : name(name) {}
    virtual int test(const Skill *s, Recipe **r) = 0;
    virtual ~BuffCondition() {}

    template <class Archive> void serialize(Archive &archive) { archive(name); }
};
class GradeBuffCondition : public BuffCondition {
  public:
    int grade;
    GradeBuffCondition(int grade)
        : grade(grade),
          BuffCondition(std::string("等级做到") + (char)('0' + grade)) {}
    int test(const Skill *s, Recipe **r) override;
    ~GradeBuffCondition() override {}

    template <class Archive> void serialize(Archive &archive) {
        archive(cereal::base_class<BuffCondition>(this), grade);
    }
    template <class Archive>
    static void
    load_and_construct(Archive &ar,
                       cereal::construct<GradeBuffCondition> &construct) {
        int grade;
        ar(grade);
        construct(grade);
    } // Necessary, since no default constructor.
};

class ThreeSameCookAbilityBuffCondition : public BuffCondition {
  public:
    ThreeSameCookAbilityBuffCondition()
        : BuffCondition(std::string("三技法相同")) {}
    int test(const Skill *s, Recipe **r) override;
    ~ThreeSameCookAbilityBuffCondition() override {}

    template <class Archive> void serialize(Archive &archive) {
        archive(cereal::base_class<BuffCondition>(this));
    }
};
CEREAL_FORCE_DYNAMIC_INIT(Types)

class ConditionalBuff {
  public:
    std::shared_ptr<BuffCondition> conditionFunc;
    Skill skill;
    ConditionalBuff(std::shared_ptr<BuffCondition> conditionFunc, Skill skill)
        : conditionFunc(conditionFunc), skill(skill) {}
    ConditionalBuff() : conditionFunc(NULL), skill(Skill()) {}

    template <class Archive> void serialize(Archive &archive) {
        archive(conditionFunc, skill);
    }
};
#endif
