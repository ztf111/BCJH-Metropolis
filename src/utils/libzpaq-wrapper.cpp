#include "libzpaq-wrapper.hpp"
#include <iostream>
#include <sstream>

StringStreamReader::StringStreamReader(std::stringstream &ss) : ss(ss) {}
int StringStreamReader::get() { return ss.get(); }

StringStreamWriter::StringStreamWriter(std::stringstream &ss) : ss(ss) {}
void StringStreamWriter::put(int c) { ss.put(c); }

OStreamWriter::OStreamWriter(std::ostream &os) : os(os) {}
void OStreamWriter::put(int c) { os.put(c); }

IStreamReader::IStreamReader(std::istream &is) : is(is) {}
int IStreamReader::get() { return is.get(); }

void libzpaq::error(const char *msg) {
    std::cerr << "Error: " << msg << std::endl;
    throw std::runtime_error(msg);
}

void compress(std::stringstream &in, std::ostream &out) {
    StringStreamReader reader(in);
    OStreamWriter writer(out);

    libzpaq::compress(&reader, &writer, "5");
}

void decompress(std::istream &in, std::stringstream &out) {
    IStreamReader reader(in);
    StringStreamWriter writer(out);
    libzpaq::decompress(&reader, &writer);
}
