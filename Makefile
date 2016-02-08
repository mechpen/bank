SUBDIRS = src

.PHONY: all clean test $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) --directory=$@ $(TOPTARGET)

clean:
	$(MAKE) TOPTARGET=clean

test: all
	cd test && python test_all.py
