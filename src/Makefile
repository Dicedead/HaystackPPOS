## ======================================================================
## partial Makefile provided to students
##

LIBMONGOOSEDIR = libmongoose/
## don't forget to export LD_LIBRARY_PATH pointing to it
LIBMONGOOSE_CFLAGS += -I$(LIBMONGOOSEDIR)
$(LIBMONGOOSEDIR)/libmongoose.so: $(LIBMONGOOSEDIR)/mongoose.c  $(LIBMONGOOSEDIR)/mongoose.h
	make -C $(LIBMONGOOSEDIR)

VIPS_CFLAGS += $$(pkg-config vips --cflags)
VIPS_LIBS   += $$(pkg-config vips --libs)

LDLIBS += -ljson-c

.PHONY: clean new newlibs style \
feedback feedback-VM-CO clone-ssh clean-fake-ssh \
submit1 submit2 submit

CFLAGS += -std=c11 -Wall -pedantic -g

# a bit more checks if you'd like to (uncomment)
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# ----------------------------------------------------------------------
# feel free to update/modify this part as you wish

TARGETS += imgStoreMgr
TARGETS += imgStore_server
all: $(TARGETS)

LDLIBS += -lm

CHECK_TARGETS += tests/test-imgStore-implementation tests/unit-test-cmd_args tests/unit-test-dedup
OBJS  +=
RUBS = $(OBJS) core



imgStoreMgr: imgStoreMgr.o tools.o error.o imgst_list.o imgst_create.o imgst_delete.o image_content.o util.o imgst_read.o imgst_insert.o  dedup.o imgst_gbcollect.o
imgStoreMgr: LDLIBS += $(VIPS_LIBS) -lssl -lcrypto
imgStoreMgr: CFLAGS += $(VIPS_CFLAGS)


imgStore_server : LDLIBS += $(VIPS_LIBS)  -L$(LIBMONGOOSEDIR) -lmongoose
imgStore_server : CFLAGS += $(VIPS_CFLAGS) $(LIBMONGOOSE_CFLAGS)
imgStore_server: imgStore_server.o imgst_list.o tools.o error.o imgst_list.o image_content.o util.o imgst_read.o

imgStore_server.o: imgStore_server.c util.h imgStore.h error.h
dedup.o: dedup.c dedup.h imgStore.h error.h
error.o: error.c
image_content.o: image_content.c image_content.h imgStore.h error.h
imgst_create.o: imgst_create.c imgStore.h error.h
imgst_delete.o: imgst_delete.c imgStore.h error.h
imgst_insert.o: imgst_insert.c imgStore.h error.h dedup.h image_content.h
imgst_list.o: imgst_list.c imgStore.h error.h
imgst_gbcollect.o : imgst_gbcollect.c imgStore.h error.h
imgStoreMgr.o: imgStoreMgr.c util.h imgStore.h error.h
imgst_read.o: imgst_read.c imgStore.h error.h image_content.h
tools.o: tools.c imgStore.h error.h
util.o: util.c

# ----------------------------------------------------------------------
# This part is to make your life easier. See handouts how to make use of it.

## ======================================================================
## Tests

# target to run black-box tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
	    for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file  || exit 1; done; \
	fi



VIPS_LIBS   += $(pkg-config vips --libs) -lm
VIPS_FLAGS  += $(pkg-config vips --cflags)

# all those libs are required on Debian, adapt to your box
$(CHECK_TARGETS): LDLIBS += $(VIPS_LIBS) -lcheck -lm -lrt -pthread -lsubunit -lvips -lgobject-2.0 -lglib-2.0

tests/unit-test-cmd_args.o:
tests/unit-test-cmd_args: tests/unit-test-cmd_args.o tools.o error.o imgst_list.o imgst_create.o imgst_delete.o imgst_read.o image_content.o dedup.o $(OBJS)

tests/unit-test-dedup.o:
tests/unit-test-dedup: tests/unit-test-dedup.o tools.o error.o imgst_list.o imgst_create.o dedup.o $(OBJS)

check:: CFLAGS += $(VIPS_FLAGS) -I.
check:: $(CHECK_TARGETS)
	export LD_LIBRARY_PATH=.; $(foreach target,$(CHECK_TARGETS),./$(target) &&) true

clean::
	-@/bin/rm -f *.o *~ $(CHECK_TARGETS)

new: clean all

static-check:
	CCC_CC=$(CC) scan-build -analyze-headers --status-bugs -maxloop 64 make -j1 new

style:
	astyle -n -o -A8 -xt0 *.[ch]

## ======================================================================
## Feedback

## chappeli/pps21-feedback:weekXX
IMAGE=chappeli/pps21-feedback:week10
## Note: vous pouvez changer le tag latest pour week04, ou week05, etc.

REPO := $(shell git config --get remote.origin.url)
SSH_DIR := $(HOME)/.ssh

feedback:
	@echo Will use $(REPO) inside container
	@docker pull $(IMAGE)
	@docker run -it --rm -e REPO=$(REPO) -v $(SSH_DIR):/opt/.ssh $(IMAGE)

clone-ssh:
	@-$(eval SSH_DIR := $(HOME)/.$(shell date "+%s;$$"|sha256sum|cut -c-32))
	@cp -r $(HOME)/.ssh/. $(SSH_DIR)

clean-fake-ssh:
	@case $(SSH_DIR) in $(HOME)/\.????????????????????????????????) $(RM) -fr $(SSH_DIR) ;; *) echo "Dare not remove \"$(SSH_DIR)\"" ;; esac

feedback-VM-CO: clone-ssh feedback clean-fake-ssh

## ======================================================================
## Submit

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf "what 'make submit'??\nIt'\''s either 'make submit1' or 'make submit2'...\n"