# 1. 显式定义 Verilator 路径
VERILATOR = verilator

# 2. 定义编译参数（让 Makefile 更整洁）
VERILATOR_FLAGS = -Wall --cc --exe --build

# 3. 这里的 include 可能会引入路径报错的 git_commit，我们手动定义一个空的以防万一
-include ../Makefile
all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL")
	@echo "开始 Verilator 仿真编译..."
	$(VERILATOR) $(VERILATOR_FLAGS) csrc/main.cpp vsrc/top.v

clean:
	rm -rf obj_dir

.PHONY: sim clean
