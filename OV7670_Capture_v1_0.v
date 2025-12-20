`timescale 1 ns / 1 ps

module OV7670_Capture_v1_0 #
(
    parameter integer C_M00_AXIS_TDATA_WIDTH = 16
)
(
    // Camera signals
    input  wire pclk,
    input  wire vsync,
    input  wire href,
    input  wire [7:0] d,

    // AXI Stream interface
    input  wire  m00_axis_aclk,
    input  wire  m00_axis_aresetn,
    output wire  m00_axis_tvalid,
    output wire [C_M00_AXIS_TDATA_WIDTH-1:0] m00_axis_tdata,
    output wire  m00_axis_tlast,
    input  wire  m00_axis_tready
);

    wire [15:0] pixel_data;
    wire pixel_valid;
    wire frame_done;

    //------------------------------------------------------------
    // OV7670 data capture core
    //------------------------------------------------------------
    ov7670_capture_core u_core (
        .pclk(pclk),
        .vsync(vsync),
        .href(href),
        .d(d),
        .pixel_data(pixel_data),
        .pixel_valid(pixel_valid),
        .frame_done(frame_done)
    );

    //------------------------------------------------------------
    // AXIS master
    //------------------------------------------------------------
    OV7670_Capture_v1_0_M00_AXIS #(
        .C_M_AXIS_TDATA_WIDTH(C_M00_AXIS_TDATA_WIDTH)
    )
    axis_master (
        .M_AXIS_ACLK(m00_axis_aclk),
        .M_AXIS_ARESETN(m00_axis_aresetn),

        .pixel_data(pixel_data),
        .pixel_valid(pixel_valid),
        .frame_done(frame_done),

        .M_AXIS_TVALID(m00_axis_tvalid),
        .M_AXIS_TDATA (m00_axis_tdata),
        .M_AXIS_TLAST (m00_axis_tlast),
        .M_AXIS_TREADY(m00_axis_tready)
    );

endmodule
