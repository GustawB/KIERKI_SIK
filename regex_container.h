#include <boost/regex.hpp>
#include <iostream>

namespace rgx
{
    using std::string;
    class RegexContainer
    {
    private:
        bool has_unique_chars(const strong& s);
    public:
        // User shouldn't be able to create an object of this class.
        RegexContainer() = delete;
        ~RegexContainer() = delete;

        static bool IAM_check(const std::string& s);
        static bool BUSY_check(const std::string& s);
        static  bool DEAL_check(const std::string& s);
    };

    inline bool RegexContainer::has_unique_chars(const string& s)
    {
        for (size_t i = 0; i < s.size(); ++i)
        {
            for (size_t j = i + 1; j < s.size(); ++j)
            {
                if (s[i] == s[j])
                {
                    return false;
                }
            }
        }

        return true;
    }

    inline static bool RegexContainer::IAM_check(const std::string& s)
    {
        boost::regex IAM_regex("IAM[NESW]\\r\\n");
        return boost::regex_match(s, IAM_regex);
    }

    inline static bool BUSY_check(const std::string& s)
    {
        boost::regex BUSY_regex("BUSY([NESW]{0,4})\\r\\n");

        if (!boost::regex_match(s, BUSY_regex))
        {
            return false;
        }
        else
        {
            string seats = s.substr(4, s.find('\r') - 1);
            return has_unique_chars(seats);
        }
    }

    inline static bool DEAL_check(const std::string& s)
    {
        boost::regex DEAL_regex("DEAL[1-7][NESW]([][NESW]{0,4})\\r\\n");
    }
} // namespace rgx