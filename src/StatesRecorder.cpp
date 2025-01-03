#include "StatesRecorder.hpp"
#include "include/cereal/archives/portable_binary.hpp"
#include "include/cereal/types/array.hpp"
#include "include/cereal/types/vector.hpp"
#include "include/cereal/types/memory.hpp"
#include "include/cereal/types/tuple.hpp"
#include "include/libzpaq/libzpaq.h"
void libzpaq::error(const char *msg) {
    std::cerr << "Error: " << msg << std::endl;
    exit(1);
}

void StatesSerializer::serialize(std::ostream &stream, States *state) {
    Chef chefs[NUM_CHEFS];
    int recipe[DISH_PER_CHEF * NUM_CHEFS];
    for (int i = 0; i < NUM_CHEFS; i++) {
        chefs[i] = state->getChef(i);
    }
    for (int i = 0; i < NUM_CHEFS * DISH_PER_CHEF; i++) {
        recipe[i] = state->recipe[i] == NULL ? -1 : state->recipe[i]->id;
    }
    GlobalAbilityBuff gab{Chef::globalAbilityMale, Chef::globalAbilityFemale,
                          Chef::globalAbilityBuff};
    cereal::PortableBinaryOutputArchive archive(stream);
    archive(chefs, recipe, gab);
}

States *StatesSerializer::deserialize(std::istream &stream) {
    States *state = new States();
    Chef chefs[NUM_CHEFS];
    int recipe[DISH_PER_CHEF * NUM_CHEFS];
    cereal::PortableBinaryInputArchive archive(stream);
    archive(chefs, recipe, this->gab);

    for (int i = 0; i < NUM_CHEFS; i++) {
        state->setChef(i, chefs[i]);
    }
    for (int i = 0; i < NUM_CHEFS * DISH_PER_CHEF; i++) {
        if (recipe[i] != -1) {
            state->recipe[i] = rl->byId(recipe[i]);
        }
    }
    return state;
}

StatesSerializer::StatesSerializer(CList *cl, RList *rl) {
    this->cl = cl;
    this->rl = rl;
}

StatesRecorderBase::StatesRecorderBase(CList *cl, RList *rl)
    : serializer(cl, rl) {}

StatesRecorderBase::~StatesRecorderBase() {
    for (auto &state : states) {
        delete state;
    }
    delete[] states_ptr;
}

void StatesRecorderBase::deserialize_states(std::istringstream &inputStream) {
    try {
        while (true) {
            std::string encoded_state;
            std::getline(inputStream, encoded_state);
            if (encoded_state.empty()) {
                break;
            }
            std::string decoded_state = base64_decode(encoded_state);
            std::istringstream iss(decoded_state);
            States *state = serializer.deserialize(iss);
            if (state == NULL) {
                break;
            }
            states.push_back(state);
        }
    } catch (std::exception &e) {
        std::cerr << "Error while decoding states: " << e.what() << std::endl;
    }
}

States **StatesRecorderBase::get_states(size_t i) {
    states_ptr = new States *[i];
    if (i > states.size()) {
        for (size_t j = 0; j < states.size(); j++) {
            states_ptr[j] = states[j];
        }
        for (size_t j = states.size(); j < i; j++) {
            states_ptr[j] = NULL;
        }
    } else {
        for (size_t j = 0; j < i; j++) {
            states_ptr[j] = states[j];
        }
    }
    return states_ptr;
}

StatesRecorderFile::StatesRecorderFile(std::string filename, std::size_t id,
                                       CList *cl, RList *rl)
    : StatesRecorderBase(cl, rl) {
    std::ifstream inputFile(filename);

    if (inputFile.is_open()) {
        std::string encoded_id;
        std::getline(inputFile, encoded_id);
        std::string decoded_id = base64_decode(encoded_id);
        std::size_t file_id =
            *reinterpret_cast<const std::size_t *>(decoded_id.data());
        if (file_id == id) {
            std::istringstream inputStream(
                std::string((std::istreambuf_iterator<char>(inputFile)),
                            std::istreambuf_iterator<char>()));
            deserialize_states(inputStream);
            std::cout << "从存档点" << id << "恢复" << states.size()
                      << "条记录。若要重新开始，请删除同一目录下states."
                         "txt文件。"
                      << std::endl;
        }
    }
    inputFile.close();
    this->id = id;
    this->filename = filename;
}

StatesRecorderFile::~StatesRecorderFile() {
    if (file.is_open()) {
        file.close();
    }
}

void StatesRecorderFile::writeHeader() {
    file = std::ofstream(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: could not create states recorder" << filename
                  << std::endl;
    } else {
        std::string encoded_id = base64_encode(
            reinterpret_cast<const unsigned char *>(&id), sizeof(std::size_t));
        file << encoded_id << std::endl;
    }
}

void StatesRecorderFile::add_state(States *state) {
    if (!id_written) {
        writeHeader();
        id_written = true;
    }
    if (file.is_open()) {
        std::ostringstream oss;
        serializer.serialize(oss, state);
        std::string encoded_state = base64_encode(
            reinterpret_cast<const unsigned char *>(oss.str().data()),
            oss.str().size());
        file << encoded_state << std::endl;
    }
}

StatesRecorderString::StatesRecorderString(std::string encoded_str, CList *cl,
                                           RList *rl)
    : StatesRecorderBase(cl, rl) {
    std::istringstream inputStream(encoded_str);
    deserialize_states(inputStream);
}

void StatesRecorderString::add_state(States *state) {
    std::ostringstream oss;
    serializer.serialize(oss, state);
    std::string encoded_state =
        base64_encode(reinterpret_cast<const unsigned char *>(oss.str().data()),
                      oss.str().size());
    encodedStream << encoded_state << "\n";
}

std::string StatesRecorderString::get_encoded_states() {
    std::string result = encodedStream.str();
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}
