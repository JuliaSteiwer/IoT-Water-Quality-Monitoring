function decodeUplink(input) {
  
  var temp = input.bytes[0] << 8 | input.bytes[1];
  var ph = input.bytes[2] << 8 | input.bytes[3];
  var tds = input.bytes[4] << 8 | input.bytes[5];
  var ec = input.bytes[6] << 8 | input.bytes[7];
  var doVal = input.bytes[8] << 8 | input.bytes[9];
  var ntu = input.bytes[10] << 8 | input.bytes[11];
  
  return {
    data: {
      field1: temp/100,
      field2: ph/100,
      field3: tds/100,
      field4: ec/100,
      field5: doVal/100,
      field6: ntu/100
    },
    warnings: [],
    errors: []
  };
}
