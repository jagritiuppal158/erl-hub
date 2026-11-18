CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRC      = main.cpp Inventory.cpp Logistics.cpp Persistence.cpp
OBJ      = $(SRC:.cpp=.o)

erlhub: $(OBJ)
	$(CXX) $(CXXFLAGS) -o erlhub $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: erlhub
	./erlhub

clean:
	rm -f $(OBJ) erlhub erlhub_data.bin erlhub_data.bin.tmp
