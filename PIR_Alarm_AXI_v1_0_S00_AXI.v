`timescale 1 ns / 1 ps
module PIR_Alarm_AXI_v1_0_S00_AXI #
(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 4
)
(
    input wire S_AXI_ACLK,
    input wire S_AXI_ARESETN,

    // Write address channel
    input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_AWADDR,
    input wire S_AXI_AWVALID,
    output reg  S_AXI_AWREADY,

    // Write data channel
    input wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_WDATA,
    input wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0] S_AXI_WSTRB,
    input wire S_AXI_WVALID,
    output reg  S_AXI_WREADY,

    // Write response
    output reg  S_AXI_BVALID,
    input wire S_AXI_BREADY,
    output wire [1 : 0] S_AXI_BRESP,

    // Read address channel
    input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_ARADDR,
    input wire S_AXI_ARVALID,
    output reg  S_AXI_ARREADY,

    // Read data channel
    output reg  [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_RDATA,
    output reg  S_AXI_RVALID,
    input wire  S_AXI_RREADY,
    output wire [1 : 0] S_AXI_RRESP,

    // PIR & outputs
    input  wire pir_in,
    output wire led,
    output wire buzzer,
    output wire enable_capture
);

    //---------------------------------------------------------
    // AXI always OKAY
    //---------------------------------------------------------
    assign S_AXI_RRESP = 2'b00;
    assign S_AXI_BRESP = 2'b00;

    //---------------------------------------------------------
    // Registers
    //---------------------------------------------------------
    reg [31:0] slv_reg0;     // bit0 = motion_flag (RO), bit1 = enable_capture (RW)
    reg enable_cap_reg;      // single driver for enable output
    wire motion_flag;

    //---------------------------------------------------------
    // PIR CORE
    //---------------------------------------------------------
    pir_alarm PIR_core (
        .clk(S_AXI_ACLK),
        .rst_n(S_AXI_ARESETN),
        .pir_in(pir_in),
        .led(led),
        .buzzer(buzzer),
        .motion_flag(motion_flag)
    );

    assign enable_capture = enable_cap_reg;

    //---------------------------------------------------------
    // UNIFIED ALWAYS BLOCK (Fix multiple driver issue)
    //---------------------------------------------------------
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            //-------------------------------------------------
            // Reset everything
            //-------------------------------------------------
            slv_reg0       <= 0;
            enable_cap_reg <= 0;

            S_AXI_AWREADY  <= 0;
            S_AXI_WREADY   <= 0;
            S_AXI_BVALID   <= 0;
            S_AXI_ARREADY  <= 0;
            S_AXI_RVALID   <= 0;
            S_AXI_RDATA    <= 0;
        end else begin

            //-------------------------------------------------
            // PIR: update motion flag
            //-------------------------------------------------
            slv_reg0[0] <= motion_flag;

            //-------------------------------------------------
            // Auto-enable when motion detected
            //-------------------------------------------------
            if (motion_flag)
                enable_cap_reg <= 1;

            //-------------------------------------------------
            // AXI WRITE CHANNEL
            //-------------------------------------------------

            // AWREADY handshake
            if (!S_AXI_AWREADY && S_AXI_AWVALID)
                S_AXI_AWREADY <= 1;
            else
                S_AXI_AWREADY <= 0;

            // WREADY handshake
            if (!S_AXI_WREADY && S_AXI_WVALID)
                S_AXI_WREADY <= 1;
            else
                S_AXI_WREADY <= 0;

            // Write operation when both valid & ready
            if (S_AXI_AWVALID && S_AXI_WVALID &&
                S_AXI_AWREADY && S_AXI_WREADY) begin

                case (S_AXI_AWADDR[3:2])
                    2'b00: begin
                        slv_reg0[1]    <= S_AXI_WDATA[1];
                        enable_cap_reg <= S_AXI_WDATA[1];   // SINGLE DRIVER
                    end
                endcase

                S_AXI_BVALID <= 1;
            end else if (S_AXI_BVALID && S_AXI_BREADY)
                S_AXI_BVALID <= 0;

            //-------------------------------------------------
            // AXI READ CHANNEL
            //-------------------------------------------------

            // ARREADY handshake
            if (!S_AXI_ARREADY && S_AXI_ARVALID)
                S_AXI_ARREADY <= 1;
            else
                S_AXI_ARREADY <= 0;

            // Read operation
            if (S_AXI_ARVALID && S_AXI_ARREADY) begin
                case (S_AXI_ARADDR[3:2])
                    2'b00: S_AXI_RDATA <= slv_reg0;
                    default: S_AXI_RDATA <= 32'hDEADBEEF;
                endcase

                S_AXI_RVALID <= 1;

            end else if (S_AXI_RVALID && S_AXI_RREADY)
                S_AXI_RVALID <= 0;

        end
    end

endmodule
