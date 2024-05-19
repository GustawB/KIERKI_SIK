#ifndef FILE_READER_H
#define FILE_READER_H

#include <iostream>
#include <fstream>
#include <string>
#include <array>

using std::cout;
using std::string;
using std::array;
using std::ifstream;
using std::stoi;
using std::getline;

class FileReader
{
public:
    FileReader() = delete;
    FileReader(const string& file_name);
    ~FileReader();

    ssize_t read_next_deal();

    string get_seat() const;
    int16_t get_trick_type() const;
    array<string, 4> get_cards() const;

private:
    ifstream file_stream;
    bool b_was_stream_opened;

    string file;
    int16_t trick_type;
    string seat;
    array<string, 4> cards;
};

#endif // FILE_READER_H