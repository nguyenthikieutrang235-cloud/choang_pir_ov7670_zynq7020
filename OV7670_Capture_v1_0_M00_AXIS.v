`timescale 1 ns / 1 ps

module OV7670_Capture_v1_0_M00_AXIS #
(
    parameter integer C_M_AXIS_TDATA_WIDTH = 16
)
(
    input wire  M_AXIS_ACLK,
    input wire  M_AXIS_ARESETN,

    // Data from OV7670 core
    input wire [15:0] pixel_data,
    input wire pixel_valid,
    input wire frame_done,

    // AXIS output
    output reg  M_AXIS_TVALID,
    output reg [C_M_AXIS_TDATA_WIDTH-1 : 0] M_AXIS_TDATA,
    output reg  M_AXIS_TLAST,
    input wire  M_AXIS_TREADY
);

    always @(posedge M_AXIS_ACLK) begin
        if (!M_AXIS_ARESETN) begin
            M_AXIS_TVALID <= 0;
            M_AXIS_TDATA  <= 0;
            M_AXIS_TLAST  <= 0;
        end else begin
            if (pixel_valid) begin
                M_AXIS_TVALID <= 1;
                M_AXIS_TDATA  <= pixel_data;   // *** 16-bit output ***
                M_AXIS_TLAST  <= frame_done;
            end else if (M_AXIS_TREADY) begin
                M_AXIS_TVALID <= 0;
                M_AXIS_TLAST  <= 0;
            end
        end
    end

endmodule
