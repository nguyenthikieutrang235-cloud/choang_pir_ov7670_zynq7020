`timescale 1ns / 1ps
module ov7670_capture (
    input  wire        pclk,
    input  wire        rst,
    input  wire        enable_capture,
    
    input  wire        vsync,
    input  wire        href,
    input  wire [7:0]  d,

    output reg  [15:0] pixel_data,
    output reg         pixel_valid,
    output reg         sof,        // start of frame
    output reg         eol         // end of line
);

    reg [7:0] byte_high;
    reg       byte_phase;

    always @(posedge pclk or posedge rst) begin
        if (rst) begin
            byte_phase  <= 0;
            pixel_valid <= 0;
            sof         <= 0;
            eol         <= 0;
        end 
        else if (!enable_capture) begin
            byte_phase  <= 0;
            pixel_valid <= 0;
            sof         <= 0;
            eol         <= 0;
        end
        else begin
            pixel_valid <= 0;
            sof         <= 0;
            eol         <= 0;

            if (vsync) begin
                byte_phase <= 0;
                sof <= 1;     // Start of frame
            end
            else if (href) begin
                if (byte_phase == 0) begin
                    byte_high <= d;
                    byte_phase <= 1;
                end 
                else begin
                    pixel_data <= {byte_high, d};
                    pixel_valid <= 1;
                    byte_phase <= 0;
                end
            end 
            else begin
                eol <= 1;      // End of line
                byte_phase <= 0;
            end
        end
    end

endmodule
