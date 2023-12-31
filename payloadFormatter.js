function decodeUplink(input) {
  
  var temp = input.bytes[0] << 8 | input.bytes[1];
  var ph = input.bytes[2] << 8 | input.bytes[3];
  
  return {
    data: {
      field1: temp/100,
      field2: ph/100
    },
    warnings: [],
    errors: []
  };
}
