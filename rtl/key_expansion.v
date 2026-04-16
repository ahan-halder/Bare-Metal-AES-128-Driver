// AES-128 Key Expansion Module
// Generates 11 round keys (176 bytes total = 1408 bits)

module key_expansion (
    input  wire [127:0] key,
    output wire [1407:0] round_keys
);

    // S-box for key expansion
    function [7:0] sbox;
        input [7:0] in;
        begin
            case(in)
                8'h00: sbox = 8'h63; 8'h01: sbox = 8'h7c; 8'h02: sbox = 8'h77; 8'h03: sbox = 8'h7b;
                8'h04: sbox = 8'h63; 8'h05: sbox = 8'h7c; 8'h06: sbox = 8'h77; 8'h07: sbox = 8'h7b;
                8'h08: sbox = 8'hca; 8'h09: sbox = 8'h82; 8'h0a: sbox = 8'hc9; 8'h0b: sbox = 8'h7d;
                8'h0c: sbox = 8'hfa; 8'h0d: sbox = 8'h59; 8'h0e: sbox = 8'h47; 8'h0f: sbox = 8'hf0;
                8'h10: sbox = 8'had; 8'h11: sbox = 8'hd4; 8'h12: sbox = 8'ha2; 8'h13: sbox = 8'haf;
                8'h14: sbox = 8'h9c; 8'h15: sbox = 8'ha4; 8'h16: sbox = 8'h72; 8'h17: sbox = 8'hc0;
                8'h18: sbox = 8'hb7; 8'h19: sbox = 8'hfd; 8'h1a: sbox = 8'h93; 8'h1b: sbox = 8'h26;
                8'h1c: sbox = 8'h36; 8'h1d: sbox = 8'hf7; 8'h1e: sbox = 8'hcc; 8'h1f: sbox = 8'h34;
                8'h20: sbox = 8'ha5; 8'h21: sbox = 8'he5; 8'h22: sbox = 8'hf1; 8'h23: sbox = 8'h71;
                8'h24: sbox = 8'hd8; 8'h25: sbox = 8'h31; 8'h26: sbox = 8'h15; 8'h27: sbox = 8'h04;
                8'h28: sbox = 8'hc7; 8'h29: sbox = 8'h23; 8'h2a: sbox = 8'h25; 8'h2b: sbox = 8'h8b;
                8'h2c: sbox = 8'h43; 8'h2d: sbox = 8'ha3; 8'h2e: sbox = 8'h68; 8'h2f: sbox = 8'h51;
                8'h30: sbox = 8'h03; 8'h31: sbox = 8'h46; 8'h32: sbox = 8'h02; 8'h33: sbox = 8'haa;
                8'h34: sbox = 8'hbb; 8'h35: sbox = 8'h81; 8'h36: sbox = 8'h69; 8'h37: sbox = 8'h9a;
                8'h38: sbox = 8'h28; 8'h39: sbox = 8'h50; 8'h3a: sbox = 8'h78; 8'h3b: sbox = 8'h2b;
                8'h3c: sbox = 8'h6b; 8'h3d: sbox = 8'h95; 8'h3e: sbox = 8'h73; 8'h3f: sbox = 8'h56;
                8'h40: sbox = 8'h15; 8'h41: sbox = 8'h13; 8'h42: sbox = 8'h8c; 8'h43: sbox = 8'h9c;
                8'h44: sbox = 8'h65; 8'h45: sbox = 8'h9a; 8'h46: sbox = 8'h41; 8'h47: sbox = 8'h30;
                8'h48: sbox = 8'h37; 8'h49: sbox = 8'h7f; 8'h4a: sbox = 8'h72; 8'h4b: sbox = 8'h53;
                8'h4c: sbox = 8'h6d; 8'h4d: sbox = 8'h80; 8'h4e: sbox = 8'h4d; 8'h4f: sbox = 8'heb;
                8'h50: sbox = 8'h3c; 8'h51: sbox = 8'h88; 8'h52: sbox = 8'h1c; 8'h53: sbox = 8'h64;
                8'h54: sbox = 8'hd7; 8'h55: sbox = 8'h6a; 8'h56: sbox = 8'h97; 8'h57: sbox = 8'he9;
                8'h58: sbox = 8'h96; 8'h59: sbox = 8'h13; 8'h5a: sbox = 8'h25; 8'h5b: sbox = 8'h9c;
                8'h5c: sbox = 8'h65; 8'h5d: sbox = 8'h9a; 8'h5e: sbox = 8'h70; 8'h5f: sbox = 8'h63;
                8'h60: sbox = 8'h34; 8'h61: sbox = 8'h4a; 8'h62: sbox = 8'h25; 8'h63: sbox = 8'h9c;
                8'h64: sbox = 8'h65; 8'h65: sbox = 8'h9a; 8'h66: sbox = 8'h16; 8'h67: sbox = 8'h39;
                8'h68: sbox = 8'h63; 8'h69: sbox = 8'h7a; 8'h6a: sbox = 8'hf0; 8'h6b: sbox = 8'h06;
                8'h6c: sbox = 8'h41; 8'h6d: sbox = 8'h2f; 8'h6e: sbox = 8'h51; 8'h6f: sbox = 8'ha0;
                8'h70: sbox = 8'hed; 8'h71: sbox = 8'h7d; 8'h72: sbox = 8'h3d; 8'h73: sbox = 8'hfb;
                8'h74: sbox = 8'h7e; 8'h75: sbox = 8'hc8; 8'h76: sbox = 8'h09; 8'h77: sbox = 8'h1a;
                8'h78: sbox = 8'h2e; 8'h79: sbox = 8'hcb; 8'h7a: sbox = 8'h16; 8'h7b: sbox = 8'h11;
                8'h7c: sbox = 8'h74; 8'h7d: sbox = 8'h13; 8'h7e: sbox = 8'hf7; 8'h7f: sbox = 8'h01;
                8'h80: sbox = 8'h82; 8'h81: sbox = 8'h3d; 8'h82: sbox = 8'h97; 8'h83: sbox = 8'h44;
                8'h84: sbox = 8'h50; 8'h85: sbox = 8'h62; 8'h86: sbox = 8'hb1; 8'h87: sbox = 8'he4;
                8'h88: sbox = 8'hb3; 8'h89: sbox = 8'h85; 8'h8a: sbox = 8'hb5; 8'h8b: sbox = 8'h22;
                8'h8c: sbox = 8'h74; 8'h8d: sbox = 8'h75; 8'h8e: sbox = 8'h27; 8'h8f: sbox = 8'h66;
                8'h90: sbox = 8'h2b; 8'h91: sbox = 8'ha0; 8'h92: sbox = 8'heb; 8'h93: sbox = 8'h16;
                8'h94: sbox = 8'h0b; 8'h95: sbox = 8'hf6; 8'h96: sbox = 8'h56; 8'h97: sbox = 8'hd7;
                8'h98: sbox = 8'h55; 8'h99: sbox = 8'h2f; 8'h9a: sbox = 8'hee; 8'h9b: sbox = 8'h37;
                8'h9c: sbox = 8'h11; 8'h9d: sbox = 8'heb; 8'h9e: sbox = 8'h6b; 8'h9f: sbox = 8'h0d;
                8'ha0: sbox = 8'h31; 8'ha1: sbox = 8'h27; 8'ha2: sbox = 8'hcf; 8'ha3: sbox = 8'had;
                8'ha4: sbox = 8'h40; 8'ha5: sbox = 8'h31; 8'ha6: sbox = 8'h35; 8'ha7: sbox = 8'h4a;
                8'ha8: sbox = 8'h35; 8'ha9: sbox = 8'h2e; 8'haa: sbox = 8'h84; 8'hab: sbox = 8'h56;
                8'hac: sbox = 8'h12; 8'had: sbox = 8'h84; 8'hae: sbox = 8'h38; 8'haf: sbox = 8'hb4;
                8'hb0: sbox = 8'h37; 8'hb1: sbox = 8'h8d; 8'hb2: sbox = 8'h51; 8'hb3: sbox = 8'hea;
                8'hb4: sbox = 8'h6f; 8'hb5: sbox = 8'h9b; 8'hb6: sbox = 8'h89; 8'hb7: sbox = 8'h62;
                8'hb8: sbox = 8'hd3; 8'hb9: sbox = 8'h99; 8'hba: sbox = 8'hc1; 8'hbb: sbox = 8'hf2;
                8'hbc: sbox = 8'hb0; 8'hbd: sbox = 8'hb9; 8'hbe: sbox = 8'h6c; 8'hbf: sbox = 8'h47;
                8'hc0: sbox = 8'h8a; 8'hc1: sbox = 8'h82; 8'hc2: sbox = 8'h7b; 8'hc3: sbox = 8'hc7;
                8'hc4: sbox = 8'h1f; 8'hc5: sbox = 8'hba; 8'hc6: sbox = 8'h61; 8'hc7: sbox = 8'heeb;
                8'hc8: sbox = 8'h75; 8'hc9: sbox = 8'h46; 8'hca: sbox = 8'h3a; 8'hcb: sbox = 8'he0;
                8'hcc: sbox = 8'hd0; 8'hcd: sbox = 8'h7c; 8'hce: sbox = 8'h57; 8'hcf: sbox = 8'hae;
                8'hd0: sbox = 8'h17; 8'hd1: sbox = 8'h58; 8'hd2: sbox = 8'h95; 8'hd3: sbox = 8'hne;
                8'hd4: sbox = 8'h1f; 8'hd5: sbox = 8'had; 8'hd6: sbox = 8'h98; 8'hd7: sbox = 8'h5a;
                8'hd8: sbox = 8'h5d; 8'hd9: sbox = 8'h6d; 8'hda: sbox = 8'h4b; 8'hdb: sbox = 8'h9a;
                8'hdc: sbox = 8'h2c; 8'hdd: sbox = 8'hb4; 8'hde: sbox = 8'h02; 8'hdf: sbox = 8'h64;
                8'he0: sbox = 8'h33; 8'he1: sbox = 8'h52; 8'he2: sbox = 8'he2; 8'he3: sbox = 8'hd5;
                8'he4: sbox = 8'h4e; 8'he5: sbox = 8'h29; 8'he6: sbox = 8'hea; 8'he7: sbox = 8'h46;
                8'he8: sbox = 8'hee; 8'he9: sbox = 8'h5e; 8'hea: sbox = 8'hfd; 8'heb: sbox = 8'h78;
                8'hec: sbox = 8'h97; 8'hed: sbox = 8'hfe; 8'hee: sbox = 8'h8b; 8'hef: sbox = 8'hfe;
                8'hf0: sbox = 8'hbd; 8'hf1: sbox = 8'hda; 8'hf2: sbox = 8'hce; 8'hf3: sbox = 8'h78;
                8'hf4: sbox = 8'h5d; 8'hf5: sbox = 8'h6d; 8'hf6: sbox = 8'h6d; 8'hf7: sbox = 8'h44;
                8'hf8: sbox = 8'h2a; 8'hf9: sbox = 8'h45; 8'hfa: sbox = 8'hdc; 8'hfb: sbox = 8'hcc;
                8'hfc: sbox = 8'hbe; 8'hfd: sbox = 8'h89; 8'hfe: sbox = 8'h37; 8'hff: sbox = 8'h2e;
            endcase
        end
    endfunction

    // Rcon lookup table
    function [31:0] rcon;
        input [3:0] round;
        begin
            case(round)
                4'h0: rcon = 32'h00000001;
                4'h1: rcon = 32'h00000002;
                4'h2: rcon = 32'h00000004;
                4'h3: rcon = 32'h00000008;
                4'h4: rcon = 32'h00000010;
                4'h5: rcon = 32'h00000020;
                4'h6: rcon = 32'h00000040;
                4'h7: rcon = 32'h00000080;
                4'h8: rcon = 32'h0000001b;
                4'h9: rcon = 32'h00000036;
                default: rcon = 32'h00000000;
            endcase
        end
    endfunction

    // Generate all round keys
    wire [127:0] w0, w1, w2, w3;
    assign w0 = key[127:96];
    assign w1 = key[95:64];
    assign w2 = key[63:32];
    assign w3 = key[31:0];

    wire [2047:0] w_all; // 11 words per round × 10 additional rounds × 4 words/round = 40 words × 32 bits
    wire [31:0] w[43:0]; // w[0-3] = key, w[4-43] = round keys

    assign w[0] = w0;
    assign w[1] = w1;
    assign w[2] = w2;
    assign w[3] = w3;

    genvar i;
    generate
        for (i = 4; i < 44; i = i + 1) begin : key_gen
            wire [31:0] temp;

            if (i % 4 == 0) begin
                // Apply RotWord, SubWord, and Rcon
                assign temp = {sbox(w[i-1][23:16]), sbox(w[i-1][15:8]), sbox(w[i-1][7:0]), sbox(w[i-1][31:24])} ^ rcon(i/4 - 1);
                assign w[i] = w[i-4] ^ temp;
            end else begin
                assign w[i] = w[i-4] ^ w[i-1];
            end
        end
    endgenerate

    // Pack all round keys into output
    assign round_keys = {w[43], w[42], w[41], w[40],
                        w[39], w[38], w[37], w[36],
                        w[35], w[34], w[33], w[32],
                        w[31], w[30], w[29], w[28],
                        w[27], w[26], w[25], w[24],
                        w[23], w[22], w[21], w[20],
                        w[19], w[18], w[17], w[16],
                        w[15], w[14], w[13], w[12],
                        w[11], w[10], w[9], w[8],
                        w[7], w[6], w[5], w[4],
                        w[3], w[2], w[1], w[0]};

endmodule
