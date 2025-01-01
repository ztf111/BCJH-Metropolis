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
#include "include/base64.h" // Include the provided base64 encoding/decoding library

class StatesSerializer {
    CList *cl;
    RList *rl;

  public:
    StatesSerializer(CList *cl, RList *rl) {
        this->cl = cl;
        this->rl = rl;
    }

    static void serialize(std::ostream &stream, States *state) {
        int chefs[NUM_CHEFS];
        Tool tools[NUM_CHEFS];
        int recipe[DISH_PER_CHEF * NUM_CHEFS];
        for (int i = 0; i < NUM_CHEFS; i++) {
            chefs[i] = state->getChef(i).id;
            tools[i] = state->getTool(i);
        }
        for (int i = 0; i < NUM_CHEFS * DISH_PER_CHEF; i++) {
            recipe[i] = state->recipe[i] == NULL ? -1 : state->recipe[i]->id;
        }
        stream.write((char *)chefs, sizeof(int) * NUM_CHEFS);
        stream.write((char *)tools, sizeof(Tool) * NUM_CHEFS);
        stream.write((char *)recipe, sizeof(int) * NUM_CHEFS * DISH_PER_CHEF);
    }

    States *deserialize(std::istream &stream) {
        States *state = new States();
        int chefs[NUM_CHEFS];
        Tool tools[NUM_CHEFS];
        int recipe[DISH_PER_CHEF * NUM_CHEFS];
        stream.read((char *)chefs, sizeof(int) * NUM_CHEFS);
        stream.read((char *)tools, sizeof(Tool) * NUM_CHEFS);
        stream.read((char *)recipe, sizeof(int) * NUM_CHEFS * DISH_PER_CHEF);
        if (stream.fail()) {
            delete state;
            return NULL;
        }
        for (int i = 0; i < NUM_CHEFS; i++) {
            state->setChef(i, *cl->byId(chefs[i]));
            state->modifyTool(i, tools[i]);
        }
        for (int i = 0; i < NUM_CHEFS * DISH_PER_CHEF; i++) {
            if (recipe[i] != -1) {
                state->recipe[i] = rl->byId(recipe[i]);
            }
        }
        return state;
    }
};

class StatesRecorderBase {
  protected:
    std::vector<States *> states;
    States **states_ptr;
    StatesSerializer serializer;

    void deserialize_states(std::istringstream &inputStream) {
        try {
            while (true) {
                std::string encoded_state;
                std::getline(inputStream, encoded_state);
                if (encoded_state.empty()) {
                    break;
                }
                int decoded_len;
                unsigned char *decoded_state =
                    unbase64(encoded_state.c_str(), encoded_state.length(),
                             &decoded_len);
                std::istringstream iss(std::string(
                    reinterpret_cast<char *>(decoded_state), decoded_len));
                States *state = serializer.deserialize(iss);
                free(decoded_state);
                if (state == NULL) {
                    break;
                }
                states.push_back(state);
            }
        } catch (std::exception &e) {
            std::cerr << "Error while decoding states: " << e.what()
                      << std::endl;
        }
    }

  public:
    StatesRecorderBase(CList *cl, RList *rl) : serializer(cl, rl) {}

    virtual ~StatesRecorderBase() {
        for (auto &state : states) {
            delete state;
        }
        delete[] states_ptr;
    }

    States **get_states(size_t i) {
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

    virtual void add_state(States *state) = 0;
};

class StatesRecorderFile : public StatesRecorderBase {
    std::ofstream file;
    std::size_t id;
    bool id_written = false;
    std::string filename;

    void writeHeader() {
        file = std::ofstream(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: could not create states recorder" << filename
                      << std::endl;
        } else {
            int encoded_len;
            char *encoded_id = base64(&id, sizeof(std::size_t), &encoded_len);
            file << std::string(encoded_id, encoded_len) << std::endl;
            free(encoded_id);
        }
    }

  public:
    StatesRecorderFile(std::string filename, std::size_t id, CList *cl,
                       RList *rl)
        : StatesRecorderBase(cl, rl) {
        std::ifstream inputFile(filename);

        if (inputFile.is_open()) {
            std::string encoded_id;
            std::getline(inputFile, encoded_id);
            int decoded_len;
            unsigned char *decoded_id =
                unbase64(encoded_id.c_str(), encoded_id.length(), &decoded_len);
            std::size_t file_id =
                *reinterpret_cast<const std::size_t *>(decoded_id);
            free(decoded_id);
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

    ~StatesRecorderFile() {
        if (file.is_open()) {
            file.close();
        }
    }

    void add_state(States *state) override {
        if (!id_written) {
            writeHeader();
            id_written = true;
        }
        if (file.is_open()) {
            std::ostringstream oss;
            serializer.serialize(oss, state);
            int encoded_len;
            char *encoded_state =
                base64(oss.str().data(), oss.str().size(), &encoded_len);
            file << std::string(encoded_state, encoded_len) << std::endl;
            free(encoded_state);
        }
    }
};

class StatesRecorderString : public StatesRecorderBase {
    std::ostringstream encodedStream;

  public:
    StatesRecorderString(std::string encoded_str, CList *cl, RList *rl)
        : StatesRecorderBase(cl, rl) {
        std::istringstream inputStream(encoded_str);
        deserialize_states(inputStream);
    }

    void add_state(States *state) override {
        std::ostringstream oss;
        serializer.serialize(oss, state);
        int encoded_len;
        char *encoded_state =
            base64(oss.str().data(), oss.str().size(), &encoded_len);
        encodedStream << std::string(encoded_state, encoded_len) << "\n";
        free(encoded_state);
    }

    std::string get_encoded_states() {
        std::string result = encodedStream.str();
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return result;
    }
};

#endif