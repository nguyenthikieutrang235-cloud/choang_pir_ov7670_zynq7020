`timescale 1ns / 1ps
module pir_alarm(
    input  wire clk,
    input  wire rst_n,
    input  wire pir_in,
    output reg  led,
    output reg  buzzer,
    output reg  motion_flag
);

    //------------------------------------------
    // 1) ??ng b? tín hi?u vào clock domain
    //------------------------------------------
    reg pir_sync1, pir_sync2;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pir_sync1 <= 0;
            pir_sync2 <= 0;
        end else begin
            pir_sync1 <= pir_in;
            pir_sync2 <= pir_sync1;
        end
    end

    //------------------------------------------
    // 2) Phát hi?n c?nh lên (edge detect)
    //------------------------------------------
    reg pir_last;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            pir_last <= 0;
        else
            pir_last <= pir_sync2;
    end

    wire pir_rise = pir_sync2 && !pir_last;

    //------------------------------------------
    // 3) motion_flag latch khi có chuy?n ??ng
    // PS s? reset b?ng cách ghi t? AXI
    //------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            motion_flag <= 0;
        end else begin
            if (pir_rise)
                motion_flag <= 1;
            // ? PH?N NÀY PS RESET T? AXI
            // b?n s? n?i motion_flag <= reset_signal trong S00_AXI
        end
    end

    //------------------------------------------
    // 4) LED và buzzer d?a trên motion_flag
    //------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            led <= 0;
            buzzer <= 0;
        end else begin
            led    <= motion_flag;
            buzzer <= motion_flag;
        end
    end

endmodule
