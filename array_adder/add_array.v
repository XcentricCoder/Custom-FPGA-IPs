`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 17.09.2026 17:53:14
// Design Name: 
// Module Name: add_array
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module add_array(
input wire aclk,
input wire aresetn,


input wire [31:0] s_axis_tdata,
output wire s_axis_tready,
input wire s_axis_tvalid,
input wire s_axis_tlast,

output wire [31:0] m_axis_tdata,
input wire m_axis_tready,
output wire m_axis_tvalid,
output wire m_axis_tlast,

output wire error
    );
   
    
  localparam IDLE = 3'b00;
  localparam RECEIVE_A = 3'b01;
  localparam RECEIVE_B = 3'b10;
  localparam CHECK_SIZE = 3'b11;
  localparam SEND_C = 3'b100;
  localparam ERROR = 3'b101;
  
  reg [2:0]states;
  reg [7:0] array_A [1023:0];
  reg [7:0] array_B [1023:0];
  
  reg [10:0] count_A;
  reg [10:0] count_B;
  reg [10:0] count_out;
  
  reg [10:0] size_A;
  reg [10:0] size_B;
  
  reg error_reg;
  
  assign error = error_reg;
  
  assign s_axis_tready = (states == RECEIVE_A) || (states == RECEIVE_B);
  assign m_axis_tdata = (states == SEND_C) ? {24'd0, (array_A[count_out] + array_B[count_out])}: 32'd0;
  assign m_axis_tvalid = (states == SEND_C);
  assign m_axis_tlast =  (states == SEND_C) && (count_out == size_A - 1);
  
  always@(posedge aclk or negedge aresetn) begin
  if(~aresetn) begin
    states <= IDLE;
    count_A <= 11'd0;
    count_B <= 11'd0;
    count_out <= 11'd0;
    size_A <= 11'd0;
    size_B <= 11'd0;
    error_reg <= 1'b0;    
  end
  else begin
    case(states) 
        IDLE: begin
            states <= RECEIVE_A;
            count_A <= 11'd0;
            count_B <= 11'd0;
            count_out <= 11'd0;
            size_A <= 11'd0;
            size_B <= 11'd0;
            error_reg <= 1'b0;
        end
        
        RECEIVE_A: begin
            if (s_axis_tvalid && s_axis_tready) begin
                if(count_A >= 1024) begin
                    error_reg <= 1'b1;
                    states <= ERROR;
                end
                else begin
                    array_A[count_A] <= s_axis_tdata[7:0];
                    if(s_axis_tlast) begin
                        size_A <= count_A + 1;
                        count_A <= 11'b0;
                        count_B <= 11'b0;
                        states <= RECEIVE_B;
                    end else begin
                        count_A <= count_A + 1;
                    end
                end
            end
        end
        
         RECEIVE_B: begin
            if (s_axis_tvalid && s_axis_tready) begin
                if(count_B >= 1024) begin
                    error_reg <= 1'b1;
                    states <= ERROR;
                end
                else begin
                    array_B[count_B] <= s_axis_tdata[7:0];
                    if(s_axis_tlast) begin
                        size_B <= count_B + 1;
                        count_B <= 11'b0;
                        count_out <= 11'b0;
                        states <= CHECK_SIZE;
                    end else begin
                        count_B <= count_B + 1;
                    end
                end
            end
        end
        
        CHECK_SIZE: begin
            if(size_A == size_B ) begin
                states <= SEND_C;
                count_out <= 11'b0;                
            end else begin
                error_reg <= 1'b1;
                states <= ERROR;
            end
        end
        
        SEND_C: begin
            if(m_axis_tvalid && m_axis_tready) begin
                if(count_out == size_A -1) begin
                    count_out <= 11'b0;
                    states <= IDLE;
                end
                else begin
                    count_out <= count_out + 1;
                end
            end
        end
        
        ERROR: begin 
            states <= ERROR;
            error_reg <= 1'b1;
        end
        
        default: begin
            states <= IDLE;
            count_A <= 11'd0;
            count_B <= 11'd0;
            count_out <= 11'd0;
            size_A <= 11'd0;
            size_B <= 11'd0;
            error_reg <= 1'b0;
        end
    endcase
  end
  end
endmodule
