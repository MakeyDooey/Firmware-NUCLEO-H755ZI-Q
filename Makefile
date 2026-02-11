# --- Master Build Orchestrator ---

CORE_M7 = CM7
CORE_M4 = CM4

.PHONY: all clean $(CORE_M7) $(CORE_M4)

# The 'all' target now forces a sequential execution
all:
	@echo "--- Starting CM7 Build Phase ---"
	$(MAKE) $(CORE_M7)
	@echo "--- Starting CM4 Build Phase ---"
	$(MAKE) $(CORE_M4)

$(CORE_M7):
	$(MAKE) -C $(CORE_M7) -j$(nproc)

$(CORE_M4):
	$(MAKE) -C $(CORE_M4) -j$(nproc)

clean:
	@echo "--- Cleaning Build Artifacts ---"
	rm -rf $(CORE_M7)/build $(CORE_M4)/build
