module ov7670_capture_core(
    input wire pclk,
    input wire vsync,
    input wire href,
    input wire [7:0] d,

    output reg [15:0] pixel_data,
    output reg pixel_valid,
    output reg frame_done
);

    reg [7:0] byte_msb;
    reg toggle = 0;

    always @(posedge pclk) begin
        pixel_valid <= 0;
        frame_done <= 0;

        if (vsync) begin
            toggle <= 0;
            frame_done <= 1;
        end
        else if (href) begin
            if (!toggle) begin
                byte_msb <= d;
                toggle <= 1;
            end else begin
                pixel_data <= {byte_msb, d};
                pixel_valid <= 1;
                toggle <= 0;
            end
        end else begin
            toggle <= 0;
        end
    end

endmodule
