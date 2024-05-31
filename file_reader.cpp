#include "file_reader.h"

FileReader::FileReader(const string& file_name)
    : b_was_stream_opened(false), file(file_name), trick_type(0) {}

FileReader::~FileReader() {}

string FileReader::get_seat() const { return seat; }

int16_t FileReader::get_trick_type() const { return trick_type; }

array<string, 4> FileReader::get_cards() const { return cards; }

ssize_t FileReader::read_next_deal()
{
    if (!b_was_stream_opened)
    {
        file_stream.open(file);
        if (!file_stream.is_open())
        {
            return -1;
        }
        b_was_stream_opened = true;
    }

    string line;
    getline(file_stream, line);
    if (line.empty()) 
    { // End of file;
        file_stream.close();
        return 0; 
    }
    trick_type = stoi(line.substr(0, 1));
    seat = line.substr(1, 1);
    for (int i = 0; i < 4; ++i)
    {
        getline(file_stream, line);
        cards[i] = line;
    }

    return 1;
}