// AES-128 Complete Encryption/Decryption Core
// Supports both encryption and decryption operations

module aes_128_core (
    input  wire         clk,
    input  wire         rst,
    input  wire         start,
    input  wire [127:0] plaintext,
    input  wire [127:0] key,
    input  wire         encrypt,   // 1: encrypt, 0: decrypt
    output reg  [127:0] ciphertext,
    output reg          done
);

    // Internal registers
    reg  [3:0]   round;
    reg  [127:0] state;
    reg          busy;

    // Round key storage (11 x 128 bits)
    wire [1407:0] round_keys;
    reg  [127:0]  round_key;

    // Encryption wires
    wire [127:0] sub_bytes_out;
    wire [127:0] shift_rows_out;
    wire [127:0] mix_columns_out;

    // Decryption wires - using existing modules from project
    wire [127:0] inv_shift_rows_out;
    wire [127:0] inv_sub_bytes_out;
    wire [127:0] inv_mix_columns_out;
    wire [127:0] after_key_xor;

    // Key Expansion Module
    key_expansion key_exp (
        .key(key),
        .round_keys(round_keys)
    );

    // Encryption Path Modules
    sub_bytes   encrypt_sb  (.in(state),         .out(sub_bytes_out));
    shift_rows  encrypt_sr  (.in(sub_bytes_out), .out(shift_rows_out));
    mix_columns encrypt_mc  (.in(shift_rows_out), .out(mix_columns_out));

    // Decryption Path Modules
    InvShiftRows      decrypt_isr (.state_in(state),           .state_out(inv_shift_rows_out));
    InvSubstitutionMatrix decrypt_isb (.Data(inv_shift_rows_out), .Data_out(inv_sub_bytes_out));
    InvMixColumns     decrypt_imc (.state_in(after_key_xor),   .state_out(inv_mix_columns_out));

    // XOR after InvSubBytes with round key
    assign after_key_xor = inv_sub_bytes_out ^ round_key;

    // Round Key Multiplexer
    always @(*) begin
        if (encrypt) begin
            // Encryption: round i uses round_keys[10-i]
            round_key = (round == 0) ? round_keys[127:0] :
                       (round == 1) ? round_keys[255:128] :
                       (round == 2) ? round_keys[383:256] :
                       (round == 3) ? round_keys[511:384] :
                       (round == 4) ? round_keys[639:512] :
                       (round == 5) ? round_keys[767:640] :
                       (round == 6) ? round_keys[895:768] :
                       (round == 7) ? round_keys[1023:896] :
                       (round == 8) ? round_keys[1151:1024] :
                       (round == 9) ? round_keys[1279:1152] :
                       round_keys[1407:1280];
        end else begin
            // Decryption: reverse order
            round_key = (round == 0) ? round_keys[1407:1280] :
                       (round == 1) ? round_keys[1279:1152] :
                       (round == 2) ? round_keys[1151:1024] :
                       (round == 3) ? round_keys[1023:896] :
                       (round == 4) ? round_keys[895:768] :
                       (round == 5) ? round_keys[767:640] :
                       (round == 6) ? round_keys[639:512] :
                       (round == 7) ? round_keys[511:384] :
                       (round == 8) ? round_keys[383:256] :
                       (round == 9) ? round_keys[255:128] :
                       round_keys[127:0];
        end
    end

    // Main State Machine
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            round      <= 0;
            state      <= 128'h0;
            ciphertext <= 128'h0;
            done       <= 1'b0;
            busy       <= 1'b0;
        end else if (start && !busy) begin
            // Initial state setup
            busy  <= 1'b1;
            done  <= 1'b0;
            round <= 4'h0;
            // AddRoundKey with round 0 key
            if (encrypt) begin
                state <= plaintext ^ round_keys[127:0];
            end else begin
                state <= plaintext ^ round_keys[1407:1280];
            end
        end else if (busy && round < 10) begin
            // Rounds 1-9
            round <= round + 4'h1;
            if (encrypt) begin
                // Encryption: SubBytes -> ShiftRows -> MixColumns -> AddRoundKey
                state <= mix_columns_out ^ round_key;
            end else begin
                // Decryption: InvShiftRows -> InvSubBytes -> AddRoundKey -> InvMixColumns
                state <= inv_mix_columns_out;
            end
        end else if (busy && round == 10) begin
            // Final round (no MixColumns)
            if (encrypt) begin
                // Encryption final: SubBytes -> ShiftRows -> AddRoundKey
                ciphertext <= shift_rows_out ^ round_key;
            end else begin
                // Decryption final: InvShiftRows -> InvSubBytes -> AddRoundKey
                ciphertext <= after_key_xor;
            end
            busy  <= 1'b0;
            done  <= 1'b1;
            round <= 4'h0;
        end
    end

endmodule
