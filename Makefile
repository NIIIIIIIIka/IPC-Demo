DEMO_DIRS = pipe-demo fifo-demo uds-demo tcp-demo shm-demo msgqueue-demo
OPTIONAL_DEMO_DIRS = aeron-demo iceoryx2-demo

all: $(DEMO_DIRS)

$(DEMO_DIRS) $(OPTIONAL_DEMO_DIRS):
	$(MAKE) -C $@

clean:
	@for d in $(DEMO_DIRS); do $(MAKE) -C $$d clean; done
	@for d in $(OPTIONAL_DEMO_DIRS); do $(MAKE) -C $$d clean; done

optional: $(OPTIONAL_DEMO_DIRS)

.PHONY: all optional clean $(DEMO_DIRS) $(OPTIONAL_DEMO_DIRS)
