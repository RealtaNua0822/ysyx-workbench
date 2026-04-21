module top(
  input clk,
  input rst,
  output reg [15:0] ledr
);
  reg [31:0] count; // 增加一个 32 位计数器
  reg [3:0]  led_idx;

  always @(posedge clk) begin
    if (rst) begin
      count <= 0;
      led_idx <= 0;
      ledr <= 16'h1;
    end else begin
      if (count == 5000000) begin // 调这个数字，越大灯闪得越慢
        count <= 0;
        led_idx <= led_idx + 1;
        ledr <= (16'h1 << led_idx); 
      end else begin
        count <= count + 1;
      end
    end
  end
endmodule