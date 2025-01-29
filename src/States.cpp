#include "States.hpp"
#include "utils/math.hpp"
double getStatesSkillsTime = 0;

template <typename T> inline void copy(T *dst, const T *src, int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

void mergeSkills(Skill *skillsResult, const Skill *selfSkills,
                 const Skill *companyBuffs, const Skill *nextBuffs,
                 Tags *const *tagOfCompanyBuff, const Chef *chefs) {
    for (size_t i = 0; i < NUM_CHEFS; i++) {
        skillsResult[i] = selfSkills[i];
        skillsResult[i].ability.add(Ability(chefs[i].getTool()));
    }
    // 光环
    for (size_t g = 0; g < NUM_GUESTS; g++) {
        for (size_t src = g * CHEFS_PER_GUEST; src < (g + 1) * CHEFS_PER_GUEST;
             src++) {
            for (size_t dst = src; dst < (g + 1) * CHEFS_PER_GUEST; dst++) {
                if (tagOfCompanyBuff[src]->intersectsWith(
                        *chefs[dst].tagForCompanyBuff) ||
                    dst == src) {
                    skillsResult[dst] += companyBuffs[src];
                }
            }
        }
        for (size_t i = g * CHEFS_PER_GUEST + 1; i < (g + 1) * CHEFS_PER_GUEST;
             i++) {
            skillsResult[i] += nextBuffs[i - 1];
        }
    }
    // 技法先加减后乘除
    for (size_t i = 0; i < NUM_CHEFS; i++) {
        CookAbility &b = skillsResult[i].cookAbilityPercentBuff;
        CookAbility &r = skillsResult[i].ability;
        int *rPtr = &r.stirfry;
        int *bPtr = &b.stirfry;
        for (int j = 0; j < 6; j++) {
            rPtr[j] = int_ceil(rPtr[j] * (bPtr[j] + 100.0) / 100.0);
        }
    }
}
inline void applyConditionBuff(const Skill *const cookAbilitySkill,
                               const ConditionalEffects &conditionalEffects,
                               Skill *skillTarget, Recipe **recipe) {
    // 条件技能
    for (auto &ce : conditionalEffects) {
        for (int i = ce->conditionFunc->test(cookAbilitySkill, recipe); i > 0;
             i--) {
            *skillTarget += ce->skill;
        }
    }
}
const Skill *States::getCookAbilities(FLAG_getCookAbilities flag) {
    // Not related to specific dish
    if (cookAbilitiesValid && (flag == DEFAULT)) {
        return cookAbilitiesCache.data();
    }
    std::vector<Skill> selfSkills(NUM_CHEFS);
    std::vector<Skill> companySkills(NUM_CHEFS);
    std::vector<Skill> nextSkills(NUM_CHEFS);
    std::vector<Tags *> tagOfCompanyBuff(NUM_CHEFS);
    for (size_t i = 0; i < NUM_CHEFS; i++) {
        selfSkills[i] = *chefs[i].skill;
        companySkills[i] = *chefs[i].companyBuff;
        nextSkills[i] = *chefs[i].nextBuff;
        tagOfCompanyBuff[i] = &chefs[i].companyBuff->chefTagsForPARTIAL;
    }
    cookAbilitiesValid = true;

    mergeSkills(cookAbilitiesCache.data(), selfSkills.data(),
                companySkills.data(), nextSkills.data(),
                tagOfCompanyBuff.data(), chefs.data());
    return cookAbilitiesCache.data();
}

void States::getSkills(Skill *skills, FLAG_getCookAbilities flag) {
#ifdef MEASURE_TIME
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif
    if (!chefHasStrangeSkills) {
        copy<Skill>(skills, getCookAbilities(flag), NUM_CHEFS);
#ifdef MEASURE_TIME
        clock_gettime(CLOCK_MONOTONIC, &end);
        getStatesSkillsTime +=
            (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
#endif
        return;
    }
    // Skills needs to be copied to test conditional effects
    Skill selfSkills[NUM_CHEFS];
    Skill companySkills[NUM_CHEFS];
    Skill nextSkills[NUM_CHEFS];
    Tags *tagOfCompanyBuff[NUM_CHEFS];
    for (size_t i = 0; i < NUM_CHEFS; i++) {
        selfSkills[i] = *chefs[i].skill;
        companySkills[i] = *chefs[i].companyBuff;
        nextSkills[i] = *chefs[i].nextBuff;
        tagOfCompanyBuff[i] = &chefs[i].companyBuff->chefTagsForPARTIAL;
    }
    auto skillsPreview = getCookAbilities(flag);
    for (size_t i = 0; i < NUM_CHEFS; i++) {
        applyConditionBuff(skillsPreview + i,
                           chefs[i].skill->conditionalEffects, selfSkills + i,
                           recipe.data() + i * DISH_PER_CHEF);
        applyConditionBuff(
            skillsPreview + i, chefs[i].companyBuff->conditionalEffects,
            companySkills + i, recipe.data() + i * DISH_PER_CHEF);
        applyConditionBuff(skillsPreview + i,
                           chefs[i].nextBuff->conditionalEffects,
                           nextSkills + i, recipe.data() + i * DISH_PER_CHEF);
    }
    mergeSkills(skills, selfSkills, companySkills, nextSkills, tagOfCompanyBuff,
                chefs.data());

#ifdef MEASURE_TIME
    clock_gettime(CLOCK_MONOTONIC, &end);
    getStatesSkillsTime +=
        (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
#endif
    return;
};