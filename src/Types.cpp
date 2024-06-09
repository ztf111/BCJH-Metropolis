#include "Types.hpp"
#include "Recipe.hpp"
#include <cassert>

int GradeBuffCondition::test(const Skill *s, Recipe **r) {
    int count = 0;
    for (int i = 0; i < DISH_PER_CHEF; i++) {
        count += (s->ability / r[i]->cookAbility >= grade);
    }
    return count;
}

int ThreeSameCookAbilityBuffCondition::test(const Skill *s, Recipe **r) {
    Ability tmp(1, 1, 1, 1, 1, 1);
    for (int i = 0; i < DISH_PER_CHEF; i++) {
        tmp = tmp && r[i]->cookAbility;
    }
    return (tmp != 0);
}
void Skill::operator+=(const Skill &s) {
    this->ability.add(s.ability);
    this->cookAbilityPercentBuff.add(s.cookAbilityPercentBuff);
    this->abilityBuff.add(s.abilityBuff);
    this->abilityBaseBuff.add(s.abilityBaseBuff);
    this->flavorBuff.add(s.flavorBuff);
    this->materialBuff.add(s.materialBuff);
    this->rarityBuff.add(s.rarityBuff);
    this->gradeBuff.add(s.gradeBuff);
    this->pricePercentBuff += s.pricePercentBuff;
    this->baseAddBuff += s.baseAddBuff;
    this->conditionalEffects.insert(this->conditionalEffects.end(),
                                    s.conditionalEffects.begin(),
                                    s.conditionalEffects.end());
    if (this->type == PARTIAL || this->type == UNSET) {
        // PARTIAL: loading from json
        // UNSET: load to empty skill in chef
        assert(this->chefTagsForPARTIAL.size() == 0);
        this->chefTagsForPARTIAL = s.chefTagsForPARTIAL;
    }
    if (this->type == UNSET) {
        this->type = s.type;
    }
    if (this->type == PARTIAL || this->type == NEXT) {
        assert(this->type == s.type);
        // Only Unset and Self can merge with different types.
    }
}
