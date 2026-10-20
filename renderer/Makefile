OUT := spirv
SHADERS := $(wildcard shaders/*)
HEADERS := $(patsubst shaders/%,$(OUT)/%.h,$(SHADERS))

all: $(HEADERS)
clean:
	rm -rf $(OUT)

$(OUT)/%.h: shaders/%
	@mkdir -p $(OUT)
	@glslc $< -o - | xxd -n $(notdir $<)_contents -i > $@
	@echo "Compiled: $<"

.PHONY: all clean
