BUILD_DIR := build

.PHONY: all build install clean

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
	@cmake --build $(BUILD_DIR) -j$$(nproc)
	@echo "Compilation terminée : $(BUILD_DIR)/nemesis"

install: build
	@sudo cmake --install $(BUILD_DIR)
	@echo "Installé dans /usr/local/bin/nemesis"

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Build nettoyé"

rebuild: clean build

reinstall: clean install
