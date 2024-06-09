#ifndef FILE_READER_H
#define FILE_READER_H

#include <iostream>
#include <fstream>
#include <string>
#include <array>

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

    /*
    * Read the next deal from the file.
    * Return 1 if a deal was read successfully.
    * Return 0 if the end of the file was reached.
    * Return -1 if the file could not be opened.
    */
    ssize_t read_next_deal();

    /*
    * Get the seat of the player starting the last read deal.
    */
    string get_seat() const;

    /*
    * Get the trick type of the last read deal.
    */
    int16_t get_trick_type() const;

    /*
    * Get the starting hand fo every player in the last read deal.
    */
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