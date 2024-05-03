CC = g++
BOOST_ROOT = /home/gustaw/boost_library/boost_1_74_0
CFLAGS = -Wall -Wextra -O2 -std=c++20 -I$(BOOST_ROOT)
LFLAGS = -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a

.PHONY: all clean

TARGET1 = kierki-serwer
TARGET2 = kierki-klient

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TARGET2): $(TARGET2).o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET1) $(TARGET2) *.o *~
