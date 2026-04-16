// csrc/main.cpp
#include <Vtop.h>
#include <verilated.h>
#include <stdio.h>

int main(int argc, char** argv) {
    VerilatedContext* const contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    Vtop* const top = new Vtop{contextp};

    // 仿真逻辑...
    while (!contextp->gotFinish()) {
        int a = rand() & 1;
        int b = rand() & 1;
        top->a = a;
        top->b = b;
        top->eval();
        printf("a = %d, b = %d, f = %d\n", a, b, top->f);
    }

    delete top;
    delete contextp;
    return 0;
}
