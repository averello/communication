CC = gcc
#override CFLAGS += -Wall -g3 -pedantic -std=c99 -I${INC} -I$(OPENSSL)/include -D_XOPEN_SOURCE=700 -DDEBUG=1
override CFLAGS += -Wall -g3 -pedantic -std=c99 -I${INC} -I$(OPENSSL)/include -D_XOPEN_SOURCE=700
SHAREDFLAGS=-fPIC -shared
BASE_LDFLAGS = -lc -lpthread -lcrypto -L$(OPENSSL)/lib -lcrypto
LDFLAGS = $(BASE_LDFLAGS) -Llib -l${COMMUNICATION}
WLFLAGS=-Wl,-rpath,$(LIB)/lib$(COMMUNICATION).so.$(COMMUNICATIONMAJORVERSION)
BIN = bin
INC = $(SRC)
OBJ = obj
SRC = src
MKDIR = mkdir
OPENSSL=/Vrac/openssl

LIB = lib
TEST = test
MKDIR = mkdir

COMMUNICATION = communication

LD_LIBRARY_PATH := $(shell echo $$LD_LIBRARY_PATH):$(LIB)

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
STRIP = strip --strip-unneeded
#LD_LIBRARY_PATH := $(LD_LIBRARY_PATH):$(OPENSSL)/lib
endif

ifeq ($(UNAME), Darwin)
STRIP = strip -X -x -S
endif

DOC = doc

.PHONY: all directories compileall runall clean cleanall


#.SUFFIXES:            # Delete the default suffixes
#.SUFFIXES: .c .o .h           # Delete the default suffixes

# +------------+
# | Target all |
# +------------+

all: directories compileall

# +------------+
# | Target lib |
# +------------+

COMMUNICATIONSTATIC = ${LIB}/lib${COMMUNICATION}.a
libcommunication : directories $(COMMUNICATIONSTATIC)

COMMUNICATIONMAJORVERSION=1
COMMUNICATIONMINORVERSION=0
COMMUNICATIONRELEASENUMBER=1
COMMUNICATIONSONAME = ${LIB}/lib${COMMUNICATION}.so.$(COMMUNICATIONMAJORVERSION)
COMMUNICATIONREALNAME = ${LIB}/lib${COMMUNICATION}.so.$(COMMUNICATIONMAJORVERSION).$(COMMUNICATIONMINORVERSION).$(COMMUNICATIONRELEASENUMBER)
COMMUNICATIONSHARED = ${LIB}/lib${COMMUNICATION}.so
FIRSTLINK = $(patsubst $(LIB)/%,%,$(COMMUNICATIONREALNAME))
SECONDLINK = $(patsubst $(LIB)/%,%,$(COMMUNICATIONSONAME))
libcommunicationshared : directories $(COMMUNICATIONSHARED)
	@ln -s $(FIRSTLINK) $(COMMUNICATIONSONAME) 
	@ln -s $(SECONDLINK) $(COMMUNICATIONSHARED) 

TESTS = $(patsubst $(TEST)/%.c,$(BIN)/%,$(wildcard $(TEST)/*.c))
tests : directories libcommunication $(TESTS)
	@for test in ${TESTS}; do \
		echo "**** Testing $$test"; \
	echo "---- end of ${TESTSTRING}"; \
	done
	@#	./"$$test" \

test% : $(BIN)/test%
	@echo "**** Testing $@";
	@$<
	@echo "end of $@";


valgrind% : $(BIN)/test%
	@valgrind  --track-origins=yes --leak-check=full --show-reachable=yes $<

doc : $(DOC)/html/index.html $(SRC)/$(COMMUNICATION).h

$(DOC)/html/index.html : 
	@cd $(DOC); /usr/bin/env doxygen


# +--------------------+
# | Target directories |
# +--------------------+

directories: ${OBJ} ${BIN} ${LIB} 

${OBJ}:
	${MKDIR} -p $@
${BIN}:
	${MKDIR} -p $@
${LIB}:
	${MKDIR} -p $@

# +----------------------------+
# | Target compilation with -c |
# +----------------------------+

${OBJ}/%.o : ${SRC}/%.c
	$(CC) -c -o $@ $< ${CFLAGS} $(SHAREDFLAGS)

${OBJ}/%.o : ${TEST}/%.c
	$(CC) -c -o $@ $< ${CFLAGS}

${BIN}/% : ${OBJ}/%.o
	${CC} -o $@ $< ${LDFLAGS}


${LIB}/lib${COMMUNICATION}.a : $(patsubst ${SRC}/%.c,${OBJ}/%.o,$(wildcard ${SRC}/*.c))
	${AR} r ${LIB}/lib${COMMUNICATION}.a $?

${LIB}/lib${COMMUNICATION}.so : $(COMMUNICATIONSONAME)

${LIB}/lib${COMMUNICATION}.so.$(COMMUNICATIONMAJORVERSION) : $(COMMUNICATIONREALNAME)

${LIB}/lib${COMMUNICATION}.so.$(COMMUNICATIONMAJORVERSION).$(COMMUNICATIONMINORVERSION).$(COMMUNICATIONRELEASENUMBER) : $(patsubst ${SRC}/%.c,${OBJ}/%.o,$(wildcard ${SRC}/*.c))
	$(CC) $(CFLAGS) $(SHAREDFLAGS) $(WLFLAGS) -o $@  $? $(BASE_LDFLAGS)
	$(STRIP) $@


#${BIN}/${TESTSTRING} : libcobj $(OBJ)/${TESTSTRING}.o 
#	${CC} -o $@ $< ${LDFLAGS}



# +-------------------+
# | Target compileall |
# +-------------------+

compileall: libcommunication

# +---------------+
# | Target runall |
# +---------------+

runall: compileall
	$(MAKE) tests

# +--------------+
# | Target clean |
# +--------------+

clean:
	-rm -f ${OBJ}/* ${BIN}/* $(LIB)/* $(DOC)/html/search/* $(DOC)/latex/* $(DOC)/html/*

# +-----------------+
# | Target cleanall |
# +-----------------+

cleanall: clean
	-rmdir ${OBJ} ${BIN} $(LIB) $(DOC)/latex $(DOC)/html/search $(DOC)/html 2>/dev/null || exit 0
	-rm -f ${INC}/*~ ${SRC}/*~ *~ ${TEST}/*~

