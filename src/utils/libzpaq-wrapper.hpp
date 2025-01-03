#ifndef LIBZPAQ_WRAPPER_HPP
#define LIBZPAQ_WRAPPER_HPP
#include "../include/libzpaq/libzpaq.h"
#include <iostream>
#include <sstream>

class StringStreamReader : public libzpaq::Reader {
  public:
    StringStreamReader(std::stringstream &ss);
    int get() override;

  private:
    std::stringstream &ss;
};

class StringStreamWriter : public libzpaq::Writer {
  public:
    StringStreamWriter(std::stringstream &ss);
    void put(int c) override;

  private:
    std::stringstream &ss;
};

class OStreamWriter : public libzpaq::Writer {
  public:
    OStreamWriter(std::ostream &os);
    void put(int c) override;

  private:
    std::ostream &os;
};

class IStreamReader : public libzpaq::Reader {
  public:
    IStreamReader(std::istream &is);
    int get() override;

  private:
    std::istream &is;
};

namespace libzpaq {
void error(const char *msg);
}

void compress(std::stringstream &in, std::ostream &out);
void decompress(std::istream &in, std::stringstream &out);

#endif