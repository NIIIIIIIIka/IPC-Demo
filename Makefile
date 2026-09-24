DEMO_DIRS = pipe-demo fifo-demo uds-demo tcp-demo shm-demo msgqueue-demo

all: $(DEMO_DIRS)

$(DEMO_DIRS):
	$(MAKE) -C $@

clean:
	@for d in $(DEMO_DIRS); do $(MAKE) -C $$d clean; done

.PHONY: all clean $(DEMO_DIRS)
