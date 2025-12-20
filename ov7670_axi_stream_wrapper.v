`timescale 1ns / 1ps
module ov7670_axi_stream_wrapper #
(
    parameter DATA_WIDTH = 16
)
(
    // Camera interface
    input  wire        pclk,
    input  wire        rst,
    input  wire        enable_capture,
    input  wire        vsync,
    input  wire        href,
    input  wire [7:0]  d,

    // AXI Stream output (to AXI DMA S2MM)
    output wire [DATA_WIDTH-1:0] m_axis_tdata,
    output wire                  m_axis_tvalid,
    input  wire                  m_axis_tready,
    output wire                  m_axis_tuser,  // SOF
    output wire                  m_axis_tlast   // EOL
);

    wire [15:0] pixel_data;
    wire pixel_valid, sof, eol;

    ov7670_capture core (
        .pclk(pclk),
        .rst(rst),
        .enable_capture(enable_capture),

        .vsync(vsync),
        .href(href),
        .d(d),

        .pixel_data(pixel_data),
        .pixel_valid(pixel_valid),
        .sof(sof),
        .eol(eol)
    );

    //-----------------------------------------------------
    // AXI-Stream handshake: DMA almost always ready = OK
    //-----------------------------------------------------
    assign m_axis_tdata  = pixel_data;
    assign m_axis_tvalid = pixel_valid;
    assign m_axis_tuser  = sof;     // marks start of frame
    assign m_axis_tlast  = eol;     // marks end of line

endmodule
