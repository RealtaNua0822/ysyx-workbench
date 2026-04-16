#include <Vtop.h>
#include <verilated.h>
#include <stdio.h>
#include "verilated_vcd_c.h" // 必须包含此头文件

int main(int argc, char** argv) {
    VerilatedContext* const contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    
    // 1. 开启追踪功能
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;

    Vtop* const top = new Vtop{contextp};

    // 2. 关联追踪对象与顶级模块
    top->trace(tfp, 99); // 追踪深度
    tfp->open("wave.vcd"); // 设置保存的文件名

    // 3. 建议增加时间限制，防止生成无限大的波形文件
    while (!contextp->gotFinish() && contextp->time() < 100) {
        int a = rand() & 1;
        int b = rand() & 1;
        top->a = a;
        top->b = b;
        
        top->eval();
        
        // 4. 将当前时刻的所有信号状态写入 VCD 文件
        tfp->dump(contextp->time()); 
        
        printf("a = %d, b = %d, f = %d\n", a, b, top->f);
        
        // 5. 推进仿真时间戳（否则波形图上所有变化都挤在 0 时刻）
        contextp->timeInc(1); 
    }

    // 6. 必须关闭文件，否则 VCD 文件尾部不完整，GTKWave 打不开
    tfp->close(); 
    
    delete top;
    delete contextp;
    return 0;
}
