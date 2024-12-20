function decodeUplink(input) {
  
    var temp = input.bytes[0] << 8 | input.bytes[1];
    var ph = input.bytes[2] << 8 | input.bytes[3];
    var tds = input.bytes[4] << 24 | input.bytes[5] << 16 | input.bytes[6] << 8 | input.bytes[7];
    var ec = input.bytes[8] << 24 | input.bytes[9] << 16 | input.bytes[10] << 8 | input.bytes[11];
    var do_val = input.bytes[12] << 8 | input.bytes[13];
    var turb = input.bytes[14] << 24 | input.bytes[15] << 16 | input.bytes[16] << 8 | input.bytes[17];
  
    return {
      data: {
        Temperature: temp/100,
        pH: ph/100,
        Total_Dissolved_Solids: tds/100,
        Electrical_Conductivity: ec/100,
        Dissolved_Oxygen: do_val/100,
        Turbidity: turb/100
      }
    };
}
