#include <iostream>
#include <thread>

namespace serwer 
{
    using std::thread;
    using std::cout;
    class Serwer
    {
    private:
        thread connection_thread;
    public:
        Serwer();
        ~Serwer();
    };

    inline Serwer::Serwer()
    {
        cout << "Creating thread\n";
        connection_thread = thread([]() { cout << "Hello from serwer\n"; });
    }

    inline Serwer::~Serwer()
    {
        connection_thread.join();
        cout << "Joined thread\n";
    }
} // namespace serwer