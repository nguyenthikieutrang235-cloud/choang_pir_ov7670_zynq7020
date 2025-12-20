`timescale 1ns / 1ps
module ov7670_capture_top #
(
    parameter DATA_WIDTH = 16
)
(
    // To be connected in BD
    input  wire        pclk,
    input  wire        rst,
    input  wire        enable_capture,
    input  wire        vsync,
    input  wire        href,
    input  wire [7:0]  d,

    // AXI Stream Outputs
    output wire [DATA_WIDTH-1:0] m_axis_tdata,
    output wire                  m_axis_tvalid,
    input  wire                  m_axis_tready,
    output wire                  m_axis_tuser,
    output wire                  m_axis_tlast
);

    ov7670_axi_stream_wrapper stream_wrap (
        .pclk(pclk),
        .rst(rst),
        .enable_capture(enable_capture),
        .vsync(vsync),
        .href(href),
        .d(d),

        .m_axis_tdata(m_axis_tdata),
        .m_axis_tvalid(m_axis_tvalid),
        .m_axis_tready(m_axis_tready),
        .m_axis_tuser(m_axis_tuser),
        .m_axis_tlast(m_axis_tlast)
    );

endmodule
