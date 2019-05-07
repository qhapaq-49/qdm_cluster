
CC = g++
FLAG = -Ofast -MMD -std=c++14 -fopenmp -Wformat-security
#FLAG = -pg -MMD -std=c++14 -fopenmp
TARGET = qdm_cluster.exe

SOURCES = main.cpp usi.cpp agent.cpp process.cpp misc.cpp pvparse.cpp logger.cpp
OBJDIR = ../../obj
OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))
DEPENDS  = $(OBJECTS:.o=.d)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(FLAG)

$(OBJDIR)/%.o: %.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) $(FLAG) -o $@ -c $<

all:
	$(MAKE) FLAG='$(FLAG)' $(TARGET)

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET)

-include $(DEPENDS)

