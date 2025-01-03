#ifndef STATE_RECORDER_HPP
#define STATE_RECORDER_HPP
#include <fstream>
#include <iostream>
#include "States.hpp"
#include <vector>
#include "Chef.hpp"
#include "Recipe.hpp"
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include "include/base64/base64.h"
#include "utils/libzpaq-wrapper.hpp"

struct GlobalAbilityBuff {
    int globalAbilityMale;
    int globalAbilityFemale;
    CookAbility globalAbilityBuff;
    template <class Archive> void serialize(Archive &archive) {
        archive(globalAbilityMale, globalAbilityFemale, globalAbilityBuff);
    }
};

class StatesSerializer {
    GlobalAbilityBuff gab;
    CList *cl;
    RList *rl;

  public:
    StatesSerializer(CList *cl, RList *rl);
    static void serialize(std::ostream &stream, States *state);
    States *deserialize(std::istream &stream);
};

class StatesRecorderBase {
  protected:
    std::vector<States *> states;
    States **states_ptr;
    StatesSerializer serializer;

    void deserialize_states(std::istringstream &inputStream);

  public:
    StatesRecorderBase(CList *cl, RList *rl);
    virtual ~StatesRecorderBase();
    States **get_states(size_t i);
    virtual void add_state(States *state) = 0;
};

class StatesRecorderFile : public StatesRecorderBase {
    std::ofstream file;
    std::size_t id;
    bool id_written = false;
    std::string filename;

    void writeHeader();

  public:
    StatesRecorderFile(std::string filename, std::size_t id, CList *cl,
                       RList *rl);
    ~StatesRecorderFile();
    void add_state(States *state) override;
};

class StatesRecorderString : public StatesRecorderBase {
    std::ostringstream encodedStream;

  public:
    StatesRecorderString(std::string encoded_str, CList *cl, RList *rl);
    void add_state(States *state) override;
    std::string get_encoded_states();
};

#endif