#include <iostream>
#include <fstream>
#include <string>
#include <array>


namespace file_reader
{
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

    inline FileReader::FileReader(const string& file_name)
        : b_was_stream_opened(false), file(file_name), trick_type(0) {}

    inline FileReader::~FileReader() {}

    inline string FileReader::get_seat() const { return seat; }

    inline int16_t FileReader::get_trick_type() const { return trick_type; }

    inline array<string, 4> FileReader::get_cards() const { return cards; }

    inline ssize_t FileReader::read_next_deal()
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
        if (line.empty()) { return 0; } // End of file I guess.;
        trick_type = stoi(line.substr(0, 1));
        seat = line.substr(1, 1);
        for (int i = 0; i < 4; ++i)
        {
            getline(file_stream, line);
            cards[i] = line;
        }

        return 1;
    }  
} // namespace file_reader