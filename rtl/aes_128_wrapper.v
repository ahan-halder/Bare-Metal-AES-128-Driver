// AES-128 Memory-Mapped Register Wrapper
// Register Map:
// 0x00: CTRL (control register) [1:0] = {mode, start}
// 0x04: STATUS (status register) [1:0] = {done, busy}
// 0x08-0x0F: KEY (128-bit key, 4 x 32-bit registers)
// 0x10-0x17: DATA_IN (128-bit input, 4 x 32-bit registers)
// 0x18-0x1F: DATA_OUT (128-bit output, 4 x 32-bit registers)

module aes_128_wrapper (
    input  wire         clk,
    input  wire         rst,

    // MMIO Interface (32-bit bus)
    input  wire [31:0]  addr,
    input  wire [31:0]  write_data,
    output wire [31:0]  read_data,
    input  wire         write_en,
    input  wire         read_en,

    // Interrupt
    output wire         irq
);

    // Internal registers
    reg  [1:0]  ctrl;          // [1]: mode (0=encrypt, 1=decrypt), [0]: start
    reg  [127:0] key_reg;
    reg  [127:0] data_in_reg;
    reg  [127:0] data_out_reg;
    reg  [1:0]  status;        // [1]: done, [0]: busy

    // AES Core
    wire [127:0] aes_output;
    wire         aes_done;

    aes_128_core aes_inst (
        .clk(clk),
        .rst(rst),
        .start(ctrl[0]),
        .plaintext(data_in_reg),
        .key(key_reg),
        .encrypt(~ctrl[1]),  // mode: 0=encrypt, 1=decrypt
        .ciphertext(aes_output),
        .done(aes_done)
    );

    // Status tracking
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            status <= 2'b00;
        end else begin
            if (ctrl[0]) begin
                status[0] <= 1'b1;  // Mark busy
            end else if (aes_done) begin
                status <= 2'b10;    // Mark done
                data_out_reg <= aes_output;
            end else begin
                status <= 2'b00;    // Idle
            end
        end
    end

    // Clear start bit after one cycle
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            ctrl[0] <= 1'b0;
        end else if (ctrl[0]) begin
            ctrl[0] <= 1'b0;  // Auto-clear after one cycle
        end
    end

    // Write interface
    always @(posedge clk) begin
        if (write_en) begin
            case (addr[4:2])
                3'b000: ctrl <= write_data[1:0];                    // CTRL register
                3'b001: begin end                                    // STATUS register (read-only)
                3'b010: key_reg[127:96] <= write_data[31:0];       // KEY[0]
                3'b011: key_reg[95:64]  <= write_data[31:0];       // KEY[1]
                3'b100: key_reg[63:32]  <= write_data[31:0];       // KEY[2]
                3'b101: key_reg[31:0]   <= write_data[31:0];       // KEY[3]
                3'b110: data_in_reg[127:96] <= write_data[31:0];   // DATA_IN[0]
                3'b111: data_in_reg[95:64]  <= write_data[31:0];   // DATA_IN[1]
                default: begin end
            endcase
        end
    end

    // Additional write ports for DATA_IN[2:3]
    always @(posedge clk) begin
        if (write_en && addr[5]) begin
            case (addr[3:2])
                2'b00: data_in_reg[63:32]  <= write_data[31:0];    // DATA_IN[2]
                2'b01: data_in_reg[31:0]   <= write_data[31:0];    // DATA_IN[3]
                2'b10: begin end                                     // DATA_OUT[0] (read-only)
                2'b11: begin end                                     // DATA_OUT[1] (read-only)
                default: begin end
            endcase
        end
    end

    // Read interface
    assign read_data = (read_en) ?
        (addr[4:2] == 3'b000) ? {30'h0, ctrl} :
        (addr[4:2] == 3'b001) ? {30'h0, status} :
        (addr[4:2] == 3'b010) ? key_reg[127:96] :
        (addr[4:2] == 3'b011) ? key_reg[95:64] :
        (addr[4:2] == 3'b100) ? key_reg[63:32] :
        (addr[4:2] == 3'b101) ? key_reg[31:0] :
        (addr[5]) && (addr[3:2] == 2'b10) ? data_out_reg[127:96] :
        (addr[5]) && (addr[3:2] == 2'b11) ? data_out_reg[95:64] :
        (addr[5] == 1'b0) && (addr[3:2] == 2'b10) ? data_in_reg[127:96] :
        (addr[5] == 1'b0) && (addr[3:2] == 2'b11) ? data_in_reg[95:64] :
        32'h00000000 : 32'h00000000;

    // Interrupt: assert when done
    assign irq = status[1];

endmodule
