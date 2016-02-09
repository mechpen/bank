SUBDIRS = src

.PHONY: all clean tests $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) --directory=$@ $(TOPTARGET)

clean:
	$(MAKE) TOPTARGET=clean

test: all
	cd tests && python3 test_all.py && rm -rf db && rm -rf logs
