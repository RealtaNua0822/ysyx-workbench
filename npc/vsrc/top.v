module top(
  input a,
  input b,
  input clk,
  input rst,
  output f
);
  assign f = a ^ b;
endmodule
